#include "zh_vector.h"

static void s_zh_vector_resize(zh_vector_t *vector, uint16_t capacity);

esp_err_t zh_vector_init(zh_vector_t *vector, uint16_t unit)
{
    vector->capacity = 0;
    vector->size = 0;
    vector->unit = unit;
    vector->items = calloc(vector->capacity, sizeof(void *));
    return ESP_OK;
}

esp_err_t zh_vector_free(zh_vector_t *vector)
{
    for (uint16_t i = 0; i < zh_vector_get_size(vector); ++i)
    {
        free(zh_vector_get_item(vector, i));
    }
    free(vector->items);
    return ESP_OK;
}

uint16_t zh_vector_get_size(zh_vector_t *vector)
{
    return vector->size;
}

static void s_zh_vector_resize(zh_vector_t *vector, uint16_t capacity)
{
    void **items = realloc(vector->items, sizeof(void *) * capacity);
    vector->items = items;
    vector->capacity = capacity;
}

esp_err_t zh_vector_push_back(zh_vector_t *vector, void *item)
{
    if (vector->capacity == vector->size)
    {
        s_zh_vector_resize(vector, vector->capacity + 1);
    }
    void *temp = calloc(1, vector->unit);
    memcpy(temp, item, vector->unit);
    vector->items[vector->size++] = temp;
    return ESP_OK;
}

esp_err_t zh_vector_change_item(zh_vector_t *vector, uint16_t index, void *item)
{
    if (index < vector->size)
    {
        void *temp = zh_vector_get_item(vector, index);
        memcpy(temp, item, vector->unit);
        return ESP_OK;
    }
    return ESP_FAIL;
}

void *zh_vector_get_item(zh_vector_t *vector, uint16_t index)
{
    void *item = NULL;
    if (index < vector->size)
    {
        item = vector->items[index];
    }
    return item;
}

esp_err_t zh_vector_delete_item(zh_vector_t *vector, uint16_t index)
{
    if (index >= vector->size)
    {
        return ESP_FAIL;
    }
    free(zh_vector_get_item(vector, index));
    vector->items[index] = NULL;
    for (uint8_t i = index; i < (vector->size - 1); ++i)
    {
        vector->items[i] = vector->items[i + 1];
        vector->items[i + 1] = NULL;
    }
    --vector->size;
    s_zh_vector_resize(vector, vector->capacity - 1);
    return ESP_OK;
}