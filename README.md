# OpenI6X: OpenTX for Flysky FS-i6X

![OpenI6X](https://circleci.com/gh/OpenI6X/opentx.svg?style=shield)
[![Release](https://img.shields.io/github/v/release/OpenI6X/opentx?include_prereleases)](https://github.com/OpenI6X/opentx/releases/latest)
[![GitHub all releases](https://img.shields.io/github/downloads/OpenI6X/opentx/total)](https://github.com/OpenI6X/opentx/releases)
[![Discord](https://img.shields.io/discord/973289741862727741.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/3vKfYNTVa2)

You can find instructions and videos contributed by fellow early adopters in the [RCGroups](https://www.rcgroups.com/forums/showthread.php?3916435-FlySky-I6X-port-of-OpenTX), [Telegram](https://t.me/otx_flysky_i6x), [Discord](https://discord.gg/3vKfYNTVa2) and [Rakish Rc](https://www.youtube.com/playlist?list=PLfzAEbvn4Bgr3ndNrwp87UimoKVhXkzBa) youtube tutorials. 

For new contributions see [developers guide](https://github.com/OpenI6X/opentx/wiki/Contribute).

## Table of Contents

[How to install](#installation)<br>
[Features](#features)<br>
[Navigation](#navigation)<br>
[Proper shutdown (I see square icon)](#shutdown)<br>
[USB connection](#usb-connection)<br>
[Mode 1 and Mode 3 radios](#mode-1--mode-3-radios)<br>
[All optional hardware connections](#all-optional-hardware-connections)<br>
[Adjustable backlight level mod](#adjustable-backlight-level-mod)<br>
[Contributors](#contributors)<br>

## Installation

Go to the [wiki](https://github.com/OpenI6X/opentx/wiki) for detailed steps.

## Features

Comparison with original firmware:

|                           | FlySky i6X | OpenTX i6X |
|---------------------------|------------|------------|
| Channels                  | 6/10       | 16         |
| Mixers                    | 3          | 32         |
| External module protocols | PPM        | PPM, CRSF  |
| Logical switches          | _          | ✓          |
| Global variables          | _          | ✓          |
| Timers                    | _          | ✓          |
| Nicer sounds              | _          | ✓          |
| Use trims as buttons      | _          | ✓          |
| ExpressLRS ready          | _          | ✓          |

* Protocols:
  * AFHDS2A with SBUS, IBUS and extended SBUS16, IBUS16 - 16 channels modes
  * PPM in/out
  * CRSF with ExpressLRS and Crossfire modules:
    * CRSFshot
    * MEGA Bauds up to 1.8M
    * ExpressLRS V3 configuration (ELRSV3.lua port)
* 16 channels
* Telemetry
* Adjustable backlight brightness level (requires [basic mod](#all-optional-hardware-connections) & backlight_mod build)
* Audio tones, alarms and vario sound custom implementation
* DFU bootloader - Start by pushing trims to the center, like regular OpenTX one
* USB Joystick & Storage modes
* AUX Serial port with modes:
  * CRSF Telemetry mirror
  * SBUS Trainer
  * Debug (on DEBUG builds)
  
## Navigation

| Key | Function                                                                                           |
| --- |----------------------------------------------------------------------------------------------------|
| UP     | Up. Scroll values. Hold on main screen for stats.                                                  |                              
| DOWN   | Down. Scroll values. Hold on main screen for telemetry.                                            |                                  
| OK     | Confirm. Hold on main screen for model menu. Short press on main screen for popup menu.            |
| CANCEL | Exit/Back/Cancel.                                                                                  |                      
| BIND   | Scroll pages right or left (long press), go right in a line. Hold on main screen for general menu. |

## Shutdown

FlySky FS-i6X don't have a software controlled shutdown button, therefore do not switch off you device when you see small square icon in top right corner of main screen - it indicates that there are settings not yet saved. Wait until it disappears or use "Save all" option from main screen popup menu.

When to use "Save all" option:
* When you don't want to wait until square icon disapper before shutdown.
* With USB connected - when USB is connected then settings are not stored with standard delay.
* To save timers.

## USB connection

FlySky FS-i6X don't have a USB VBUS making it impossible to detect USB connection. Without the modification you have to go to General settings and set USB Detect to "Once" everytime you're connecting USB.<br>
Auto connection detection can be added manually by wiring `PA15` to USB VBUS preferably with a resistor (I have used 1K).


## Mode 1 & Mode 3 radios

With Mode 1 & Mode 3 radios you may experience inverted gimbal movement and swapped gimbals on main screen. To fix this swap gimbal connectors (red-white one with black-white one).

## All optional hardware connections

| PCB Pad    | Function                                              |
|------------|-------------------------------------------------------|
| `TX2`      | S.Port (CRSF)                                         |
| `PA9`      | AUX Serial port TX                                    |
| `PA10`     | AUX Serial port RX                                    |
| `PA15`     | USB VBUS (USB connection detection)                   |
| `PC13`     | External module power control (it was `PC9` up to OpenI6X 1.5.0) |
| `PC9`+`BL` | Wiring those together allows for adjustable backlight |

![hw](https://github.com/OpenI6X/opentx/raw/master/doc/flysky/openi6x_hardware.jpeg?raw=true)

## Adjustable backlight level mod

Wire `PC9` and `BL` pads together.

![hw](https://github.com/OpenI6X/opentx/raw/master/doc/flysky/backlight_mod.jpg?raw=true)

## Contributors

* ExpressLRS V2/V3 config, USB support, sound support, bootloader, backporting, bugfixing - Janek ([ajjjjjjjj](https://github.com/ajjjjjjjj)) ongoing collaboration.
* All the RF code was taken from the great KotelloRC's [erfly6: Er9X for i6 and i6x](https://bitbucket.org/KotelloRC/erfly6/src/master/).
* ADC code taken from [OpenGround](https://github.com/fishpepper/OpenGround).
* This work is based on Jakub's ([qba667](https://github.com/qba667)) work and is forked from his repo.
* All the contributors of [OpenTX](https://github.com/opentx/opentx/) 
