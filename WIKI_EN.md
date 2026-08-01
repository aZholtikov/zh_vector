# zh_vector - Vector (Dynamic Array) Component for ESP-IDF

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Technical Specifications](#technical-specifications)
- [Error Codes](#error-codes)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

`zh_vector` is a lightweight, efficient vector (dynamic array) component for ESP-IDF (Espressif IoT Development Framework). It provides a flexible container that can store elements of any data type with automatic memory management. The vector automatically resizes as items are added or removed, making it ideal for dynamic data management in embedded systems.

The component is designed specifically for ESP32 microcontrollers and uses ESP-IDF's heap management functions for optimal memory allocation with different memory caps (internal/external RAM).

---

## Features

- **Type Agnostic**: Supports any data type (integers, floats, structs, custom types, etc.)
- **Automatic Memory Management**: Automatic memory allocation and deallocation
- **Dynamic Resizing**: Vector capacity grows and shrinks as needed
- **Maximum Capacity**: Up to 65,535 elements (16-bit index limit)
- **ESP-IDF Optimized**: Uses heap_caps functions for memory allocation with memory caps
- **Error Handling**: Comprehensive error checking with detailed logging
- **Thread-Safe**: Thread-safe (uses FreeRTOS mutex)
- **Minimal Overhead**: Low memory and CPU overhead

---

## Installation

Navigate to your project's components directory:

```bash
cd ../your_project/components
```

Clone the repository:

```bash
git clone https://github.com/aZholtikov/zh_vector
```

In your application, include the header:

```c
#include "zh_vector.h"
```

The component will be automatically built with your project.

---

## API Reference

All functions in this library use double pointer (`zh_vector_t **`) for the vector parameter to allow for proper memory management and thread-safe operations.

### zh_vector_t Structure

The structure is declared as `typedef struct _zh_vector_t zh_vector_t;` and encapsulates internal implementation details.

**Fields (internal):**

| Field | Type | Description |
|-------|------|-------------|
| `items` | `void **` | Array of element pointers. Items[0..size-1] are valid. Allocated via heap_caps_calloc, reallocated via heap_caps_realloc. |
| `capacity` | `uint16_t` | Current allocated capacity (number of slots). Grows on insertion - may exceed size after deletions. |
| `size` | `uint16_t` | Current number of elements (0 ≤ size ≤ capacity). |
| `unit` | `uint16_t` | Size (in bytes) of a single element. Set once in zh_vector_init() and immutable. |
| `mutex` | `SemaphoreHandle_t` | FreeRTOS mutex. Created in zh_vector_init(), deleted in zh_vector_free. Automatically locked/unlocked in public functions. |

---

### zh_vector_init()

Initializes the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). If the vector pointer is NULL, memory will be allocated.
- `unit` - Size of each element in bytes

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or zero unit size)
- `ESP_ERR_INVALID_STATE` - Vector already initialized
- `ESP_ERR_NO_MEM` - Memory allocation failed

**Example:**

```c
zh_vector_t *vector = NULL;
esp_err_t ret = zh_vector_init(&vector, sizeof(int)); // For integers
if (ret != ESP_OK) {
    // Handle error
}
// Don't forget to free: zh_vector_free(&vector);
```

---

### zh_vector_free()

Deinitializes the vector and frees all allocated memory. Sets the vector pointer to NULL.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or NULL vector)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** All dynamically allocated element memory is also freed. The vector pointer is set to NULL after deinitialization.

---

### zh_vector_get_capacity()

Gets the current allocated capacity of the vector (maximum number of elements without reallocation).

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `capacity` - Pointer to variable to store the capacity. Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or capacity pointer)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** Capacity may be larger than the current size (e.g., after deletions), and is reduced lazily when size < capacity/2 (or when size becomes 0).

---

### zh_vector_get_size()

Gets the current number of elements in the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `size` - Pointer to variable to store the size. Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or size pointer)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

---

### zh_vector_push_front()

Adds an element to the beginning of the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `item` - Pointer to the element to add. Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or item pointer)
- `ESP_ERR_NO_MEM` - Memory allocation failed
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** The function allocates memory for a copy of the item and copies the data. All existing elements are shifted right by one position.

---

### zh_vector_push_back()

Adds an element to the end of the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `item` - Pointer to the element to add. Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or item pointer)
- `ESP_ERR_NO_MEM` - Memory allocation failed
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** The function allocates memory for a copy of the item and copies the data.

---

### zh_vector_change_item()

