[![Release](https://img.shields.io/github/v/release/OpenI6X/opentx?include_prereleases)](https://github.com/OpenI6X/opentx/releases/latest)
[![GitHub all releases](https://img.shields.io/github/downloads/OpenI6X/opentx/total)](https://github.com/OpenI6X/opentx/releases)
[![GitHub license](https://img.shields.io/github/license/OpenI6X/opentx)](https://github.com/openi6x/opentx/blob/master/LICENSE)
![OpenI6X](https://circleci.com/gh/OpenI6X/opentx.svg?style=shield)
[![Discord](https://img.shields.io/discord/973289741862727741.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/3vKfYNTVa2)

![Banner](https://github.com/OpenI6X/opentx/blob/master/doc/flysky/banner.png?raw=true)

# OpenI6X

**OpenTX / EdgeTX for the FlySky FS-i6X** — turn a budget radio into a pro-grade transmitter.

The FlySky **FS-i6X** is solid, inexpensive hardware held back by basic firmware. **OpenI6X** replaces it with a community-driven port of **OpenTX / EdgeTX** — the same open-source radio operating system used by high-end transmitters — unlocking features that would normally cost you a new radio.

The goal of this project is to bring customized EdgeTX / OpenTX to the FlySky i6X and other STM32F0-based radios. It's free, it's open source — and it's reversible: you can back up your stock firmware and go back any time you like.

Join our [Discord](https://discord.gg/3vKfYNTVa2), [RCGroups](https://www.rcgroups.com/forums/showthread.php?3916435-FlySky-I6X-port-of-OpenTX) or [Telegram](https://t.me/otx_flysky_i6x) to contribute, discuss or get help.


## Quick start

> [!IMPORTANT]
> **Before your first flash, back up your stock firmware** (the one-command backup is in step 3). Radio settings and model memory are **not** migrated and will be lost — and that backup is also your way back to stock firmware whenever you want it.

Good news: **no soldering iron required** — just a screwdriver, a USB cable and something metal to bridge two pads.

### 1 · Download the firmware

Grab a `.zip` from the [latest release](https://github.com/OpenI6X/opentx/releases/latest) and pick your feature set:

| Build | Heli menu | INAV Lite telemetry | DFPlayer voice |
| --- | :---: | :---: | :---: |
| `openi6x_<version>.zip` | — | ✓ | — |
| `openi6x_<version>_dfplayer.zip` | — | ✓ *(English only)* | ✓ |
| `openi6x_<version>_heli.zip` | ✓ | — | — |
| `openi6x_<version>_heli_dfplayer.zip` | ✓ | — | ✓ |

Every archive contains `.bin` files for **all supported languages** — no need to pick one upfront. DFPlayer voice requires the [hardware mod](https://github.com/OpenI6X/opentx/wiki/Modifications#dfplayer).

### 2 · Put the radio into DFU mode

**Coming from stock firmware (first flash only):**

1. Power the radio off and open the case.
2. Bridge the two pads labeled `R53` — hold a wire or tweezers across them, no soldering needed.
3. Still bridging, connect USB and switch the radio on.
4. Your computer will detect a new device: **STM32 BOOTLOADER**.

<details>
<summary>What do the pads look like? (photo)</summary>

<img src="https://raw.githubusercontent.com/OpenI6X/opentx/master/doc/flysky/dfu_jump_r53.jpg" alt="R53 pads on the FS-i6X main board" width="420"/>

<sub>The pads marked <code>R53</code> on the FS-i6X main board.</sub>
</details>

> [!TIP]
> Already running OpenI6X? Upgrades are much easier — **hold both inner horizontal trim switches while powering on** and the radio enters DFU mode by itself. No disassembly, ever again.

If your computer doesn't see the *STM32 BOOTLOADER* device, you probably need drivers — the [flashing guide](https://github.com/OpenI6X/opentx/wiki/Flashing-&-Upgrading) covers the Windows fixes (STM32 drivers or [Zadig](https://www.hanselman.com/blog/how-to-fix-dfuutil-stm-winusb-zadig-bootloaders-and-other-firmware-flashing-issues-on-windows)).

### 3 · Flash it

The easiest way is [**EdgeTX Buddy**](https://buddy.edgetx.org) — it flashes straight from your browser via WebUSB (use Chrome, Edge or another Chromium-based browser):

1. Choose **Local file** and drag & drop your `.bin`.
2. Click **Flash via USB** and select the STM32 BOOTLOADER device.
3. Click through and wait for the process to finish. Done!

<details>
<summary>Prefer a terminal or other tools?</summary>

**Back up your stock firmware (recommended before first flash):**

```
dfu-util -s 0x08000000:leave -a 0 -U backup.bin
```

**Flash OpenI6X:**

```
dfu-util -s 0x08000000:leave -a 0 -D firmware.bin
```

Also works: [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html), [Geehy DFUProgrammer](https://www.geehy.com/support/apm32/?id=383) (for APM32-based radios) and [QMK Toolbox](https://github.com/qmk/qmk_toolbox).

Note: the official FlySky updater **cannot** be used for this.
</details>

### 4 · Fly!

Remove the R53 bridge, power-cycle the radio, calibrate your sticks (see the [manual](https://github.com/OpenI6X/opentx/wiki/Manual)) and set up your first model.

> [!TIP]
> Flashing third-party firmware is done at your own risk. OpenI6X is a community project and is **not affiliated with or endorsed by FlySky**. Read the [flashing guide](https://github.com/OpenI6X/opentx/wiki/Flashing-&-Upgrading) before you start — and if in doubt, ask on [Discord](https://discord.gg/3vKfYNTVa2).

## Hardware mods

The fun doesn't stop at firmware — the wiki's [Modifications](https://github.com/OpenI6X/opentx/wiki/Modifications) page covers:

- [DFPlayer voice announcements](https://github.com/OpenI6X/opentx/wiki/Modifications#dfplayer)
- [Adjustable backlight brightness](https://github.com/OpenI6X/opentx/wiki/Modifications#adjustable-backlight-level)
- [FS-HZCZ03-ADJ hall gimbal upgrade](https://github.com/OpenI6X/opentx/wiki/Modifications#flysky-fs-hzcz03-adj-gimbal)
- External ExpressLRS module — and more

## Documentation

| Guide | What you'll find |
| --- | --- |
| [Manual](https://github.com/OpenI6X/opentx/wiki/Manual) | The OpenI6X user interface and differences from official EdgeTX / OpenTX radios |
| [EdgeTX manual](https://manual.edgetx.org/bw-radios) | The official EdgeTX manual for B&W radios — applies to OpenI6X for the most part |
| [Flashing & upgrading](https://github.com/OpenI6X/opentx/wiki/Flashing-&-Upgrading) | Install, update or restore firmware, with troubleshooting |
| [Modifications](https://github.com/OpenI6X/opentx/wiki/Modifications) | Hardware mods: DFPlayer, backlight, gimbals, ELRS module, … |
| [Development guide](https://github.com/OpenI6X/opentx/wiki/Development) | Building from source and build-time options |
| [Wiki](https://github.com/OpenI6X/opentx/wiki) | Everything else |


## Comparison with original firmware

| Feature                   | Flysky i6X | OpenI6X                      |
|---------------------------|------------|------------------------------|
| Channels                  | 6/10       | 16                           |
| Mixers                    | 3          | 32                           |
| Models                    | 20         | 20 / unlimited<sup>[1]</sup> |
| Protocols                 | AFHDS, AFHDS2A, PPM | 14/16Ch AFHDS2A, PPM, CRSF |
| Trainer                   | PPM        | PPM, SBUS                    |
| Timers                    | _          | ✓                            |
| Voice annoucements        | _          | ✓<sup>[2]</sup>              |
| Variometer                | _          | ✓                            |
| ExpressLRS ready          | _          | ✓ Configurator built-in (no need for LUA) |
| Adjustable screen brightness | _       | ✓<sup>[3]</sup>              |
| USB Modes                 | Joystick   | Joystick, Storage, Serial (Telemetry mirror, Debug) |
| AUX Serial port           | _          | ✓ SBUS Trainer, Telemetry mirror, Debug |
| Flysky FS-HZCZ03-ADJ Gimbal support | _   | ✓<sup>[4]</sup>    |
| Languages                 | EN, CN      | PL, EN, CZ, DE, ES, FI, FR, IT, NL, PT, SE |

<sub>[1] Unlimited by using USB mass storage mode eeprom backup/restore.</sub><br>
<sub>[2] By adding DFPlayer, see [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#dfplayer) in wiki.</sub><br>
<sub>[3] By wiring 2 pads, see [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#adjustable-backlight-level) in wiki.</sub><br>
<sub>[4] See [instructions](https://github.com/OpenI6X/opentx/wiki/Modifications#flysky-fs-hzcz03-adj-gimbal) in wiki.</sub>

<br>

## Contributing

OpenI6X is built by people like you, and new contributors are always welcome:

- Found a bug? [Open an issue](https://github.com/OpenI6X/opentx/issues) (please check [existing ones](https://github.com/OpenI6X/opentx/issues?q=is%3Aissue) first).
- Have an idea? Feature suggestions are welcome — especially with a pull request attached.
- Improve the documentation — the [wiki](https://github.com/OpenI6X/opentx/wiki) and this README always need love.
- Help translate — the radio speaks 11 languages thanks to contributors.
- Want to hack on the firmware? The [development guide](https://github.com/OpenI6X/opentx/wiki/Development) gets you building in minutes.

If you find OpenI6X useful, consider starring the repo — it helps others discover the project.
