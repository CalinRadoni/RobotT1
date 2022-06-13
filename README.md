# Robot T1

## Introduction

## Partitions

## Log level

In `platformio.ini` set `CORE_DEBUG_LEVEL` to a value according to `ARDUHAL_LOG_LEVEL` definitions from [esp32-hal-log.h](https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-log.h):

```c
#define ARDUHAL_LOG_LEVEL_NONE       (0)
#define ARDUHAL_LOG_LEVEL_ERROR      (1)
#define ARDUHAL_LOG_LEVEL_WARN       (2)
#define ARDUHAL_LOG_LEVEL_INFO       (3)
#define ARDUHAL_LOG_LEVEL_DEBUG      (4)
#define ARDUHAL_LOG_LEVEL_VERBOSE    (5)
```
