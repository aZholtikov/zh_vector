/**
 * @file zh_vector.h
 *
 * @brief Thread-safe dynamic array (vector) implementation for ESP-IDF.
 *
 * This vector stores elements by pointer (i.e., it holds `void*` to heap-allocated copies of items).
 * The vector is protected by a FreeRTOS mutex, making it safe for use in multi-threaded environments.
 *
 * @note The vector allocates memory for each item on push operations.
 * @note The vector does **not** own the lifetime of the data pointed to by the user — it copies the data.
 * @warning The `item` parameter of zh_vector_get_item must point to a buffer of at least `unit` bytes.
 *          The function copies the element *by value* — no pointers are returned.
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
     * @brief Opaque vector type.
     */
    typedef struct _zh_vector_t zh_vector_t;

    /**
     * @brief Initializes a vector.
     *
     * Allocates memory for the vector structure, internal item pointer array and a FreeRTOS mutex.
     *
     * @note Safe to call with `*vector == NULL` or with `*vector` pointing to uninitialized memory.
     * @warning Do **not** call this function twice on the same variable without an intervening `zh_vector_free()` — it will return `ESP_ERR_INVALID_STATE`.
     *          To reuse a vector, call `zh_vector_free(&v);` then `zh_vector_init(&v, ...);`.
     *
     * @param[out] vector Double pointer to vector structure (`zh_vector_t **`). On success, `*vector` points to newly allocated structure.
     * @param[in] unit Size (in bytes) of each item. Must be > 0.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_NO_MEM if memory allocation fails (vector struct or mutex).
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `unit == 0`.
     * @return ESP_ERR_INVALID_STATE if `*vector != NULL` (already initialized).
     */
    esp_err_t zh_vector_init(zh_vector_t **vector, uint16_t unit);

    /**
     * @brief Deinitializes and frees the entire vector.
     *
     * Frees all item copies, the internal pointer array, and the mutex. Sets `*vector` to `NULL` before returning.
     *
     * @warning After this call, all previously obtained item pointers become invalid.
     * @note Safe to call with `vector == NULL` or `*vector == NULL` (returns `ESP_OK`).
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`. Set to `NULL` on exit.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `*vector == NULL` (not initialized).
     * @return ESP_ERR_INVALID_STATE if the internal mutex cannot be acquired (rare, indicates a system error).
     */
    esp_err_t zh_vector_free(zh_vector_t **vector);

    /**
     * @brief Gets the current number of items in the vector.
     *
     * @note Thread-safe: internally locks/unlocks the mutex.
     *
     * @param[in] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[out] size Pointer to store the number of items. Must not be `NULL`.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `size == NULL` or `*vector == NULL` (not initialized).
     * @return ESP_ERR_INVALID_STATE if the internal mutex cannot be acquired (rare, indicates a system error).
     */
    esp_err_t zh_vector_get_size(zh_vector_t **vector, uint16_t *size);

    /**
     * @brief Adds a copy of the item to the beginning of the vector.
     *
     * A copy of `item` is allocated on the heap and stored in the vector.
     * The vector owns the lifetime of this copy (freed on `zh_vector_free()` or `zh_vector_delete_item()`).
     * All existing items are shifted one position to the right.
     * This may involve reallocation of the internal pointer array via `heap_caps_realloc`.
     *
     * @note Memory for the item and potential reallocation is done via `heap_caps_calloc`/`heap_caps_realloc` (`MALLOC_CAP_8BIT`).
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] item Pointer to the data to copy into the new item. Must not be `NULL`.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `item == NULL` or `*vector == NULL` (not initialized).
     * @return ESP_ERR_NO_MEM if memory allocation fails.
     * @return ESP_ERR_INVALID_STATE if the internal mutex cannot be acquired (rare, indicates a system error).
     */
    esp_err_t zh_vector_push_front(zh_vector_t **vector, const void *item);

    /**
     * @brief Adds a copy of the item to the end of the vector.
     *
     * A copy of `item` is allocated on the heap and stored in the vector.
     * The vector owns the lifetime of this copy (freed on `zh_vector_free()` or `zh_vector_delete_item()`).
     * This may involve reallocation of the internal pointer array via `heap_caps_realloc`.
     *
     * @note Memory for the item and potential reallocation is done via `heap_caps_calloc`/`heap_caps_realloc` (`MALLOC_CAP_8BIT`).
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] item Pointer to the data to copy into the new item. Must not be `NULL`.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `item == NULL` or `*vector == NULL` (not initialized).
     * @return ESP_ERR_NO_MEM if memory allocation fails.
     * @return ESP_ERR_INVALID_STATE if the internal mutex cannot be acquired (rare, indicates a system error).
     */
    esp_err_t zh_vector_push_back(zh_vector_t **vector, const void *item);

    /**
     * @brief Replaces the item at the specified index with a copy of the provided data.
     *
     * @warning Modifies the existing item *in-place* — no new memory allocation occurs.
     *          Only the data is overwritten (via `memcpy`). The item size must match the vector’s `unit`.
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] index Index of the item to replace (must be < `(*vector)->size`).
     * @param[in] item Pointer to new data to copy into the item. Must not be `NULL`.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL`, `item == NULL`, `index >= (*vector)->size` or `*vector == NULL` (not initialized).
     * @return ESP_ERR_INVALID_STATE if the internal mutex cannot be acquired (rare, indicates a system error).
     */
    esp_err_t zh_vector_change_item(zh_vector_t **vector, uint16_t index, const void *item);

    /**
     * @brief Retrieves the item at the specified index by copying it into the user-provided buffer.
     *
     * @note Thread-safe: internally locks/unlocks the mutex.
     * @note This function **copies** the element — no pointers are returned or stored.
     *
     * @param[in] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] index Index of the item to retrieve (0-based).
     * @param[out] item Pointer to a buffer of at least `unit` bytes, where the item will be copied.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL`, `item == NULL`, `index >= (*vector)->size` or `*vector == NULL` (not initialized).
     * @return ESP_ERR_INVALID_STATE if the internal mutex cannot be acquired (rare, indicates a system error).
     */
    esp_err_t zh_vector_get_item(zh_vector_t **vector, uint16_t index, void *item);

    /**
     * @brief Deletes the item at the specified index and shifts subsequent items one position left.
     *
     * The deleted item’s memory is freed.
     * If the size drops below half of the capacity (and size > 0), the capacity is reduced via `_resize(size)`.
     * If the size reaches 0, capacity is set to 0 and the pointer array is freed.
     *
     * @note Thread-safe: internally locks/unlocks the mutex.
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] index Index of the item to delete (must be < `(*vector)->size`).
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL`, `index >= (*vector)->size` or `*vector == NULL` (not initialized).
     * @return ESP_ERR_NO_MEM if memory allocation fails.
     * @return ESP_ERR_INVALID_STATE if the internal mutex cannot be acquired (rare, indicates a system error).
     */
    esp_err_t zh_vector_delete_item(zh_vector_t **vector, uint16_t index);

#ifdef __cplusplus
}
#endif