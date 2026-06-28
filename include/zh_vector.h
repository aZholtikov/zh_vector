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
 * @warning The pointers returned by zh_vector_get_item are valid only while the internal mutex is held.
 *          Do **not** store or use them after calling other vector functions or releasing the mutex.
 */

#pragma once

#include <string.h>
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
     * Allocates memory for the vector structure, internal item pointer array, and a FreeRTOS mutex.
     *
     * If `*vector` is non-NULL on entry:
     * - If the vector is already initialized (`is_initialized == ZH_VECTOR_MAGIC`), returns `ESP_ERR_INVALID_STATE`.
     * - Otherwise (e.g., uninitialized memory or stale state), safely releases stale resources
     *   (only the mutex and the internal items pointer array — **not the individual item data**, which is not owned by the vector)
     *   and reinitializes.
     *
     * @note Uses `ZH_VECTOR_MAGIC` magic value to distinguish initialized vs uninitialized state.
     * @note Safe to call with `*vector == NULL`, or with `*vector` pointing to uninitialized memory.
     * @warning Do not call this function twice on the same vector without an intervening `zh_vector_free()` —
     *          it will return `ESP_ERR_INVALID_STATE`. To reuse a vector, call `zh_vector_free(&v);`
     *          then `zh_vector_init(&v, ...);`.
     *
     * @param[out] vector Double pointer to vector structure (`zh_vector_t **`). On success, `*vector` points to newly allocated structure.
     * @param[in] unit Size (in bytes) of each item. Must be > 0.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_NO_MEM if memory allocation fails (vector struct or mutex).
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `unit == 0`.
     * @return ESP_ERR_INVALID_STATE if `*vector != NULL` and `(*vector)->is_initialized == ZH_VECTOR_MAGIC`.
     */
    esp_err_t zh_vector_init(zh_vector_t **vector, uint16_t unit);

    /**
     * @brief Deinitializes and frees the entire vector.
     *
     * Frees all item copies, the internal pointer array, and the mutex.
     * Sets `*vector` to `NULL` before returning.
     *
     * @warning After this call, all previously obtained item pointers become invalid.
     * @note Safe to call with `vector == NULL` or `*vector == NULL` (returns `ESP_OK`).
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`. Set to `NULL` on exit.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL`.
     */
    esp_err_t zh_vector_free(zh_vector_t **vector);

    /**
     * @brief Gets the current number of items in the vector.
     *
     * @param[in] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[out] size Pointer to store the number of items. Must not be `NULL`.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `size == NULL`.
     * @return ESP_ERR_INVALID_STATE if the vector is not initialized.
     */
    esp_err_t zh_vector_get_size(zh_vector_t **vector, size_t *size);

    /**
     * @brief Adds a copy of the item to the beginning of the vector.
     *
     * A copy of `item` is allocated on the heap and stored in the vector.
     * The vector owns the lifetime of this copy (freed on `zh_vector_free()` or `zh_vector_delete_item()`).
     * All existing items are shifted one position to the right.
     * This may involve reallocation of the internal pointer array via `heap_caps_realloc`.
     * @note Memory for the item and potential reallocation is done via `heap_caps_realloc` (`MALLOC_CAP_INTERNAL` or `MALLOC_CAP_DEFAULT`).
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] item Pointer to the data to copy into the new item. Must not be `NULL`.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `item == NULL`.
     * @return ESP_ERR_NO_MEM if memory allocation fails.
     * @return ESP_ERR_INVALID_STATE if the vector is not initialized.
     */
    esp_err_t zh_vector_push_front(zh_vector_t **vector, const void *item);

    /**
     * @brief Adds a copy of the item to the end of the vector.
     *
     * A copy of `item` is allocated on the heap and stored in the vector.
     * The vector owns the lifetime of this copy (freed on `zh_vector_free()` or `zh_vector_delete_item()`).
     * This may involve reallocation of the internal pointer array via `heap_caps_realloc`.
     * @note Memory for the item and potential reallocation is done via `heap_caps_realloc` (`MALLOC_CAP_INTERNAL` or `MALLOC_CAP_DEFAULT`).
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] item Pointer to the data to copy into the new item. Must not be `NULL`.
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `item == NULL`.
     * @return ESP_ERR_NO_MEM if memory allocation fails.
     * @return ESP_ERR_INVALID_STATE if the vector is not initialized.
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
     * @return ESP_ERR_INVALID_ARG if `vector == NULL`, `item == NULL`, or `index >= (*vector)->size`.
     * @return ESP_ERR_INVALID_STATE if the vector is not initialized.
     */
    esp_err_t zh_vector_change_item(zh_vector_t **vector, uint16_t index, const void *item);

    /**
     * @brief Retrieves a pointer to the item at the specified index.
     *
     * @warning The returned pointer is **only valid while the vector’s mutex is held** — i.e., before
     *          calling any other vector function or allowing task switches (in case other functions are called).
     *          Do **not** store or use this pointer outside the critical section.
     *
     * @param[in] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] index Index of the item to retrieve (0-based).
     *
     * @return Pointer to the item, or `NULL` if:
     *         - `vector == NULL`, or
     *         - vector is not initialized, or
     *         - `index >= (*vector)->size`, or
     *         - internal mutex could not be acquired.
     * @note `NULL` may also be returned if `(*vector)->size == 0` (no items to retrieve).
     */
    void *zh_vector_get_item(zh_vector_t **vector, uint16_t index);

    /**
     * @brief Deletes the item at the specified index and shifts subsequent items one position left.
     *
     * The deleted item’s memory is freed.
     *
     * @param[in,out] vector Double pointer to vector structure (`zh_vector_t **`). Must not be `NULL`.
     * @param[in] index Index of the item to delete (must be < `(*vector)->size`).
     *
     * @return ESP_OK on success.
     * @return ESP_ERR_INVALID_ARG if `vector == NULL` or `index >= (*vector)->size`.
     * @return ESP_ERR_NO_MEM if reallocation fails (rare).
     * @return ESP_ERR_INVALID_STATE if the vector is not initialized.
     */
    esp_err_t zh_vector_delete_item(zh_vector_t **vector, uint16_t index);

#ifdef __cplusplus
}
#endif