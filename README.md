[![Release](https://img.shields.io/github/v/release/OpenI6X/opentx?include_prereleases)](https://github.com/OpenI6X/opentx/releases/latest)
[![GitHub all releases](https://img.shields.io/github/downloads/OpenI6X/opentx/total)](https://github.com/OpenI6X/opentx/releases)
[![GitHub license](https://img.shields.io/github/license/OpenI6X/opentx)](https://github.com/openi6x/opentx/blob/master/LICENSE)
![OpenI6X](https://circleci.com/gh/OpenI6X/opentx.svg?style=shield)
[![Gitpod ready-to-code](https://img.shields.io/badge/Gitpod-ready--to--code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/openi6x/opentx/tree/master)
[![Discord](https://img.shields.io/discord/973289741862727741.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/3vKfYNTVa2)

![Banner](https://github.com/OpenI6X/opentx/blob/master/doc/flysky/banner.png?raw=true)

# Custom OpenTX / EdgeTX for Flysky FS-i6X

Join our [Discord](https://discord.gg/3vKfYNTVa2), [RCGroups](https://www.rcgroups.com/forums/showthread.php?3916435-FlySky-I6X-port-of-OpenTX) or [Telegram](https://t.me/otx_flysky_i6x) to contribute, discuss or get help.<br> 

## Instructions

- [FlySky i6X user interface](https://github.com/OpenI6X/opentx/wiki/Manual) and other differences to official EdgeTX/OpenTX devices.<br>
- [EdgeTX manual](https://manual.edgetx.org/bw-radios)
- [How to install, upgrade or restore firmware](https://github.com/OpenI6X/opentx/wiki/Flashing-&-Upgrading)<br>
- [Developers guide, how to build](https://github.com/OpenI6X/opentx/wiki/Development)<br>
- [Modifications](https://github.com/OpenI6X/opentx/wiki/Modifications)<br>

## Comparison

| Feature                   | FlySky i6X | OpenI6X                      |
|---------------------------|------------|------------------------------|
| Channels                  | 6/10       | 16                           |
| Mixers                    | 3          | 32                           |
| Models                    | 20         | 20 / unlimited<sup>[1]</sup> |
| Protocols                 | AFHDS, AFHDS2A, PPM | AFHDS2A + 16Ch modes, PPM, CRSF |
| Trainer                   | PPM        | PPM, SBUS                    |
| Timers                    | _          | ✓                            |
| Voice annoucements        | _          | ✓<sup>[2]</sup>              |
| Variometer                | _          | ✓                            |
| ExpressLRS ready          | _          | ✓ Configurator built-in (no need for LUA) |
| Adjustable screen brightness | _       | ✓<sup>[3]</sup>              |
| USB Modes                 | Joystick   | Joystick, Storage, Serial (Telemetry mirror, Debug) |
| AUX Serial port           | _          | ✓ SBUS Trainer, Telemetry mirror, Debug |
| FlySky FS-HZCZ03-ADJ Gimbal support | _   | ✓<sup>[4]</sup>    |
| Languages                 | EN, CN      | PL, EN, CZ, DE, ES, FI, FR, IT, NL, PT, SE |

<sub>[1] Unlimited by using USB mass storage mode eeprom backup/restore.</sub><br>
<sub>[2] By adding DFPlayer, see [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#dfplayer) in wiki.</sub><br>
<sub>[3] By wiring 2 pads, see [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#adjustable-backlight-level) in wiki.</sub><br>
<sub>[4] See [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#flysky-fs-hzcz03-adj-gimbal) in wiki.</sub>
  
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
