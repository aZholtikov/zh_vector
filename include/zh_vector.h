#pragma once

#include "stdio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct zh_vector_t
    {
        void **items;
        uint8_t capacity;
        uint8_t size;
    } zh_vector_t;

    /**
     * @brief      Initialize vector.
     *
     * @param[in]  vector  Pointer to structure of vector.
     *
     * @return
     *              - ESP_OK always
     */
    esp_err_t zh_vector_init(zh_vector_t *vector);

    /**
     * @brief      Deinitialize vector. Free all allocated memory.
     *
     * @param[in]  vector  Pointer to structure of vector.
     *
     * @return
     *              - ESP_OK always
     */
    esp_err_t zh_vector_free(zh_vector_t *vector);

    /**
     * @brief      Get current vector size.
     *
     * @param[in]  vector  Pointer to structure of vector.
     *
     * @return
     *              - Vector size
     */
    uint8_t zh_vector_get_size(zh_vector_t *vector);

    /**
     * @brief      Add item at end of vector. If sufficient memory is not available then it will resize the memory.
     *
     * @param[in]  vector  Pointer to structure of vector.
     * @param[in]  item    Pointer to item for add.
     *
     * @return
     *              - ESP_OK always
     */
    esp_err_t zh_vector_push_back(zh_vector_t *vector, void *item);

    /**
     * @brief      Change item by index.
     *
     * @param[in]  vector  Pointer to structure of vector.
     * @param[in]  index   Index of item for change.
     * @param[in]  item    Pointer to new data of item.
     *
     * @return
     *              - ESP_OK if change success
     *              - ESP_FAIL if index does not exist
     */
    esp_err_t zh_vector_change_item(zh_vector_t *vector, uint8_t index, void *item);

    /**
     * @brief      Get item by index.
     *
     * @param[in]  vector  Pointer to structure of vector.
     * @param[in]  index   Index of item for get.
     *
     * @return
     *              - Pointer to item
     *              - NULL if index does not exist
     */
    void *zh_vector_get_item(zh_vector_t *vector, uint8_t index);

    /**
     * @brief      Delete item by index and shifts all elements in vector.
     *
     * @param[in]  vector  Pointer to structure of vector.
     * @param[in]  index   Index of item for delete.
     *
     * @return
     *              - ESP_OK if delete success
     *              - ESP_FAIL if index does not exist
     */
    esp_err_t zh_vector_delete_item(zh_vector_t *vector, uint8_t index);

#ifdef __cplusplus
}
#endif