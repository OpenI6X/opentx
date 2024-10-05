#!/bin/bash
dfu-util -a0 -s 0x08000000:leave -d 0483:df11 -D firmware.bin
