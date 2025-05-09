/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"
#include "stamp.h"

#define STR2(s) #s
#define DEFNUMSTR(s)  STR2(s)

#define EEPROM_STR DEFNUMSTR(EEPROM_VER);

#if defined(PCBHORUS)
#define TAB "\037\075"
#else
#define TAB "\037\033"
#endif

#if defined(FRSKY_RELEASE)
#define DISPLAY_VERSION "-frsky"
#elif defined(FLYSKY_RELEASE)
#define DISPLAY_VERSION "-flysky"
#elif defined(JUMPER_RELEASE)
#define DISPLAY_VERSION "-jumper"
#elif defined(RADIOMASTER_RELEASE)
#define DISPLAY_VERSION "-RM"
#elif defined(TBS_RELEASE)
#define DISPLAY_VERSION "-tbs"
#elif defined(IMRC_RELEASE)
#define DISPLAY_VERSION "-imrc"
#else
#define DISPLAY_VERSION
#endif

#if defined(COLORLCD)
  const char fw_stamp[]    =   "FW" TAB ": opentx-" FLAVOUR;
#if defined(RADIOMASTER_RELEASE) || defined(JUMPER_RELEASE)
  const char vers_stamp[]  =   "VERS" TAB ": Factory firmware (" GIT_STR ")";
#else
  const char vers_stamp[]  =   "VERS" TAB ": " VERSION DISPLAY_VERSION " (" GIT_STR ")";
#endif
  const char date_stamp[]  =   "DATE" TAB ": " DATE;
  const char time_stamp[]  =   "TIME" TAB ": " TIME;
  const char eeprom_stamp[]  = "EEPR" TAB ": " EEPROM_STR;
#elif defined(BOARD_NAME)
  const char vers_stamp[]  = "FW" TAB ": opentx-" BOARD_NAME "\036VERS" TAB ": " VERSION DISPLAY_VERSION " (" GIT_STR ")" "\036DATE" TAB ": " DATE " " TIME "\036EEPR" TAB ": " EEPROM_STR;
#elif defined(RADIOMASTER_RELEASE)
  const char vers_stamp[]  = "FW" TAB ": opentx-" FLAVOUR    "\036VERS" TAB ": RM Factory (" GIT_STR ")" "\036BUILT BY : OpenTX" "\036DATE" TAB ": " DATE " " TIME "\036EEPR" TAB ": " EEPROM_STR;
#elif defined(JUMPER_RELEASE)
  const char vers_stamp[]  = "FW" TAB ": opentx-" FLAVOUR    "\036VERS" TAB ": Factory (" GIT_STR ")" "\036BUILT BY : OpenTX" "\036DATE" TAB ": " DATE " " TIME "\036EEPR" TAB ": " EEPROM_STR;
#else
  const char vers_stamp[]  = "FW" TAB ": openi6x" "\036VERS" TAB ": " VERSION DISPLAY_VERSION " (" GIT_STR ")" "\036DATE" TAB ": " DATE " " TIME "\036EEPR" TAB ": " EEPROM_STR;
#endif

/**
 * Retrieves the version of the bootloader or firmware
 * @return
 */
#if defined(STM32) && !defined(PCBI6X) // FLASH space opt

__SECTION_USED(".fwversiondata")   const char firmware_version[] = "opentx-" FLAVOUR "-" VERSION " (" GIT_STR ")";
__SECTION_USED(".bootversiondata") const char boot_version[] =     "opentx-" FLAVOUR "-" VERSION " (" GIT_STR ")";

/**
 * Tries to find opentx version in the first 1024 byte of either firmware/bootloader (the one not running) or the buffer
 * @param buffer If non-null find the firmware version in the buffer instead
 */
const char* getOtherVersion(const char* buffer)
{
  if (buffer == nullptr) {
#if defined(BOOT)
    buffer = (const char *)(FIRMWARE_ADDRESS + BOOTLOADER_SIZE);
#else
    buffer = (const char *)FIRMWARE_ADDRESS;
#endif
  }

  for (int i = 0; i < 1024; i++) {
    if (memcmp(buffer + i, "openi6x-", 7) == 0) {
      return buffer + i;
    }
  }

  return "-";
}
#endif