Changes an element at a specific index.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `index` - Index of the element to change (0-based). Must be < vector size.
- `item` - Pointer to the new element data. Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer, item pointer, or invalid index)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex or item is NULL

---

### zh_vector_get_item()

Retrieves an element at a specific index by copying it into the user-provided buffer.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `index` - Index of the element to get (0-based).
- `item` - Pointer to a buffer of at least `unit` bytes where the element will be copied. Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer, NULL item pointer, or invalid index)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** This function copies the element data into the provided buffer. No pointers are returned.

---

### zh_vector_delete_item()

Deletes an element at a specific index and shifts all subsequent elements.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `index` - Index of the element to delete (0-based). Must be < vector size.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or invalid index)
- `ESP_ERR_NO_MEM` - Memory allocation failed (during reallocation)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** All elements after the deleted index are shifted left by one position. The deleted item's memory is freed.

---

### zh_vector_delete_back()

Removes the last element from the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer, NULL vector, or vector is empty)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** The deleted item's memory is freed. If the size drops below half of the capacity, the capacity is reduced (same behaviour as zh_vector_delete_item).

---

### zh_vector_delete_front()

Removes the first element from the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer, NULL vector, or vector is empty)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** The deleted item's memory is freed, and all subsequent elements are shifted left by one position. If the size drops below half of the capacity, the capacity is reduced (same behaviour as zh_vector_delete_item).

---

### zh_vector_find_item()

Finds the first occurrence of an element in the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `item` - Pointer to the element to find. Must not be NULL.
- `index` - Pointer to variable to store the found index. Will be set to -1 if element is not found. Must not be NULL.

**Returns:**

- `ESP_OK` - Success (element found)
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer, NULL item pointer, or NULL index pointer)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)
- `ESP_ERR_NOT_FOUND` - Element not found (index set to -1)

**Note:** Uses memcmp to compare elements. Returns the index of the first occurrence. The index variable is set to -1 if the element is not found.

---

### zh_vector_find_item_in_field()

Finds the first occurrence of a value in a specific field within structures stored in the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `sample_struct` - Pointer to a sample structure of the same type as stored in the vector. Must not be NULL.
- `item` - Pointer to the field within the structure (e.g., `&sample.id`). Used to calculate the field offset within the structure. Must not be NULL.
- `size` - Size (in bytes) of the field to compare. Must be > 0.
- `value` - Pointer to the value to search for. Must not be NULL.
- `start` - Starting index for the search (0-based). Must be < vector size.
- `index` - Pointer to variable to store the found index. Will be set to -1 if value is not found. Must not be NULL.

**Returns:**

- `ESP_OK` - Success (value found)
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL pointer, zero size, or invalid start index)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)
- `ESP_ERR_NOT_FOUND` - Value not found (index set to -1)

**Note:** This function calculates the offset of the field within the structure using `item` and `sample_struct` pointers (offset = `item - sample_struct`), then searches for the specified `value` in that field across all vector elements starting from the given index. Uses memcmp for comparison. Useful for searching in vectors of structures without copying the entire structure.

---

### zh_vector_remove_duplicates()

Removes duplicate elements from the vector, keeping only the first occurrence of each element.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or NULL vector)
- `ESP_ERR_INVALID_STATE` - Failed to acquire mutex (rare system error)

**Note:** Uses memcmp to compare elements. Only the first occurrence of each unique element is preserved. The function preserves the order of first occurrences.

---

## Usage Examples

### Basic Example: Integer Vector

```c
#include "zh_vector.h"

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    zh_vector_t *vector = NULL;
    // Initialize vector for integers
    esp_err_t ret = zh_vector_init(&vector, sizeof(int));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    // Add elements
    int val1 = 10;
    int val2 = 20;
    int val3 = 30;
    zh_vector_push_front(&vector, &val1);
    zh_vector_push_back(&vector, &val2);
    zh_vector_push_back(&vector, &val3);
    uint16_t size = 0;
    ret = zh_vector_get_size(&vector, &size);
    if (ret == ESP_OK)
    {
        printf("Vector size: %zu\n", size);
    }
    // Access elements
    for (int i = 0; i < size; i++)
    {
        int item_value = 0;
        esp_err_t err = zh_vector_get_item(&vector, (uint16_t)i, &item_value);
        if (err == ESP_OK)
        {
            printf("Element %d: %d\n", i, item_value);
        }
        else
        {
            printf("Error getting element %d: %s\n", i, esp_err_to_name(err));
        }
    }
    // Change element
    int new_val = 100;
    zh_vector_change_item(&vector, 1, &new_val);
    // Delete element
    zh_vector_delete_item(&vector, 0);
    // Cleanup
    zh_vector_free(&vector);
}
```

