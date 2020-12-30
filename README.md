# ESP Components library


Interfaces to components from [ESP-IDF framework](https://github.com/espressif/esp-idf) and [ESP8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK).

## How to use

### ESP8266 RTOS SDK

This library provides a front-end to the Espressif [ESP 8266 RTOS SDK](https://github.com/espressif/ESP8266_RTOS_SDK), and the [esp-idf-lib driver library](https://github.com/UncleRus/esp-idf-lib)

Both of the above are dependencies on this library.

The idea of this library is to provide a front-end to the driver functions. For most devices, it will create a FreeRTOS task to manage the device and provide updates.

For example, the sensors (DS18B20 and DHT11) will create a task which will call back at a configured frequency with a reading. The gpio library has helpers for debounced pushbutton and toggle switches, and also to create pulsed outputs. For details see the individual components and their examples (ToDo)

To use, clone this repository somewhere, e.g.:

```Shell
cd ~/myprojects/esp
git clone xxxxx
```


Add path to components in your project makefile, e.g:

```Makefile
PROJECT_NAME := my-esp-project

EXTRA_COMPONENT_DIRS := /home/user/myprojects/esp/esp-idf-lib/components \
                        /home/user/myprojects/esp/esp-sdb-lib/components

EXCLUDE_COMPONENTS := max7219 mcp23x17 led_strip

include $(IDF_PATH)/make/project.mk
```

## Components

| Component     | Description
|---------------|---------------------------------------------------
| **gpio**      | Switches, pushbuttons relays etc (with debouncing)
| **analog**    | Analog inputs
| **ds18b20**   | DS18B20 temperature sensor
| **dht**       | DHT11 temperature and humidity sensor
| **pwm**       | PWM driver
| **ir**        | NEC infrared receiver
