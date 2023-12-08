# ESP32 ESP-IDF and ESP8266 RTOS SDK component for vector (dynamic array)

The size of vector is limited by 65535 items. Support of any data types.

## Using

In an existing project, run the following command to install the component:

```text
cd ../your_project/components
git clone http://git.zh.com.ru/alexey.zholtikov/zh_vector.git
```

In the application, add the component:

```c
#include "zh_vector.h"
```

## Example

Create, add, read, modify and delete items:

```c
#include "stdio.h"
#include "zh_vector.h"

void app_main(void)
{
    zh_vector_t vector;
    zh_vector_init(&vector);
    printf("Initial vector size is: %d\n", zh_vector_get_size(&vector));
    zh_vector_push_back(&vector, "Item 1");
    zh_vector_push_back(&vector, "Item 2");
    zh_vector_push_back(&vector, "Item 3");
    zh_vector_push_back(&vector, "Item 4");
    zh_vector_push_back(&vector, "Item 5");
    printf("Add 5 items. New vector size is: %d\n", zh_vector_get_size(&vector));
    for (uint8_t i = 0; i < zh_vector_get_size(&vector); ++i)
    {
        printf("Item position %d is: %s\n", i, (char *)zh_vector_get_item(&vector, i));
    }
    zh_vector_change_item(&vector, 3, "Item 6");
    printf("Change item on 3 position.\n");
    for (uint8_t i = 0; i < zh_vector_get_size(&vector); ++i)
    {
        printf("Item position %d is: %s\n", i, (char *)zh_vector_get_item(&vector, i));
    }
    zh_vector_delete_item(&vector, 2);
    printf("Delete item on 2 position. New vector size is: %d\n", zh_vector_get_size(&vector));
    for (uint8_t i = 0; i < zh_vector_get_size(&vector); ++i)
    {
        printf("Item position %d is: %s\n", i, (char *)zh_vector_get_item(&vector, i));
    }
    zh_vector_free(&vector);
}
```