---

### Struct Example

```c
#include "zh_vector.h"

typedef struct
{
    int id;
    char name[32];
    float value;
} my_struct_t;

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    zh_vector_t *vector = NULL;
    // Initialize vector for structs
    esp_err_t ret = zh_vector_init(&vector, sizeof(my_struct_t));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    // Add struct elements
    my_struct_t item1 = {1, "Item 1", 1.5f};
    my_struct_t item2 = {2, "Item 2", 2.5f};
    zh_vector_push_front(&vector, &item1);
    zh_vector_push_back(&vector, &item2);
    // Access and modify
    my_struct_t item_value = {0};
    esp_err_t err = zh_vector_get_item(&vector, 0, &item_value);
    if (err == ESP_OK)
    {
        item_value.value = 10.5f;
        zh_vector_change_item(&vector, 0, &item_value);
    }
    // Cleanup
    zh_vector_free(&vector);
}
```

---

### String Example (Char Arrays)

```c
#include "zh_vector.h"

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    zh_vector_t *vector = NULL;
    char buffer[100] = {0};
    // Initialize vector for strings (100 char max)
    esp_err_t ret = zh_vector_init(&vector, sizeof(buffer));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    // Add strings
    strcpy(buffer, "Hello");
    zh_vector_push_front(&vector, &buffer);
    strcpy(buffer, "World");
    zh_vector_push_back(&vector, &buffer);
    uint16_t size = 0;
    ret = zh_vector_get_size(&vector, &size);
    if (ret == ESP_OK)
    {
        // Print all strings
        for (int i = 0; i < size; i++)
        {
            char str_value[100] = {0};
            esp_err_t err = zh_vector_get_item(&vector, (uint16_t)i, str_value);
            if (err == ESP_OK)
            {
                printf("String %d: %s\n", i, str_value);
            }
            else
            {
                printf("Error getting string %d: %s\n", i, esp_err_to_name(err));
            }
        }
    }
    // Remove duplicates (if any)
    zh_vector_remove_duplicates(&vector);
    // Cleanup
    zh_vector_free(&vector);
}
```

---

### Remove Duplicates Example

```c
#include "zh_vector.h"

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    zh_vector_t *vector = NULL;
    // Initialize vector for integers
    esp_err_t ret = zh_vector_init(&vector, sizeof(int));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    // Add elements with duplicates
    int val1 = 10;
    int val2 = 20;
    int val3 = 10;
    int val4 = 30;
    int val5 = 20;
    zh_vector_push_back(&vector, &val1);
    zh_vector_push_back(&vector, &val2);
    zh_vector_push_back(&vector, &val3);
    zh_vector_push_back(&vector, &val4);
    zh_vector_push_back(&vector, &val5);
    uint16_t size = 0;
    ret = zh_vector_get_size(&vector, &size);
    if (ret == ESP_OK)
    {
        printf("Vector size before removing duplicates: %zu\n", size);
    }
    // Remove duplicates
    zh_vector_remove_duplicates(&vector);
    ret = zh_vector_get_size(&vector, &size);
    if (ret == ESP_OK)
    {
        printf("Vector size after removing duplicates: %zu\n", size);
    }
    // Access remaining elements
    for (int i = 0; i < size; ++i)
    {
        int item_value = 0;
        esp_err_t err = zh_vector_get_item(&vector, (uint16_t)i, &item_value);
        if (err == ESP_OK)
        {
            printf("Element %d: %d\n", i, item_value);
        }
        else
        {
            printf("Error getting element %d: %s\n", i, esp_err_to_name(err));
        }
    }
    // Cleanup
    zh_vector_free(&vector);
}
```

---

### Find Item Example

```c
#include "zh_vector.h"

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    zh_vector_t *vector = NULL;
    // Initialize vector for integers
    esp_err_t ret = zh_vector_init(&vector, sizeof(int));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    // Add elements
    int val1 = 10;
    int val2 = 20;
    int val3 = 30;
    zh_vector_push_back(&vector, &val1);
    zh_vector_push_back(&vector, &val2);
    zh_vector_push_back(&vector, &val3);
    // Find element
    int search_item = 20;
    int32_t found_index = -1;
    ret = zh_vector_find_item(&vector, &search_item, &found_index);
    if (ret == ESP_OK)
    {
        printf("Element %d found at index %ld\n", search_item, found_index);
    }
    else
    {
        printf("Element %d not found (index=%ld)\n", search_item, found_index);
    }
    // Find non-existing element
    search_item = 100;
    found_index = -1;
    ret = zh_vector_find_item(&vector, &search_item, &found_index);
    if (ret == ESP_OK)
    {
        printf("Element %d found at index %ld\n", search_item, found_index);
    }
    else
    {
        printf("Element %d not found (index=%ld)\n", search_item, found_index);
    }
    // Cleanup
    zh_vector_free(&vector);
}
```

