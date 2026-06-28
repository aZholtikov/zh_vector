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

#define ZH_VECTOR_MAGIC 0x5A485643

struct _zh_vector_t
{
    void **items;            /*!< Array of pointers of vector items. */
    uint16_t capacity;       /*!< Maximum capacity of the vector. @note Used to control the size of allocated memory for array of pointers of vector items. Usually equal to the current number of items in the vector. Automatically changes when items are added or deleted. */
    uint16_t size;           /*!< Number of items in the vector. */
    uint16_t unit;           /*!< Vector item size. */
    uint32_t is_initialized; /*!< Vector initialization status flag. */
    SemaphoreHandle_t mutex; /*!< FreeRTOS mutex. */
};

static esp_err_t _resize(zh_vector_t *vector, uint16_t capacity);

esp_err_t zh_vector_init(zh_vector_t **vector, uint16_t unit) // -V2008
{
    ZH_LOGI("Vector initialization begin.");
    ZH_ERROR_CHECK(vector != NULL && unit > 0, ESP_ERR_INVALID_ARG, NULL, "Vector initialization failed. Invalid argument.");
    if (*vector == NULL)
    {
        *vector = heap_caps_calloc(1, sizeof(zh_vector_t), MALLOC_CAP_8BIT);
        ZH_ERROR_CHECK(*vector != NULL, ESP_ERR_NO_MEM, NULL, "Vector initialization failed. Failed to allocate vector structure.");
    }
    else
    {
        ZH_ERROR_CHECK((*vector)->is_initialized != ZH_VECTOR_MAGIC, ESP_ERR_INVALID_STATE, NULL, "Vector initialization failed. Vector is already initialized.");
        if ((*vector)->mutex != NULL)
        {
            vSemaphoreDelete((*vector)->mutex);
            (*vector)->mutex = NULL;
        }
        if ((*vector)->items != NULL)
        {
            heap_caps_free((*vector)->items);
            (*vector)->items = NULL;
            (*vector)->size = 0;
            (*vector)->capacity = 0;
        }
    }
    (*vector)->mutex = xSemaphoreCreateMutex();
    ZH_ERROR_CHECK((*vector)->mutex != NULL, ESP_ERR_NO_MEM, heap_caps_free(*vector); *vector = NULL, "Vector initialization failed. Failed to create mutex.");
    (*vector)->items = NULL;
    (*vector)->capacity = 0;
    (*vector)->size = 0;
    (*vector)->unit = unit;
    (*vector)->is_initialized = ZH_VECTOR_MAGIC;
    ZH_LOGI("Vector initialization success.");
    return ESP_OK;
}

