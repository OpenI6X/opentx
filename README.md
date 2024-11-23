![OpenI6X](https://circleci.com/gh/OpenI6X/opentx.svg?style=shield)
[![Release](https://img.shields.io/github/v/release/OpenI6X/opentx?include_prereleases)](https://github.com/OpenI6X/opentx/releases/latest)
[![GitHub all releases](https://img.shields.io/github/downloads/OpenI6X/opentx/total)](https://github.com/OpenI6X/opentx/releases)
[![Discord](https://img.shields.io/discord/973289741862727741.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/3vKfYNTVa2)

![Banner](https://github.com/OpenI6X/opentx/blob/master/doc/flysky/banner.png?raw=true)

## Custom OpenTX / EdgeTX for Flysky FS-i6X

Join our [Discord](https://discord.gg/3vKfYNTVa2), [RCGroups](https://www.rcgroups.com/forums/showthread.php?3916435-FlySky-I6X-port-of-OpenTX) or [Telegram](https://t.me/otx_flysky_i6x) to contribute, discuss or get help!<br> 

**Configuration manual:**
- [EdgeTX manual](https://manual.edgetx.org/bw-radios)

**OpenI6X documentation:**
- **[Wiki](https://github.com/OpenI6X/opentx/wiki) - Read before asking for help**<br>
  - [How to install, upgrade or restore firmware](https://github.com/OpenI6X/opentx/wiki/Flashing-&-Upgrading) <br>
  - [Developers guide, how to build](https://github.com/OpenI6X/opentx/wiki/Development) <br>
  - [Modifications](https://github.com/OpenI6X/opentx/wiki/Modifications)<br>
- [Features](#features)<br>
- [User Interface](#user-interface)<br>
- [Proper shutdown (I see square icon)](#shutdown)<br>
- [USB connection](#usb-connection)<br>
- [Powering by 2S Li-Po/Li-ion/18650](#powering-by-2s-li-poli-ion18650)<br>
- [Mode 1 and Mode 3 radios](#mode-1--mode-3-radios)<br>
- [Credits](#credits)<br>

## Features

Short comparison with original firmware:

| Feature                   | FlySky i6X | OpenTX i6X                   |
|---------------------------|------------|------------------------------|
| Channels                  | 6/10       | 16                           |
| Mixers                    | 3          | 32                           |
| Models                    | 20         | 16 / unlimited<sup>[1]</sup> |
| Protocols                 | AFHDS, AFHDS2A, PPM | AFHDS2A + 16Ch modes, PPM, CRSF (CRSFshot)  |
| Trainer                   | PPM        | SBUS, PPM                    |
| Timers                    | _          | ✓                            |
| Voice annoucements        | _          | ✓<sup>[2]</sup>              |
| Vario                     | _          | ✓                            |
| ExpressLRS ready          | _          | ✓ Configurator built-in (no need for LUA) |
| Adjustable screen brightness | _       | ✓<sup>[3]</sup>              |
| USB Modes                 | Joystick   | Joystick, Storage, Serial (Telemetry mirror, Debug) |
| AUX Serial port           | _          | ✓ SBUS Trainer, Telemetry mirror, Debug |
| FlySky FS-HZCZ03-ADJ Digital Gimbal | _   | ✓<sup>[4]</sup>    |
| Languages                 | EN, CN      | PL, EN, CZ, DE, ES, FI, FR, IT, NL, PT, SE |

<sub>[1] Unlimited by using USB mass storage mode eeprom backup/restore.</sub><br>
<sub>[2] By adding DFPlayer, see [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#dfplayer) in wiki.</sub><br>
<sub>[3] By wiring 2 pads, see [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#adjustable-backlight-level) in wiki.</sub><br>
<sub>[4] See [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#flysky-fs-hzcz03-adj-gimbal) in wiki.</sub>
  
## User Interface

| Button | Function                                                                                           |
| --- |----------------------------------------------------------------------------------------------------|
| **[UP]**     | Move up, scroll values. Long press on the main screen to go to Statistics screen.                                        |                              
| **[DOWN]**   | Move down, scroll values. Long press on the main screen to show Telemetry screens.                                       |                                  
| **[OK]**     | Accept, select option. Short press on main screen to show Popup menu. Long press on main screen to go to Models menu.    |
| **[CANCEL]** | Return, back, cancel. Long press in ExpressLRS menu to exit.                                                             |                      
| **[BIND]**   | Scroll pages right or left (long press), go right in a line. Long press on the main screen to go to Radio Settings menu. |

## Shutdown

The FlySky FS-i6X does not have a software-controlled shutdown button. If you see a ▫ (small square) icon in the top-right corner of the main screen, wait until it disappears, as it indicates that settings are still being saved. Alternatively, you can use the "Save All" option from the main screen's popup menu to:

- Speed up the saving process.
- Save timers (timers are not saved automatically).
- Save changes while USB is connected (settings are not saved automatically when USB is connected).

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
