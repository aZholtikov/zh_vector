/**
 * @file zh_vector.h
 *
 * @brief Dynamic vector implementation with memory management and thread safety.
 *
 * This module provides a generic dynamic vector data structure with automatic
 * memory management, element insertion/deletion at any position, duplicate
 * removal, and item search capabilities. The implementation is thread-safe
 * using FreeRTOS mutexes for all public operations.
 *
 * Key features:
 * - Dynamic capacity with automatic growth and shrinkage
 * - Thread-safe operations via FreeRTOS mutex
 * - Support for arbitrary data types through void pointers
 * - Flexible insertion at front or back
 * - Duplicate removal and item search functionality
 * - Memory allocation with ESP32 heap capabilities
 *
 * @note All public functions require a valid vector pointer obtained through zh_vector_init().
 * @note All operations are protected by internal mutex for thread safety.
 */

#pragma once

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
     * @brief Forward declaration of the vector structure.
     *
     * Opaque handle representing a dynamic vector with internal state
     * including items storage, capacity, size, element size, and mutex.
     */
    typedef struct _zh_vector_t zh_vector_t;

    /**
     * @brief Initialize a new vector with specified element size.
     *
     * Allocates memory for the vector structure and creates an internal mutex.
     * The vector starts with zero capacity and will grow dynamically on first use.
     *
     * @param[out] vector Pointer to receive the initialized vector handle (must be NULL)
     * @param[in] unit Size of each element in bytes (must be > 0)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if vector is NULL or unit is zero
     * @return ESP_ERR_INVALID_STATE if vector is already initialized
     * @return ESP_ERR_NO_MEM if memory allocation fails
     */
    esp_err_t zh_vector_init(zh_vector_t **vector, uint16_t unit);

    /**
     * @brief Free all resources associated with the vector.
     *
     * Frees all allocated elements, the items array, the mutex, and the vector
     * structure itself. The vector pointer is set to NULL after deletion.
     *
     * @param[in,out] vector Pointer to the vector handle to be freed (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if vector or *vector is NULL
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     */
    esp_err_t zh_vector_free(zh_vector_t **vector);

    /**
     * @brief Get the current number of elements in the vector.
     *
     * @param[in] vector Pointer to the vector (must not be NULL)
     * @param[out] size Pointer to receive the current size (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if any parameter is NULL
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     */
    esp_err_t zh_vector_get_size(zh_vector_t **vector, uint16_t *size);

    /**
     * @brief Get the current capacity of the vector.
     *
     * @param[in] vector Pointer to the vector (must not be NULL)
     * @param[out] capacity Pointer to receive the current capacity (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if any parameter is NULL
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     */
    esp_err_t zh_vector_get_capacity(zh_vector_t **vector, uint16_t *capacity);

    /**
     * @brief Insert an element at the beginning of the vector.
     *
     * Shifts all existing elements one position forward and inserts
     * a copy of the provided item at index 0. The vector capacity
     * will be increased if necessary.
     *
     * @param[in,out] vector Pointer to the vector (must not be NULL)
     * @param[in] item Pointer to the item to insert (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if any parameter is NULL
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     * @return ESP_ERR_NO_MEM if vector is full or element allocation fails
     */
    esp_err_t zh_vector_push_front(zh_vector_t **vector, const void *item);

    /**
     * @brief Append an element to the end of the vector.
     *
     * Creates a copy of the provided item and adds it to the end of the vector.
     * The vector capacity will be increased if necessary.
     *
     * @param[in,out] vector Pointer to the vector (must not be NULL)
     * @param[in] item Pointer to the item to append (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if any parameter is NULL
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     * @return ESP_ERR_NO_MEM if vector is full or element allocation fails
     */
    esp_err_t zh_vector_push_back(zh_vector_t **vector, const void *item);

    /**
     * @brief Replace an element at the specified index.
     *
     * Copies the provided item into the existing element at the given index.
     * The element must already exist and be non-NULL.
     *
     * @param[in,out] vector Pointer to the vector (must not be NULL)
     * @param[in] index Index of the element to replace (must be < size)
     * @param[in] item Pointer to the new item data (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if any parameter is NULL or index is out of bounds
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails or item is NULL
     */
    esp_err_t zh_vector_change_item(zh_vector_t **vector, uint16_t index, const void *item);

    /**
     * @brief Retrieve a copy of the element at the specified index.
     *
     * @param[in] vector Pointer to the vector (must not be NULL)
     * @param[in] index Index of the element to retrieve (must be < size)
     * @param[out] item Pointer to buffer where the element will be copied (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if any parameter is NULL or index is out of bounds
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails or item is NULL
     */
    esp_err_t zh_vector_get_item(zh_vector_t **vector, uint16_t index, void *item);

    /**
     * @brief Delete the element at the specified index.
     *
     * Frees the element's memory, shifts remaining elements, and updates size.
     * Capacity may be reduced if it exceeds twice the current size.
     *
     * @param[in,out] vector Pointer to the vector (must not be NULL)
     * @param[in] index Index of the element to delete (must be < size)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if vector or *vector is NULL or index is out of bounds
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     */
    esp_err_t zh_vector_delete_item(zh_vector_t **vector, uint16_t index);

    /**
     * @brief Delete the last element of the vector.
     *
     * Frees the last element's memory and updates size.
     * Capacity may be reduced if it exceeds twice the current size.
     *
     * @param[in,out] vector Pointer to the vector (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if vector or *vector is NULL or vector is empty
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     */
    esp_err_t zh_vector_delete_back(zh_vector_t **vector);

    /**
     * @brief Delete the first element of the vector.
     *
     * Frees the first element's memory, shifts remaining elements, and updates size.
     * Capacity may be reduced if it exceeds twice the current size.
     *
     * @param[in,out] vector Pointer to the vector (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if vector or *vector is NULL or vector is empty
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     */
    esp_err_t zh_vector_delete_front(zh_vector_t **vector);

    /**
     * @brief Remove duplicate elements from the vector.
     *
     * Compares elements using memcmp and removes subsequent duplicates.
     * Elements are compared based on their raw binary content.
     *
     * @param[in,out] vector Pointer to the vector (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if vector or *vector is NULL
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     */
    esp_err_t zh_vector_remove_duplicates(zh_vector_t **vector);

    /**
     * @brief Find the index of the first matching element.
     *
     * Searches for an element whose binary content matches the provided item
     * using memcmp. Returns the index of the first match or -1 if not found.
     *
     * @param[in] vector Pointer to the vector (must not be NULL)
     * @param[in] item Pointer to the item to search for (must not be NULL)
     * @param[out] index Pointer to receive the found index (-1 if not found) (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if any parameter is NULL
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     * @return ESP_ERR_NOT_FOUND if item is not found in the vector
     */
    esp_err_t zh_vector_find_item(zh_vector_t **vector, const void *item, int32_t *index);

    /**
     * @brief Find an element by comparing a specific field within structures.
     *
     * Searches for an element where a specified field (identified by offset
     * and size) matches the provided value. Comparison starts from the
     * specified index.
     *
     * @param[in] vector Pointer to the vector containing structures (must not be NULL)
     * @param[in] sample_struct Pointer to a sample structure (used for field offset calculation) (must not be NULL)
     * @param[in] item Pointer to the field within the structure (e.g., `&sample.id`). Used to calculate the field offset (must not be NULL)
     * @param[in] size Size of the field in bytes (must be > 0)
     * @param[in] value Pointer to the value to match against the field (must not be NULL)
     * @param[in] start Index to begin searching from (must be < size)
     * @param[out] index Pointer to receive the found index (-1 if not found) (must not be NULL)
     *
     * @return ESP_OK on success
     * @return ESP_ERR_INVALID_ARG if any parameter is invalid
     * @return ESP_ERR_INVALID_STATE if mutex acquisition fails
     * @return ESP_ERR_NOT_FOUND if no matching element is found
     */
    esp_err_t zh_vector_find_item_in_field(zh_vector_t **vector, const void *sample_struct, const void *item, size_t size, const void *value, uint16_t start, int32_t *index);

#ifdef __cplusplus
}
#endif