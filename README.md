# ESP32 ESP-IDF and ESP8266 RTOS SDK component for vector (dynamic array)

## Tested on

1. [ESP8266 RTOS_SDK v3.4](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/index.html#)
2. [ESP32 ESP-IDF v5.4](https://docs.espressif.com/projects/esp-idf/en/release-v5.4/esp32/index.html)

## Features

1. Support of any data types.
2. The maximum size of the veсtor is 65535 elements.

## Using

In an existing project, run the following command to install the component:

```text
cd ../your_project/components
git clone http://git.zh.com.ru/alexey.zholtikov/zh_vector
```

In the application, add the component:

```c
#include "zh_vector.h"
```

## Example

Create, add, read, modify and delete items:

```c
#include "zh_vector.h"

zh_vector_t vector = {0};

char example[10] = {0};

void app_main(void)
{
    esp_log_level_set("zh_vector", ESP_LOG_NONE); // For ESP8266 first enable "Component config -> Log output -> Enable log set level" via menuconfig.
    zh_vector_init(&vector, sizeof(example));
    printf("Initial vector size is: %d\n", zh_vector_get_size(&vector));
    strcpy(example, "Item 1");
    zh_vector_push_back(&vector, &example);
    strcpy(example, "Item 2");
    zh_vector_push_back(&vector, &example);
    strcpy(example, "Item 3");
    zh_vector_push_back(&vector, &example);
    strcpy(example, "Item 4");
    zh_vector_push_back(&vector, &example);
    strcpy(example, "Item 5");
    zh_vector_push_back(&vector, &example);
    printf("Add 5 items. New vector size is: %d\n", zh_vector_get_size(&vector));
    for (uint16_t i = 0; i < zh_vector_get_size(&vector); ++i)
    {
        printf("Item position %d is: %s\n", i, (char *)zh_vector_get_item(&vector, i));
    }
    strcpy(example, "Item 6");
    zh_vector_change_item(&vector, 3, &example);
    printf("Change item on 3 position.\n");
    for (uint16_t i = 0; i < zh_vector_get_size(&vector); ++i)
    {
        printf("Item position %d is: %s\n", i, (char *)zh_vector_get_item(&vector, i));
    }
    zh_vector_delete_item(&vector, 2);
    printf("Delete item on 2 position. New vector size is: %d\n", zh_vector_get_size(&vector));
    for (uint16_t i = 0; i < zh_vector_get_size(&vector); ++i)
    {
        printf("Item position %d is: %s\n", i, (char *)zh_vector_get_item(&vector, i));
    }
    zh_vector_free(&vector);
}
```
