# OpenI6X

![OpenI6X](https://circleci.com/gh/OpenI6X/opentx.svg?style=shield)
[![Release](https://img.shields.io/github/v/release/OpenI6X/opentx?include_prereleases)](https://github.com/OpenI6X/opentx/releases)
[![GitHub all releases](https://img.shields.io/github/downloads/OpenI6X/opentx/total)](https://github.com/OpenI6X/opentx/releases)

**OpenTX** port for the venerable Flysky FS-i6X RC radio transmitter.


You can find instructions and videos contributed by fellow early adopters in the [RCGroups thread](https://www.rcgroups.com/forums/showthread.php?3916435-FlySky-I6X-port-of-OpenTX), [Telegram group](https://t.me/otx_flysky_i6x) and [Rakish Rc](https://www.youtube.com/c/RakishRc) youtube channel. 

New contributions are highly welcome! See [developers guide](https://github.com/OpenI6X/opentx/wiki/Contribute).

### Nightly builds
If you want to have all the latest features, fixes (and bugs) you can download latest build here:
1. Go to: https://github.com/OpenI6X/opentx.
2. Click on <span style="color:green;font-weight:700;font-size:20px">✓</span> icon next to latest commit id.
3. Click on "Details" next to build of your choice.
4. Click on "ARTIFACTS" tab.
5. Download .bin file.

## Installation

Go to the [wiki](https://github.com/OpenI6X/opentx/wiki) for detailed steps.

## Features 

* Protocols:
  * AFHDS2A with SBUS, IBUS and extended SBUS16, IBUS16 - 16 channels modes
  * PPM in/out
  * CRSF with ExpressLRS and Crossfire modules, CRSFshot enabled
    * MEGA Bauds up to 1.8M
    * ExpressLRS V2 configuration (ELRSV2.lua port)
* 16 channels
* Telemetry, up to 26 sensors
* Audio tones, alarms and vario sound custom implementation
* DFU bootloader - Start by pushing trims to the center, like regular OpenTX one
* USB Joystick & Storage modes (To enable on standard cable: General Settings > USB Detect: Once)

## Navigation

| Key | Function                                                                           |
| --- |------------------------------------------------------------------------------------|
| UP     | Up. Hold on main screen for stats.                                                 |                              
| DOWN   | Down. Hold on main screen for telemetry                                            |                                  
| OK     | Confirm. Hold on main screen for model menu.                                       |
| CANCEL | Exit.                                                                              |                      
| BIND   | Scroll pages right or left (long press), go right in a line. Hold on main screen for general menu. |
| LEFT POT | Change edited value.                                                               |


## Shutdown

Do not switch off you device when you see small square icon in top right corner of main screen - it indicates that there are settings not yet stored. Wait until it disappears or use graceful shutdown method.

FlySky FS-i6X don't have graceful shutdown button like other OpenTX devices, but you can trigger it by holding CANCEL button until screen turns off, then use switch.

When to use this method:
* With USB connected - when USB is connected then settings are not stored with standard delay.
* To save timers.

## Mode 1

With Mode 1 radio you may experience inverted gimbal movement and swapped gimbals on main screen. To fix this swap gimbal connectors (red-white one with black-white one).

## Hardware connections (optional)

![hw](https://github.com/OpenI6X/opentx/raw/master/doc/flysky/openi6x_hardware.jpeg?raw=true)

## Contributors

* ExpressLRS V2 config, USB support, sound support, bootloader, backporting, bugfixing - Janek ([ajjjjjjjj](https://github.com/ajjjjjjjj)) ongoing collaboration.
* All the RF code was taken from the great KotelloRC's [erfly6: Er9X for i6 and i6x](https://bitbucket.org/KotelloRC/erfly6/src/master/).
* ADC code taken from [OpenGround](https://github.com/fishpepper/OpenGround).
* This work is based on Jakub's ([qba667](https://github.com/qba667)) work and is forked from his repo.
* All the contributors of [OpenTX](https://github.com/opentx/opentx/). 
