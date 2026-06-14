# ESP32 ESP-IDF component for vector (dynamic array)

## Wiki

[EN](WIKI_EN.md) | [RU](WIKI_RU.md)

## Tested on

1. [ESP32 ESP-IDF v6.0.0](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32/index.html)

## SAST Tools

[PVS-Studio](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

## Features

1. Support of any data types.
2. The maximum size of the veсtor is 65535 elements.

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
