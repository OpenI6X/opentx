![OpenI6X](https://circleci.com/gh/OpenI6X/opentx.svg?style=shield)
[![Release](https://img.shields.io/github/v/release/OpenI6X/opentx?include_prereleases)](https://github.com/OpenI6X/opentx/releases/latest)
[![GitHub all releases](https://img.shields.io/github/downloads/OpenI6X/opentx/total)](https://github.com/OpenI6X/opentx/releases)
[![Discord](https://img.shields.io/discord/973289741862727741.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/3vKfYNTVa2)

![Banner](https://github.com/OpenI6X/opentx/blob/master/doc/flysky/banner.png?raw=true)

## Welcome to Openi6X!

Custom OpenTX/EdgeTX for Flysky FS-i6X.

Join our [Discord](https://discord.gg/3vKfYNTVa2) or [Telegram](https://t.me/otx_flysky_i6x) to contribute, discuss or get help!<br> 

- **[Wiki](https://github.com/OpenI6X/opentx/wiki) - Read before asking for help**<br>
  - [How to install and upgrade firmware](https://github.com/OpenI6X/opentx/wiki/Flashing-&-Upgrading) <br>
  - [Developers guide, how to build](https://github.com/OpenI6X/opentx/wiki/Development) <br>
  - [Modifications](https://github.com/OpenI6X/opentx/wiki/Modifications)<br>
- [Features](#features)<br>
- [Navigation](#navigation)<br>
- [Proper shutdown (I see square icon)](#shutdown)<br>
- [USB connection](#usb-connection)<br>
- [Powering by 2S Li-Po/Li-ion/18650](#powering-by-2s-li-poli-ion18650)<br>
- [Mode 1 and Mode 3 radios](#mode-1--mode-3-radios)<br>
- [Credits](#credits)<br>

For help with general OpenI6X configuration see manuals:
- [EdgeTX manual](https://manual.edgetx.org/bw-radios)
- [OpenTX manual](https://doc.open-tx.org/manual-for-opentx-2-2)

You can find some instructions in the [RCGroups](https://www.rcgroups.com/forums/showthread.php?3916435-FlySky-I6X-port-of-OpenTX) and [Rakish Rc](https://www.youtube.com/playlist?list=PLfzAEbvn4Bgr3ndNrwp87UimoKVhXkzBa) youtube tutorials. Some of that can be outdated. Read Wiki for up to date instructions.

## Features

Comparison with original firmware:

| Feature                   | FlySky i6X | OpenTX i6X                   |
|---------------------------|------------|------------------------------|
| Channels                  | 6/10       | 16                           |
| Mixers                    | 3          | 32                           |
| Models                    | 20         | 16 / unlimited<sup>[1]</sup> |
| Protocols                 | AFHDS, AFHDS2A, PPM | AFHDS2A + 16Ch modes, PPM, CRSF (CRSFshot)  |
| Trainer                   | PPM        | SBUS, PPM                    |
| Logical switches          | _          | ✓                            |
| Global variables          | _          | ✓                            |
| Timers                    | _          | ✓                            |
| Voice annoucements        | _          | ✓<sup>[2]</sup>              |
| Vario                     | _          | ✓                            |
| Use trims as buttons      | _          | ✓                            |
| ExpressLRS ready          | _          | ✓ Configurator built-in (no need for LUA) |
| Adjustable screen brightness | _       | ✓<sup>[3]</sup>              |
| USB Modes                 | Joystick   | Joystick, Storage, Serial (Telemetry mirror) |
| AUX Serial port           | _          | SBUS Trainer, Telemetry mirror, Debug |
| FlySky FS-HZCZ03-ADJ Digital Gimbal | _   | ✓<sup>[4]</sup>    |
| Languages                 | EN,CN      | PL,EN,CZ,DE,ES,FI,FR,IT,NL,PT,SE |

<sub>[1] Unlimited by using USB mass storage mode eeprom backup/restore.</sub><br>
<sub>[2] By adding DFPlayer, see [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#dfplayer) in wiki.</sub><br>
<sub>[3] By wiring 2 pads, see [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#adjustable-backlight-level) in wiki.</sub><br>
<sub>[4] See [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#flysky-fs-hzcz03-adj-gimbal) in wiki.</sub>
  
## Navigation

| Key | Function                                                                                           |
| --- |----------------------------------------------------------------------------------------------------|
| UP     | Up. Scroll values. Hold on main screen for stats.                                                  |                              
| DOWN   | Down. Scroll values. Hold on main screen for telemetry.                                            |                                  
| OK     | Confirm. Hold on main screen for model menu. Short press on main screen for popup menu.            |
| CANCEL | Exit/Back/Cancel.                                                                                  |                      
| BIND   | Scroll pages right or left (long press), go right in a line. Hold on main screen for general menu. |

DFU bootloader mode - Start by pushing horizontal trims to the center + power on a the same time.

## Shutdown

FlySky FS-i6X don't have a software controlled shutdown button. Do not switch off radio when you see `▫` (small square) icon in top right corner of main screen. It indicates that settings are not yet saved. Wait until it disappears or use "Save all" option from main screen popup menu.

When to use "Save all" option:
* When you don't want to wait until square icon disappear before shutdown.
* With USB connected - when USB is connected then settings are not stored with standard delay.
* To save timers.

## USB connection

FlySky FS-i6X don't have a USB VBUS making it impossible to detect USB connection. To connect press OK on main screen and select "USB Connect" (In version 1.8.0 or earlier it's in: Radio Setup -> "USB Detect").

## Powering by 2S Li-Po/Li-ion/18650

FlySky i6X is officially rated for up to 6V. Running anything above will damage your radio.

## Mode 1 & Mode 3 radios

With Mode 1 & Mode 3 radios you may experience inverted gimbal movement and swapped gimbals on main screen. To fix this swap gimbal connectors (red-white one with black-white one).

## Credits

* Janek ([ajjjjjjjj](https://github.com/ajjjjjjjj)), continues Kuba's and Mariano's work, added sound, USB, ExpressLRS V2/V3 configuration, telemetry mirror, SBUS trainer, new/fixed drivers, ports, bugfixes.
* Mariano ([marianomd](https://github.com/marianomd)), continued Kuba's work and made it up to useable condition! Added gimbals, buttons, AFHDS2A, PPM, CRSF.
* Kuba ([qba667](https://github.com/qba667)), started this work and made this project possible, it is forked from his repo.
* Wilhelm ([wimalopaan](https://github.com/wimalopaan)) added 16 channels SBUS16 / IBUS16 modes.
* Rafael ([rafolg](https://github.com/rafolg)), ported FlySky Hall Gimbal support from EdgeTX.
* Tom ([tmcadam](https://github.com/tmcadam)) fixed AFHDS2A PWM mode selection.
* The internal RF code was taken from the great KotelloRC's [erfly6: Er9X for i6 and i6x](https://bitbucket.org/KotelloRC/erfly6/src/master/).
* Some of the internal RF fixes are a result of analysing [pascallanger's](https://github.com/pascallanger) [DIY-Multiprotocol-TX-Module](https://github.com/pascallanger/DIY-Multiprotocol-TX-Module).
* ExpressLRS configurator is based on elrsV2/V3.lua from [ExpressLRS](https://github.com/ExpressLRS/ExpressLRS).
* Some of the ports are from [EdgeTX](https://github.com/EdgeTX/edgetx/).
* ADC code taken from [OpenGround](https://github.com/fishpepper/OpenGround).
* All the contributors of [OpenTX](https://github.com/opentx/opentx/). 
