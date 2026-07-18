#include "zh_vector.h"

static const char *TAG = "zh_vector";

#define ZH_LOGI(msg, ...) ESP_LOGI(TAG, msg, ##__VA_ARGS__)
#define ZH_LOGE(msg, err, ...) ESP_LOGE(TAG, "[%s:%d:%s] " msg, __FILE__, __LINE__, esp_err_to_name(err), ##__VA_ARGS__)

#define ZH_ERROR_CHECK(cond, err, cleanup, msg, ...) \
    if (!(cond))                                     \
    {                                                \
        ZH_LOGE(msg, err, ##__VA_ARGS__);            \
        cleanup;                                     \
        return err;                                  \
    }

#define ZH_ERROR_CHECK_VOID(cond, cleanup, msg, ...) \
    if (!(cond))                                     \
    {                                                \
        ZH_LOGE(msg, ESP_FAIL, ##__VA_ARGS__);       \
        cleanup;                                     \
        return;                                      \
    }

/**
 * @brief Internal representation of a thread-safe, dynamically resizing vector of *copied* elements.
 *
 * Stores pointers to heap-allocated *copies* of elements (not raw pointers).
 * Elements (stored in items[i]) are allocated via heap_caps_calloc(1, unit, ...) and freed via heap_caps_free.
 * Supports push front/back, change, delete and size queries.
 * Thread safety via FreeRTOS mutex.
 *
 * @note Opaque type: only zh_vector_t (typedef struct _zh_vector_t* zh_vector_t) is exposed.
 * @note Elements are deep-copied — ownership of input item is NOT transferred.
 * @note All public functions lock the mutex internally — no external locking required.
 *       Internal functions _resize() and _calc_new_capacity() are not thread-safe and must be called
 *       only while the vector's mutex is held.
 * @note Internal function _resize() guarantees invariant: 0 ≤ size ≤ capacity.
 *       In particular, _resize(..., 0) always sets size = 0, capacity = 0, and frees all element buffers.
 * @note The capacity may exceed size after deletions (memory is not shrunk immediately),
 *       and is reduced only via _resize() on explicit request (e.g., during delete).
 *
 * @warning Do not access fields directly — always use public API.
 * @warning unit must match the actual element size — mismatch causes heap corruption.
 * @note capacity and size are limited to uint16_t (max 65535 elements), to save memory on embedded devices.
 * @note items is allocated via heap_caps_realloc as `void*`, but logically treated as `void**`.
 *       Each valid items[i] (0 ≤ i < size) points to a heap buffer of exactly `unit` bytes (allocated via heap_caps_calloc(1, unit, ...)).
 *       Only the first `unit` bytes are used for the copied item.
 * @warning Actual memory usage depends on allocator overhead and alignment.
 *          Rough estimate: `sizeof(zh_vector_t) + capacity * sizeof(void*) + size * (unit + item_overhead)`.
 *          Typical heap overhead per item: ~16 bytes (metadata + alignment).
 * @note Capacity reduction is triggered when `size < capacity / 2` (integer division). For example:
 *       - capacity=4, size=1 → capacity=1 (4/2=2 > 1)
 *       - capacity=2, size=1 → capacity stays 2 (2/2=1 not > 1)
 *       - capacity=1, size=0 → capacity=0 (handled separately: size==0 && capacity>0)
 * @note When size reaches 0, capacity is always set to 0, even without explicit resize request.
 */
struct _zh_vector_t
{
    void **items;            /*!< Array of element pointers. items[0..size-1] are valid and non-NULL. Initially NULL; allocated via heap_caps_realloc (as `void*`, cast to `void**`). When capacity > old_capacity, new slots (items[old_capacity..capacity-1]) are zeroed (NULL). */
    uint16_t capacity;       /*!< Current allocated capacity (number of slots). Grows on insertion, shrinks on delete (see _resize). Limited to uint16_t (max 65535). */
    uint16_t size;           /*!< Current number of valid elements (0 ≤ size ≤ capacity). Always kept in sync: _resize(..., 0) sets size = 0; after push operations size++; after delete size--. */
    uint16_t unit;           /*!< Size (in bytes) of a single element. Set once in zh_vector_init() and immutable. Must match the actual element size — mismatch causes heap corruption. */
    SemaphoreHandle_t mutex; /*!< FreeRTOS mutex. Created in zh_vector_init() and deleted in zh_vector_free(). All public functions acquire it before accessing/modifying the vector state. The mutex itself does not affect logical state (`size`, `capacity`). */
};

static esp_err_t _resize(zh_vector_t *vector, uint16_t capacity);
static esp_err_t _delete(zh_vector_t *vector, uint16_t index);
static inline uint16_t _calc_new_capacity(uint16_t current);

esp_err_t zh_vector_init(zh_vector_t **vector, uint16_t unit) // -V2008
{
    ZH_LOGI("Vector initialization begin.");
    ZH_ERROR_CHECK(vector != NULL && unit > 0, ESP_ERR_INVALID_ARG, NULL, "Vector initialization failed. Invalid argument.");
    ZH_ERROR_CHECK(*vector == NULL, ESP_ERR_INVALID_STATE, NULL, "Vector initialization failed. Vector is already initialized.");
    *vector = heap_caps_calloc(1, sizeof(zh_vector_t), MALLOC_CAP_8BIT);
    ZH_ERROR_CHECK(*vector != NULL, ESP_ERR_NO_MEM, NULL, "Vector initialization failed. Failed to allocate vector structure.");
    (*vector)->mutex = xSemaphoreCreateMutex();
    ZH_ERROR_CHECK((*vector)->mutex != NULL, ESP_ERR_NO_MEM, heap_caps_free(*vector); *vector = NULL, "Vector initialization failed. Failed to create mutex.");
    (*vector)->items = NULL;
    (*vector)->capacity = 0;
    (*vector)->size = 0;
    (*vector)->unit = unit;
    ZH_LOGI("Vector initialization success.");
    return ESP_OK;
}

esp_err_t zh_vector_free(zh_vector_t **vector)
{
    ZH_LOGI("Vector deletion begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL, ESP_ERR_INVALID_ARG, NULL, "Vector deletion failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Vector deletion failed. Failed to acquire mutex.");
    for (uint16_t i = 0; i < (*vector)->size; ++i)
    {
        if ((*vector)->items[i] != NULL)
        {
            heap_caps_free((*vector)->items[i]);
            (*vector)->items[i] = NULL;
        }
    }
    heap_caps_free((*vector)->items);
    (*vector)->items = NULL;
    (*vector)->size = 0;
    (*vector)->capacity = 0;
    vSemaphoreDelete((*vector)->mutex);
    (*vector)->mutex = NULL;
    heap_caps_free(*vector);
    *vector = NULL;
    ZH_LOGI("Vector deletion success.");
    return ESP_OK;
}

esp_err_t zh_vector_get_size(zh_vector_t **vector, uint16_t *size)
{
    ZH_LOGI("Getting vector size begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && size != NULL, ESP_ERR_INVALID_ARG, NULL, "Getting vector size failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Getting vector size failed. Failed to acquire mutex.");
    *size = (*vector)->size;
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Getting vector size success.");
    return ESP_OK;
}

esp_err_t zh_vector_get_capacity(zh_vector_t **vector, uint16_t *capacity)
{
    ZH_LOGI("Getting vector capacity begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && capacity != NULL, ESP_ERR_INVALID_ARG, NULL, "Getting vector capacity failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Getting vector capacity failed. Failed to acquire mutex.");
    *capacity = (*vector)->capacity;
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Getting vector capacity success.");
    return ESP_OK;
}

esp_err_t zh_vector_push_front(zh_vector_t **vector, const void *item) // -V2008
{
    ZH_LOGI("Adding item to beginning of vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Adding item to beginning of vector failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Adding item to beginning of vector failed. Failed to acquire mutex.");
    ZH_ERROR_CHECK((*vector)->size < UINT16_MAX, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to beginning of vector failed. Vector is full.");
    if ((*vector)->capacity == (*vector)->size)
    {
        uint16_t new_capacity = _calc_new_capacity((*vector)->capacity);
        ZH_ERROR_CHECK(_resize(*vector, new_capacity) == ESP_OK, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to beginning of vector failed. Memory reallocation failed.");
    }
    for (uint16_t i = (*vector)->size; i > 0; --i)
    {
        (*vector)->items[i] = (*vector)->items[i - 1];
    }
    (*vector)->items[0] = heap_caps_calloc(1, (*vector)->unit, MALLOC_CAP_8BIT);
    ZH_ERROR_CHECK((*vector)->items[0] != NULL, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to beginning of vector failed. Element allocation failed.");
    memcpy((*vector)->items[0], item, (*vector)->unit);
    (*vector)->size++;
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Adding item to beginning of vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_push_back(zh_vector_t **vector, const void *item) // -V2008
{
    ZH_LOGI("Adding item to vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Adding item to vector failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Adding item to vector failed. Failed to acquire mutex.");
    ZH_ERROR_CHECK((*vector)->size < UINT16_MAX, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to vector failed. Vector is full.");
    if ((*vector)->capacity == (*vector)->size)
    {
        uint16_t new_capacity = _calc_new_capacity((*vector)->capacity);
        ZH_ERROR_CHECK(_resize(*vector, new_capacity) == ESP_OK, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to vector failed. Memory reallocation failed.");
    }
    uint16_t idx = (*vector)->size;
    (*vector)->items[idx] = heap_caps_calloc(1, (*vector)->unit, MALLOC_CAP_8BIT);
    ZH_ERROR_CHECK((*vector)->items[idx] != NULL, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to vector failed. Element allocation failed.");
    memcpy((*vector)->items[idx], item, (*vector)->unit);
    (*vector)->size++;
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Adding item to vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_change_item(zh_vector_t **vector, uint16_t index, const void *item) // -V2008
{
    ZH_LOGI("Changing item in vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Changing item in vector failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Changing item in vector failed. Failed to acquire mutex.");
    ZH_ERROR_CHECK(index < (*vector)->size, ESP_ERR_INVALID_ARG, xSemaphoreGive((*vector)->mutex), "Changing item in vector failed. Index out of bounds.");
    ZH_ERROR_CHECK((*vector)->items[index] != NULL, ESP_ERR_INVALID_STATE, xSemaphoreGive((*vector)->mutex), "Changing item in vector failed. Item is NULL.");
    memcpy((*vector)->items[index], item, (*vector)->unit);
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Changing item in vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_get_item(zh_vector_t **vector, uint16_t index, void *item) // -V2008
{
    ZH_LOGI("Getting item from vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Getting item from vector failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Getting item from vector failed. Failed to acquire mutex.");
    ZH_ERROR_CHECK(index < (*vector)->size, ESP_ERR_INVALID_ARG, xSemaphoreGive((*vector)->mutex), "Getting item from vector failed. Index out of bounds.");
    ZH_ERROR_CHECK((*vector)->items[index] != NULL, ESP_ERR_INVALID_STATE, xSemaphoreGive((*vector)->mutex), "Getting item from vector failed. Item is NULL.");
    memcpy(item, (*vector)->items[index], (*vector)->unit);
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Getting item from vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_delete_item(zh_vector_t **vector, uint16_t index) // -V2008
{
    ZH_LOGI("Deleting item in vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL, ESP_ERR_INVALID_ARG, NULL, "Deleting item in vector failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Deleting item in vector failed. Failed to acquire mutex.");
    ZH_ERROR_CHECK(index < (*vector)->size, ESP_ERR_INVALID_ARG, xSemaphoreGive((*vector)->mutex), "Deleting item in vector failed. Index out of bounds.");
    ZH_ERROR_CHECK(_delete(*vector, index) == ESP_OK, ESP_ERR_INVALID_STATE, xSemaphoreGive((*vector)->mutex), "Deleting item in vector failed. Internal error.");
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Deleting item in vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_remove_duplicates(zh_vector_t **vector) // -V2008
{
    ZH_LOGI("Removing duplicates from vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL, ESP_ERR_INVALID_ARG, NULL, "Removing duplicates from vector failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Removing duplicates from vector failed. Failed to acquire mutex.");
    if ((*vector)->size >= 2)
    {
        for (uint16_t i = 0; i < (*vector)->size - 1; ++i)
        {
            uint16_t j = i + 1;
            while (j < (*vector)->size)
            {
                if (memcmp((*vector)->items[i], (*vector)->items[j], (*vector)->unit) == 0)
                {
                    ZH_ERROR_CHECK(_delete(*vector, j) == ESP_OK, ESP_ERR_INVALID_STATE, xSemaphoreGive((*vector)->mutex), "Removing duplicates from vector failed. Internal error.");
                }
                else
                {
                    ++j;
                }
            }
        }
    }
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Removing duplicates from vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_find_item(zh_vector_t **vector, const void *item, int32_t *index)
{
    ZH_LOGI("Finding item in vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && item != NULL && index != NULL, ESP_ERR_INVALID_ARG, NULL, "Finding item in vector failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Finding item in vector failed. Failed to acquire mutex.");
    *index = -1;
    for (uint16_t i = 0; i < (*vector)->size; ++i)
    {
        if (memcmp((*vector)->items[i], item, (*vector)->unit) == 0)
        {
            *index = (int32_t)i;
            xSemaphoreGive((*vector)->mutex);
            ZH_LOGI("Finding item in vector success (found).");
            return ESP_OK;
        }
    }
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Finding item in vector success (not found).");
    return ESP_ERR_NOT_FOUND;
}

esp_err_t zh_vector_find_item_in_field(zh_vector_t **vector, const void *sample_struct, const void *item, size_t size, const void *value, uint16_t start, int32_t *index)
{
    ZH_LOGI("Finding field in structure begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && sample_struct != NULL && item != NULL && value != NULL && index != NULL && size > 0, ESP_ERR_INVALID_ARG, NULL, "Finding field in structure failed. Invalid argument.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Finding field in structure failed. Mutex acquire failed.");
    ZH_ERROR_CHECK(start < (*vector)->size, ESP_ERR_INVALID_ARG, xSemaphoreGive((*vector)->mutex), "Finding field in structure failed. Start index out of bounds.");
    size_t offset = (const uint8_t *)item - (const uint8_t *)sample_struct;
    ZH_ERROR_CHECK((offset + size) <= (*vector)->unit, ESP_ERR_INVALID_ARG, xSemaphoreGive((*vector)->mutex), "Finding field in structure failed. Field exceeds element size.");
    *index = -1;
    for (uint16_t i = start; i < (*vector)->size; ++i)
    {
        const uint8_t *elem = (const uint8_t *)(*vector)->items[i];
        if (memcmp(elem + offset, value, size) == 0)
        {
            *index = (int32_t)i;
            xSemaphoreGive((*vector)->mutex);
            ZH_LOGI("Finding field in structure success (found).");
            return ESP_OK;
        }
    }
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Finding field in structure success (not found).");
    return ESP_ERR_NOT_FOUND;
}

static esp_err_t _resize(zh_vector_t *vector, uint16_t capacity)
{
    ZH_ERROR_CHECK(capacity >= vector->size, ESP_ERR_INVALID_ARG, NULL, "Invalid argument.");
    if (capacity == 0)
    {
        if (vector->items != NULL)
        {
            for (uint16_t i = 0; i < vector->size; ++i)
            {
                heap_caps_free(vector->items[i]);
                vector->items[i] = NULL;
            }
        }
        heap_caps_free(vector->items);
        vector->items = NULL;
        vector->size = 0;
        vector->capacity = 0;
        return ESP_OK;
    }
    void *new_items = heap_caps_realloc(vector->items, sizeof(void *) * capacity, MALLOC_CAP_8BIT);
    ZH_ERROR_CHECK(new_items != NULL, ESP_ERR_NO_MEM, NULL, "Memory reallocation failed.");
    uint16_t old_capacity = vector->capacity;
    vector->items = new_items;
    if (capacity > old_capacity)
    {
        memset(vector->items + old_capacity, 0, sizeof(void *) * (capacity - old_capacity));
    }
    vector->capacity = capacity;
    return ESP_OK;
}

static esp_err_t _delete(zh_vector_t *vector, uint16_t index)
{
    ZH_ERROR_CHECK(index < vector->size, ESP_ERR_INVALID_ARG, NULL, "Invalid argument.");
    void *freed_item = vector->items[index];
    uint16_t last_idx = vector->size - 1;
    if (index != last_idx)
    {
        for (uint16_t i = index; i < last_idx; ++i)
        {
            vector->items[i] = vector->items[i + 1];
        }
    }
    vector->items[last_idx] = NULL;
    heap_caps_free(freed_item);
    freed_item = NULL;
    --vector->size;
    if (vector->size > 0 && vector->capacity / 2 > vector->size)
    {
        return _resize(vector, vector->size);
    }
    else if (vector->size == 0 && vector->capacity > 0)
    {
        _resize(vector, 0);
    }
    return ESP_OK;
}

static inline uint16_t _calc_new_capacity(uint16_t current)
{
    if (current == 0)
    {
        return 4;
    }
    else if (current > UINT16_MAX / 2)
    {
        return UINT16_MAX;
    }
    else
    {
        return current * 2;
    }
}