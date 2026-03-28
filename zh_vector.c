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

static esp_err_t _resize(zh_vector_t *vector, uint16_t capacity);

esp_err_t zh_vector_init(zh_vector_t *vector, uint16_t unit, ...)
{
    ZH_LOGI("Vector initialization begin.");
    ZH_ERROR_CHECK(vector != NULL && unit != 0, ESP_ERR_INVALID_ARG, NULL, "Vector initialization failed. Invalid argument.");
    ZH_ERROR_CHECK(vector->is_initialized == false, ESP_ERR_INVALID_STATE, NULL, "Vector initialization failed. Vector is already initialized.");
    vector->capacity = 0;
    vector->size = 0;
    vector->unit = unit;
    vector->is_initialized = true;
    ZH_LOGI("Vector initialization success.");
    return ESP_OK;
}

esp_err_t zh_vector_free(zh_vector_t *vector)
{
    ZH_LOGI("Vector deletion begin.");
    ZH_ERROR_CHECK(vector != NULL, ESP_ERR_INVALID_ARG, NULL, "Vector deletion failed. Invalid argument.");
    ZH_ERROR_CHECK(vector->is_initialized == true, ESP_ERR_INVALID_STATE, NULL, "Vector deletion fail. Vector not initialized.");
    for (uint16_t i = 0; i < vector->size; ++i)
    {
        heap_caps_free(vector->items[i]);
    }
    heap_caps_free(vector->items);
    vector->is_initialized = false;
    ZH_LOGI("Vector deletion success.");
    return ESP_OK;
}

esp_err_t zh_vector_get_size(zh_vector_t *vector)
{
    ZH_LOGI("Getting vector size begin.");
    ZH_ERROR_CHECK(vector != NULL, ESP_FAIL, NULL, "Getting vector size fail. Invalid argument.");
    ZH_ERROR_CHECK(vector->is_initialized == true, ESP_FAIL, NULL, "Getting vector size fail. Vector not initialized.");
    ZH_LOGI("Getting vector size success.");
    return vector->size;
}

esp_err_t zh_vector_push_back(zh_vector_t *vector, void *item) // -V2008
{
    ZH_LOGI("Adding item to vector begin.");
    ZH_ERROR_CHECK(vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Adding item to vector fail. Invalid argument.");
    ZH_ERROR_CHECK(vector->is_initialized == true, ESP_ERR_INVALID_STATE, NULL, "Adding item to vector fail. Vector not initialized.");
    if (vector->capacity == vector->size)
    {
        ZH_ERROR_CHECK(_resize(vector, vector->capacity + 1) == ESP_OK, ESP_ERR_NO_MEM, NULL, "Adding item to vector fail. Memory allocation fail or no free memory in the heap.");
    }
    vector->items[vector->size] = heap_caps_calloc(1, vector->unit, MALLOC_CAP_8BIT);
    ZH_ERROR_CHECK(vector->items[vector->size] != NULL, ESP_ERR_NO_MEM, NULL, "Adding item to vector fail. Memory allocation fail or no free memory in the heap.");
    memcpy(vector->items[vector->size++], item, vector->unit);
    ZH_LOGI("Adding item to vector success.");
    return ESP_OK;
}

esp_err_t zh_vector_change_item(zh_vector_t *vector, uint16_t index, void *item)
{
    ZH_LOGI("Changing item in vector begin.");
    ZH_ERROR_CHECK(vector != NULL && item != NULL, ESP_ERR_INVALID_ARG, NULL, "Changing item in vector fail. Invalid argument.");
    ZH_ERROR_CHECK(vector->is_initialized == true, ESP_ERR_INVALID_STATE, NULL, "Changing item in vector fail. Vector not initialized.");
    ZH_ERROR_CHECK(index < vector->size, ESP_FAIL, NULL, "Changing item in vector fail. Index does not exist.");
    memcpy(vector->items[index], item, vector->unit);
    ZH_LOGI("Changing item in vector success.");
    return ESP_OK;
}

void *zh_vector_get_item(zh_vector_t *vector, uint16_t index)
{
    ZH_LOGI("Getting item from vector begin.");
    if (vector == NULL)
    {
        ZH_LOGE("Getting item from vector fail. Invalid argument.", ESP_ERR_INVALID_ARG);
        return NULL;
    }
    if (vector->is_initialized == false)
    {
        ZH_LOGE("Getting item from vector fail. Vector not initialized.", ESP_ERR_INVALID_STATE);
        return NULL;
    }
    if (index < vector->size)
    {
        ZH_LOGI("Getting item from vector success.");
        return vector->items[index];
    }
    else
    {
        ZH_LOGE("Getting item from vector fail. Index does not exist.", ESP_FAIL);
        return NULL;
    }
}

esp_err_t zh_vector_delete_item(zh_vector_t *vector, uint16_t index) // -V2008
{
    ZH_LOGI("Deleting item in vector begin.");
    ZH_ERROR_CHECK(vector != NULL, ESP_ERR_INVALID_ARG, NULL, "Deleting item in vector fail. Invalid argument.");
    ZH_ERROR_CHECK(vector->is_initialized == true, ESP_ERR_INVALID_STATE, NULL, "Deleting item in vector fail. Vector not initialized.");
    ZH_ERROR_CHECK(index < vector->size, ESP_FAIL, NULL, "Deleting item in vector fail. Index does not exist.");
    heap_caps_free(vector->items[index]);
    for (uint8_t i = index; i < (vector->size - 1); ++i)
    {
        vector->items[i] = vector->items[i + 1];
        vector->items[i + 1] = NULL;
    }
    --vector->size;
    ZH_ERROR_CHECK(_resize(vector, vector->capacity - 1) == ESP_OK, ESP_ERR_NO_MEM, NULL, "Deleting item in vector fail. Memory allocation fail or no free memory in the heap.");
    ZH_LOGI("Deleting item in vector success.");
    return ESP_OK;
}

static esp_err_t _resize(zh_vector_t *vector, uint16_t capacity)
{
    if (capacity != 0)
    {
        vector->items = heap_caps_realloc(vector->items, sizeof(void *) * capacity, MALLOC_CAP_8BIT);
        ZH_ERROR_CHECK(vector->items != NULL, ESP_ERR_NO_MEM, NULL, "Memory allocation fail or no free memory in the heap.");
    }
    vector->capacity = capacity;
    return ESP_OK;
}