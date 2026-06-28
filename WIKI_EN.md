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

1. **Type Agnostic**: Supports any data type (integers, floats, structs, custom types, etc.)
2. **Automatic Memory Management**: Automatic memory allocation and deallocation
3. **Dynamic Resizing**: Vector capacity grows and shrinks as needed
4. **Maximum Capacity**: Up to 65,535 elements (16-bit index limit)
5. **ESP-IDF Optimized**: Uses heap_caps functions for memory allocation with memory caps
6. **Error Handling**: Comprehensive error checking with detailed logging
7. **Thread-Safe**: Thread-safe (uses FreeRTOS mutex)
8. **Minimal Overhead**: Low memory and CPU overhead

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

**Internal fields:**

- `void **items` — Array of pointers to vector elements
- `uint16_t capacity` — Maximum capacity of the vector
- `uint16_t size` — Number of items in the vector
- `uint16_t unit` — Vector item size (in bytes)
- `uint32_t is_initialized` — Vector initialization status flag
- `SemaphoreHandle_t mutex` — FreeRTOS mutex for thread safety

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
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer)

**Note:** All dynamically allocated element memory is also freed. The vector pointer is set to NULL after deinitialization.

---

### zh_vector_get_size()

Gets the current number of elements in the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `size` - Pointer to variable to store the size. Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or size pointer)
- `ESP_ERR_INVALID_STATE` - Vector not initialized

---

### zh_vector_push_front()

Adds an element to the beginning of the vector.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `item` - Pointer to the element to add. Must not be NULL.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or item pointer)
- `ESP_ERR_INVALID_STATE` - Vector not initialized
- `ESP_ERR_NO_MEM` - Memory allocation failed

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
- `ESP_ERR_INVALID_STATE` - Vector not initialized
- `ESP_ERR_NO_MEM` - Memory allocation failed

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
- `ESP_ERR_INVALID_STATE` - Vector not initialized
- `ESP_FAIL` - Index out of bounds

---

### zh_vector_get_item()

Gets an element at a specific index.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `index` - Index of the element to get (0-based).

**Returns:**

- `void *` - Pointer to the element (success)
- `NULL` - Error (NULL vector pointer, not initialized, or invalid index)

**Note:** Returns a pointer to the internal data. Do not free this pointer. The returned pointer is valid only while the internal mutex is held.

---

### zh_vector_delete_item()

Deletes an element at a specific index and shifts all subsequent elements.

**Parameters:**

- `vector` - Pointer to pointer to vector structure (`zh_vector_t **`). Must not be NULL.
- `index` - Index of the element to delete (0-based). Must be < vector size.

**Returns:**

- `ESP_OK` - Success
- `ESP_ERR_INVALID_ARG` - Invalid argument (NULL vector pointer or invalid index)
- `ESP_ERR_INVALID_STATE` - Vector not initialized
- `ESP_FAIL` - Index out of bounds

**Note:** All elements after the deleted index are shifted left by one position. The deleted item's memory is freed.

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
    if (ret != ESP_OK) {
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
    
    size_t size;
    ret = zh_vector_get_size(&vector, &size);
    if (ret == ESP_OK) {
        printf("Vector size: %zu\n", size);
    }
    
    // Access elements
    for (int i = 0; i < size; i++) {
        int *item = (int *)zh_vector_get_item(&vector, i);
        if (item != NULL) {
            printf("Element %d: %d\n", i, *item);
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

### Struct Example

```c
#include "zh_vector.h"

typedef struct {
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
    if (ret != ESP_OK) {
        printf("Vector initialization error\n");
        return;
    }
    
    // Add struct elements
    my_struct_t item1 = {1, "Item 1", 1.5f};
    my_struct_t item2 = {2, "Item 2", 2.5f};
    zh_vector_push_front(&vector, &item1);
    zh_vector_push_back(&vector, &item2);
    
    // Access and modify
    my_struct_t *ptr = (my_struct_t *)zh_vector_get_item(&vector, 0);
    if (ptr != NULL) {
        ptr->value = 10.5f;
    }
    
    // Cleanup
    zh_vector_free(&vector);
}
```

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
    if (ret != ESP_OK) {
        printf("Vector initialization error\n");
        return;
    }
    
    // Add strings
    strcpy(buffer, "Hello");
    zh_vector_push_front(&vector, &buffer);
    strcpy(buffer, "World");
    zh_vector_push_back(&vector, &buffer);
    
    size_t size;
    ret = zh_vector_get_size(&vector, &size);
    if (ret == ESP_OK) {
        // Print all strings
        for (int i = 0; i < size; i++) {
            char *str = (char *)zh_vector_get_item(&vector, i);
            if (str != NULL) {
                printf("String %d: %s\n", i, str);
            }
        }
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
| `ESP_ERR_INVALID_STATE` | Vector not initialized or already initialized |
| `ESP_ERR_NO_MEM` | Memory allocation failed (out of memory) |
| `ESP_FAIL` | General failure (e.g., index out of bounds) |

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

*Generated for zh_vector v3.0.0*