---

### Find Item in Field Example

```c
#include "zh_vector.h"

typedef struct
{
    int id;
    char name[32];
    float value;
} my_struct_t;

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    zh_vector_t *vector = NULL;
    // Initialize vector for structs
    esp_err_t ret = zh_vector_init(&vector, sizeof(my_struct_t));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    // Add struct elements
    my_struct_t item1 = {1, "Item 1", 1.5f};
    my_struct_t item2 = {2, "Item 2", 2.5f};
    my_struct_t item3 = {3, "Item 3", 3.5f};
    zh_vector_push_back(&vector, &item1);
    zh_vector_push_back(&vector, &item2);
    zh_vector_push_back(&vector, &item3);
    // Find struct by field value (searching by id field)
    int search_id = 2;
    int32_t found_index = -1;
    // Calculate offset of 'id' field within the structure
    my_struct_t sample = {0};
    ret = zh_vector_find_item_in_field(&vector, &sample, &sample.id, sizeof(sample.id), &search_id, 0, &found_index);
    if (ret == ESP_OK)
    {
        printf("Struct with id=%d found at index %ld\n", search_id, found_index);
        // Get the found struct
        my_struct_t found_item = {0};
        ret = zh_vector_get_item(&vector, (uint16_t)found_index, &found_item);
        if (ret == ESP_OK)
        {
            printf("Found item: id=%d, name=%s, value=%.1f\n", found_item.id, found_item.name, found_item.value);
        }
    }
    else
    {
        printf("Struct with id=%d not found (index=%ld)\n", search_id, found_index);
    }
    // Find non-existing id
    search_id = 100;
    found_index = -1;
    ret = zh_vector_find_item_in_field(&vector, &sample, &sample.id, sizeof(sample.id), &search_id, 0, &found_index);
    if (ret == ESP_OK)
    {
        printf("Struct with id=%d found at index %ld\n", search_id, found_index);
    }
    else
    {
        printf("Struct with id=%d not found (index=%ld)\n", search_id, found_index);
    }
    // Cleanup
    zh_vector_free(&vector);
}
```

---

## Technical Specifications

| Parameter | Value |
|-----------|-------|
| **Maximum Capacity** | 65,535 elements |
| **Index Type** | uint16_t (16-bit) |
| **Memory Management** | heap_caps_calloc, heap_caps_realloc, heap_caps_free |
| **Memory Caps** | MALLOC_CAP_8BIT |
| **Thread Safety** | Thread-safe (uses FreeRTOS mutex) |
| **ESP-IDF Version** | >= 5.0 |
| **Platform** | ESP32 series |
| **Language** | C (C99) |

---

## Error Codes

| Error Code | Description |
|------------|-------------|
| `ESP_OK` | Operation successful |
| `ESP_ERR_INVALID_ARG` | Invalid argument (NULL pointer or zero size) |
| `ESP_ERR_INVALID_STATE` | Failed to acquire mutex (rare system error) |
| `ESP_ERR_NO_MEM` | Memory allocation failed (out of memory) |
| `ESP_ERR_NOT_FOUND` | Element not found (for zh_vector_find_item) |

---

## Contributing

Contributions are welcome! To contribute:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

Please ensure your code follows the existing style and includes appropriate documentation.

---

## License

This project is licensed under the Apache License, Version 2.0 - see the [LICENSE](LICENSE) file for details.

### Apache License, Version 2.0

Copyright (c) 2026 Alexey Zholtikov

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

---

## Additional Notes

- **Memory Overhead**: Each element requires additional memory for the pointer in the internal pointer array
- **Performance**: O(1) for get/set by index, O(n) for insert/delete in the middle
- **Best Practices**:
  - Always initialize the vector before use
  - Free the vector when done to avoid memory leaks
  - Consider the maximum size limit (65,535 elements)
  - For string handling, consider using fixed-size buffers

---

*Generated for zh_vector v2.5.0*
