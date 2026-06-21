/**
 * @file zh_vector.h
 */

#pragma once

#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Main structure of vector data.
     */
    typedef struct _zh_vector_t zh_vector_t;

    /**
     * @brief Initialize vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] unit Size of vector item.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_init(zh_vector_t **vector, uint16_t unit);

    /**
     * @brief Deinitialize vector. Free all allocated memory.
     *
     * @param[in] vector Pointer to main structure of vector data.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_free(zh_vector_t *vector);

    /**
     * @brief Get current vector size.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[out] vector Pointer to vector size.
     *
     * @return Vector size if success or ESP_FAIL otherwise.
     */
    esp_err_t zh_vector_get_size(zh_vector_t *vector, size_t *size);

    /**
     * @brief Add item at beginning of vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] item Pointer to item for add.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_push_front(zh_vector_t *vector, const void *item);

    /**
     * @brief Add item at end of vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] item Pointer to item for add.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_push_back(zh_vector_t *vector, const void *item);

    /**
     * @brief Change item by index.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] index Index of item for change.
     * @param[in] item Pointer to new data of item.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_change_item(zh_vector_t *vector, uint16_t index, const void *item);

    /**
     * @brief Get item by index.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] index Index of item for get.
     *
     * @return Pointer to item or NULL otherwise.
     */
    void *zh_vector_get_item(zh_vector_t *vector, uint16_t index);

    /**
     * @brief Delete item by index and shifts all elements in vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] index Index of item for delete.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_delete_item(zh_vector_t *vector, uint16_t index);

#ifdef __cplusplus
}
#endif