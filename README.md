# ESP32 ESP-IDF component for vector (dynamic array)

## Wiki

[EN](WIKI_EN.md) | [RU](WIKI_RU.md)

## Tested on

1. [ESP32 ESP-IDF v6.0.0](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32/index.html)

## SAST Tools

[PVS-Studio](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

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

```text
cd ../your_project/components
git clone https://github.com/aZholtikov/zh_vector
```

In the application, add the component:

```c
#include "zh_vector.h"
```

## Examples

See Wiki [EN](WIKI_EN.md#usage-examples) | [RU](WIKI_RU.md#примеры-использования)
