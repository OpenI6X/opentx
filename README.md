[![Release](https://img.shields.io/github/v/release/OpenI6X/opentx?include_prereleases)](https://github.com/OpenI6X/opentx/releases/latest)
[![GitHub all releases](https://img.shields.io/github/downloads/OpenI6X/opentx/total)](https://github.com/OpenI6X/opentx/releases)
[![GitHub license](https://img.shields.io/github/license/OpenI6X/opentx)](https://github.com/openi6x/opentx/blob/master/LICENSE)
![OpenI6X](https://circleci.com/gh/OpenI6X/opentx.svg?style=shield)
[![Discord](https://img.shields.io/discord/973289741862727741.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/3vKfYNTVa2)

![Banner](https://github.com/OpenI6X/opentx/blob/master/doc/flysky/banner.png?raw=true)

# OpenI6X

Goal of the project is to bring customized **EdgeTX / OpenTX** to FlySky I6X and other STM32F0 based radios.

Join our [Discord](https://discord.gg/3vKfYNTVa2), [RCGroups](https://www.rcgroups.com/forums/showthread.php?3916435-FlySky-I6X-port-of-OpenTX) or [Telegram](https://t.me/otx_flysky_i6x) to contribute, discuss or get help.

New contributors are very welcome.<br>
You can help by improving documentation, adding or suggesting features, fixing or reporting bugs.

## Instructions

- [User interface](https://github.com/OpenI6X/opentx/wiki/Manual) and differences to official EdgeTX / OpenTX radios.
- [EdgeTX manual](https://manual.edgetx.org/bw-radios), mostly relevant to OpenI6X.
- [How to install, upgrade or restore firmware](https://github.com/OpenI6X/opentx/wiki/Flashing-&-Upgrading)
- [Developers guide, how to build](https://github.com/OpenI6X/opentx/wiki/Development)
- [Modifications](https://github.com/OpenI6X/opentx/wiki/Modifications)

See [Wiki](https://github.com/OpenI6X/opentx/wiki) for more.

## Comparison with original firmware

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

<br>
