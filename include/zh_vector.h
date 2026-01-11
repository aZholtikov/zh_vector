/**
 * @file zh_vector.h
 */

#pragma once

#include "string.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Main structure of vector data.
     */
    typedef struct
    {
        void **items;        /*!< Array of pointers of vector items. */
        uint16_t capacity;   /*!< Maximum capacity of the vector. @note Used to control the size of allocated memory for array of pointers of vector items. Usually equal to the current number of items in the vector. Automatically changes when items are added or deleted. */
        uint16_t size;       /*!< Number of items in the vector. */
        uint16_t unit;       /*!< Vector item size. */
        bool is_initialized; /*!< Vector initialization status flag. */
    } zh_vector_t;

    /**
     * @brief Initialize vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] unit Size of vector item.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_init(zh_vector_t *vector, uint16_t unit);

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
     *
     * @return Vector size if success or ESP_FAIL otherwise.
     */
    esp_err_t zh_vector_get_size(zh_vector_t *vector);

    /**
     * @brief Add item at end of vector.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] item Pointer to item for add.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_push_back(zh_vector_t *vector, void *item);

    /**
     * @brief Change item by index.
     *
     * @param[in] vector Pointer to main structure of vector data.
     * @param[in] index Index of item for change.
     * @param[in] item Pointer to new data of item.
     *
     * @return ESP_OK if success or an error code otherwise.
     */
    esp_err_t zh_vector_change_item(zh_vector_t *vector, uint16_t index, void *item);

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