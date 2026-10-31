// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __BOARD_ESP32C3_DEVKITC_H__
#define __BOARD_ESP32C3_DEVKITC_H__

#define LIGHT_BUTTON_GPIO          9    /* This is the button that is used for toggling the output */
#define LIGHT_BUTTON_ACTIVE_LEVEL  0

/**
 * WS2812 / NeoPixel configuration.
 *
 * A WS2812 has one serial data input, unlike the former discrete RGB LED
 * which used three PWM GPIOs. All LEDs in the strip show the same colour
 * selected from the RainMaker application.
 */
#define WS2812_DATA_GPIO        3
#define WS2812_LED_COUNT        8

#endif /**< __BOARD_ESP32C3_DEVKITC_H__ */
