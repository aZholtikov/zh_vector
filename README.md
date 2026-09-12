# ESP32 ESP-IDF component for vector (dynamic array)

## Features

1. Support for any data types through void pointers.
2. Dynamic capacity with automatic growth and shrinkage.
3. Maximum capacity of 65,535 elements (16-bit index).
4. Thread-safe implementation using FreeRTOS mutex.
5. Automatic memory management with ESP-IDF heap_caps functions.
6. Flexible insertion at front or back.
7. Duplicate removal and item search capabilities.

## Using

In an existing project, run the following command to install the component:

```bash
cd ../your_project/components
git clone https://github.com/aZholtikov/zh_vector
```

In the application, add the component:

```c
#include "zh_vector.h"
```

## Examples

### Basic Example: Integer Vector

```c
#include "zh_vector.h"

zh_vector_t *vector = NULL;

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    esp_err_t ret = zh_vector_init(&vector, sizeof(int));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
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
        printf("Vector size: %u\n", size);
    }
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
    int new_val = 100;
    zh_vector_change_item(&vector, 1, &new_val);
    zh_vector_delete_item(&vector, 0);
    zh_vector_free(&vector);
}
```

### Struct Example

```c
#include "zh_vector.h"

typedef struct
{
    int id;
    char name[32];
    float value;
} my_struct_t;

zh_vector_t *vector = NULL;

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    esp_err_t ret = zh_vector_init(&vector, sizeof(my_struct_t));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    my_struct_t item1 = {1, "Item 1", 1.5f};
    my_struct_t item2 = {2, "Item 2", 2.5f};
    zh_vector_push_front(&vector, &item1);
    zh_vector_push_back(&vector, &item2);
    my_struct_t item_value = {0};
    esp_err_t err = zh_vector_get_item(&vector, 0, &item_value);
    if (err == ESP_OK)
    {
        item_value.value = 10.5f;
        zh_vector_change_item(&vector, 0, &item_value);
    }
    zh_vector_free(&vector);
}
```

### String Example (Char Arrays)

```c
#include "zh_vector.h"

zh_vector_t *vector = NULL;

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    esp_err_t ret = zh_vector_init(&vector, 100);
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    char *str1 = malloc(100);
    char *str2 = malloc(100);
    if (str1 && str2)
    {
        strcpy(str1, "Hello");
        strcpy(str2, "World");
        zh_vector_push_back(&vector, str1);
        zh_vector_push_back(&vector, str2);
        free(str1);
        free(str2);
    }
    uint16_t size = 0;
    ret = zh_vector_get_size(&vector, &size);
    if (ret == ESP_OK)
    {
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
    zh_vector_remove_duplicates(&vector);
    zh_vector_free(&vector);
}
```

### Remove Duplicates Example

```c
#include "zh_vector.h"

zh_vector_t *vector = NULL;

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    esp_err_t ret = zh_vector_init(&vector, sizeof(int));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
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
        printf("Vector size before removing duplicates: %u\n", size);
    }
    zh_vector_remove_duplicates(&vector);
    ret = zh_vector_get_size(&vector, &size);
    if (ret == ESP_OK)
    {
        printf("Vector size after removing duplicates: %u\n", size);
    }
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
    zh_vector_free(&vector);
}
```

---

### Find Item Example

```c
#include "zh_vector.h"

zh_vector_t *vector = NULL;

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    esp_err_t ret = zh_vector_init(&vector, sizeof(int));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    int val1 = 10;
    int val2 = 20;
    int val3 = 30;
    zh_vector_push_back(&vector, &val1);
    zh_vector_push_back(&vector, &val2);
    zh_vector_push_back(&vector, &val3);
    int search_item = 20;
    int32_t found_index = -1;
    ret = zh_vector_find_item(&vector, &search_item, &found_index);
    if (ret == ESP_OK)
    {
        printf("Element %d found at index %d\n", search_item, (int)found_index);
    }
    else
    {
        printf("Element %d not found (index=%d)\n", search_item, (int)found_index);
    }
    search_item = 100;
    found_index = -1;
    ret = zh_vector_find_item(&vector, &search_item, &found_index);
    if (ret == ESP_OK)
    {
        printf("Element %d found at index %d\n", search_item, (int)found_index);
    }
    else
    {
        printf("Element %d not found (index=%d)\n", search_item, (int)found_index);
    }
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

zh_vector_t *vector = NULL;

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_ERROR);
    esp_err_t ret = zh_vector_init(&vector, sizeof(my_struct_t));
    if (ret != ESP_OK)
    {
        printf("Vector initialization error\n");
        return;
    }
    my_struct_t item1 = {1, "Item 1", 1.5f};
    my_struct_t item2 = {2, "Item 2", 2.5f};
    my_struct_t item3 = {3, "Item 3", 3.5f};
    zh_vector_push_back(&vector, &item1);
    zh_vector_push_back(&vector, &item2);
    zh_vector_push_back(&vector, &item3);
    int search_id = 2;
    int32_t found_index = -1;
    my_struct_t sample = {0};
    ret = zh_vector_find_item_in_field(&vector, &sample, &sample.id, sizeof(sample.id), &search_id, 0, &found_index);
    if (ret == ESP_OK)
    {
        printf("Struct with id=%d found at index %d\n", search_id, (int)found_index);
        my_struct_t found_item = {0};
        ret = zh_vector_get_item(&vector, (uint16_t)found_index, &found_item);
        if (ret == ESP_OK)
        {
            printf("Found item: id=%d, name=%s, value=%.1f\n", found_item.id, found_item.name, found_item.value);
        }
    }
    else
    {
        printf("Struct with id=%d not found (index=%d)\n", search_id, (int)found_index);
    }
    search_id = 100;
    found_index = -1;
    ret = zh_vector_find_item_in_field(&vector, &sample, &sample.id, sizeof(sample.id), &search_id, 0, &found_index);
    if (ret == ESP_OK)
    {
        printf("Struct with id=%d found at index %d\n", search_id, (int)found_index);
    }
    else
    {
        printf("Struct with id=%d not found (index=%d)\n", search_id, (int)found_index);
    }
    zh_vector_free(&vector);
}
```