esp_err_t zh_vector_free(zh_vector_t **vector)
{
    ZH_LOGI("Vector deletion begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL, ESP_ERR_INVALID_ARG, NULL, "Vector deletion failed. Invalid argument.");
    ZH_ERROR_CHECK((*vector)->is_initialized == ZH_VECTOR_MAGIC, ESP_ERR_INVALID_STATE, NULL, "Vector deletion fail. Vector not initialized.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Vector deletion fail. Failed to acquire mutex.");
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
    (*vector)->is_initialized = 0;
    heap_caps_free(*vector);
    *vector = NULL;
    ZH_LOGI("Vector deletion success.");
    return ESP_OK;
}

esp_err_t zh_vector_get_size(zh_vector_t **vector, size_t *size)
{
    ZH_LOGI("Getting vector size begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && size != NULL, ESP_ERR_INVALID_ARG, NULL, "Getting vector size fail. Invalid argument.");
    ZH_ERROR_CHECK((*vector)->is_initialized == ZH_VECTOR_MAGIC, ESP_ERR_INVALID_STATE, NULL, "Getting vector size fail. Vector not initialized.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Getting vector size fail. Failed to acquire mutex.");
    *size = (*vector)->size;
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Getting vector size success.");
    return ESP_OK;
}

esp_err_t zh_vector_push_front(zh_vector_t **vector, const void *item) // -V2008
{
    ZH_LOGI("Adding item to beginning of vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Adding item to beginning of vector fail. Invalid argument.");
    ZH_ERROR_CHECK((*vector)->is_initialized == ZH_VECTOR_MAGIC, ESP_ERR_INVALID_STATE, NULL, "Adding item to beginning of vector fail. Vector not initialized.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Adding item to beginning of vector fail. Failed to acquire mutex.");
    if ((*vector)->capacity == (*vector)->size)
    {
        ZH_ERROR_CHECK(_resize(*vector, (*vector)->capacity + 1) == ESP_OK, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to beginning of vector fail. Memory allocation fail or no free memory in the heap.");
    }
    for (uint16_t i = (*vector)->size; i > 0; --i)
    {
        (*vector)->items[i] = (*vector)->items[i - 1];
    }
    (*vector)->items[0] = heap_caps_calloc(1, (*vector)->unit, MALLOC_CAP_8BIT);
    ZH_ERROR_CHECK((*vector)->items[0] != NULL, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to beginning of vector fail. Memory allocation fail or no free memory in the heap.");
    memcpy((*vector)->items[0], item, (*vector)->unit);
    (*vector)->size++;
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Adding item to beginning of vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_push_back(zh_vector_t **vector, const void *item) // -V2008
{
    ZH_LOGI("Adding item to vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Adding item to vector fail. Invalid argument.");
    ZH_ERROR_CHECK((*vector)->is_initialized == ZH_VECTOR_MAGIC, ESP_ERR_INVALID_STATE, NULL, "Adding item to vector fail. Vector not initialized.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Adding item to vector fail. Failed to acquire mutex.");
    if ((*vector)->capacity == (*vector)->size)
    {
        ZH_ERROR_CHECK(_resize(*vector, (*vector)->capacity + 1) == ESP_OK, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to vector fail. Memory allocation fail or no free memory in the heap.");
    }
    uint16_t idx = (*vector)->size;
    (*vector)->items[idx] = heap_caps_calloc(1, (*vector)->unit, MALLOC_CAP_8BIT);
    ZH_ERROR_CHECK((*vector)->items[idx] != NULL, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Adding item to vector fail. Memory allocation fail or no free memory in the heap.");
    memcpy((*vector)->items[idx], item, (*vector)->unit);
    (*vector)->size++;
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Adding item to vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_change_item(zh_vector_t **vector, uint16_t index, const void *item) // -V2008
{
    ZH_LOGI("Changing item in vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Changing item in vector fail. Invalid argument.");
    ZH_ERROR_CHECK((*vector)->is_initialized == ZH_VECTOR_MAGIC, ESP_ERR_INVALID_STATE, NULL, "Changing item in vector fail. Vector not initialized.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Changing item in vector fail. Failed to acquire mutex.");
    ZH_ERROR_CHECK(index < (*vector)->size, ESP_ERR_INVALID_ARG, NULL, "Changing item in vector fail. Index does not exist.");
    memcpy((*vector)->items[index], item, (*vector)->unit);
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Changing item in vector success.");
    return ESP_OK;
}

void *zh_vector_get_item(zh_vector_t **vector, uint16_t index) // -V2008
{
    ZH_LOGI("Getting item from vector begin.");
    if (vector == NULL || *vector == NULL)
    {
        ZH_LOGE("Getting item from vector fail. Invalid argument.", ESP_ERR_INVALID_ARG);
        return NULL;
    }
    if ((*vector)->is_initialized != ZH_VECTOR_MAGIC)
    {
        ZH_LOGE("Getting item from vector fail. Vector not initialized.", ESP_ERR_INVALID_STATE);
        return NULL;
    }
    if (xSemaphoreTake((*vector)->mutex, portMAX_DELAY) != pdTRUE)
    {
        ZH_LOGE("Getting item from vector fail. Failed to acquire mutex.", ESP_FAIL);
        return NULL;
    }
    void *item = NULL;
    if (index >= (*vector)->size)
    {
        xSemaphoreGive((*vector)->mutex);
        ZH_LOGE("Getting item from vector fail. Index does not exist.", ESP_FAIL);
        return NULL;
    }
    item = (*vector)->items[index];
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Getting item from vector success.");
    return item;
}

esp_err_t zh_vector_delete_item(zh_vector_t **vector, uint16_t index) // -V2008
{
    ZH_LOGI("Deleting item in vector begin.");
    ZH_ERROR_CHECK(vector != NULL && *vector != NULL, ESP_ERR_INVALID_ARG, NULL, "Deleting item in vector fail. Invalid argument.");
    ZH_ERROR_CHECK((*vector)->is_initialized == ZH_VECTOR_MAGIC, ESP_ERR_INVALID_STATE, NULL, "Deleting item in vector fail. Vector not initialized.");
    ZH_ERROR_CHECK(xSemaphoreTake((*vector)->mutex, portMAX_DELAY) == pdTRUE, ESP_ERR_INVALID_STATE, NULL, "Deleting item in vector fail. Failed to acquire mutex.");
    ZH_ERROR_CHECK(index < (*vector)->size, ESP_ERR_INVALID_ARG, NULL, "Deleting item in vector fail. Index does not exist.");
    void *freed_item = (*vector)->items[index];
    for (uint16_t i = index; i < ((*vector)->size - 1); ++i)
    {
        (*vector)->items[i] = (*vector)->items[i + 1];
    }
    heap_caps_free(freed_item);
    (*vector)->items[--(*vector)->size] = NULL;
    if ((*vector)->capacity > (*vector)->size)
    {
        ZH_ERROR_CHECK(_resize(*vector, (*vector)->size) == ESP_OK, ESP_ERR_NO_MEM, xSemaphoreGive((*vector)->mutex), "Deleting item in vector fail. Memory allocation fail or no free memory in the heap.");
    }
    xSemaphoreGive((*vector)->mutex);
    ZH_LOGI("Deleting item in vector success.");
    return ESP_OK;
}

static esp_err_t _resize(zh_vector_t *vector, uint16_t capacity)
{
    if (capacity == 0)
    {
        heap_caps_free(vector->items);
        vector->items = NULL;
        vector->capacity = 0;
        return ESP_OK;
    }
    uint16_t old_capacity = vector->capacity;
    void *new_items = heap_caps_realloc(vector->items, sizeof(void *) * capacity, MALLOC_CAP_8BIT);
    ZH_ERROR_CHECK(new_items != NULL, ESP_ERR_NO_MEM, NULL, "Memory allocation fail or no free memory in the heap.");
    vector->items = new_items;
    if (capacity > old_capacity)
    {
        memset(vector->items + old_capacity, 0, sizeof(void *) * (capacity - old_capacity));
    }
    vector->capacity = capacity;
    return ESP_OK;
}
