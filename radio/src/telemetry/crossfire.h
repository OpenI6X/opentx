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

#ifndef _CROSSFIRE_H_
#define _CROSSFIRE_H_

#include <inttypes.h>
#include "dataconstants.h"

// Device address
#define BROADCAST_ADDRESS              0x00
#define RADIO_ADDRESS                  0xEA
#define MODULE_ADDRESS                 0xEE
#define RECEIVER_ADDRESS               0xEC

// Frame id
#define GPS_ID                         0x02
#define CF_VARIO_ID                    0x07
#define BATTERY_ID                     0x08
#define BARO_ALT_ID                    0x09
#define AIRSPEED_ID                    0x0A
#define CF_RPM_ID                      0x0C
#define TEMP_ID                        0x0D
#define CELLS_ID                       0x0E
#define LINK_ID                        0x14
#define CHANNELS_ID                    0x16
#define LINK_RX_ID                     0x1C
#define LINK_TX_ID                     0x1D
#define ATTITUDE_ID                    0x1E
#define FLIGHT_MODE_ID                 0x21
#define PING_DEVICES_ID                0x28
#define DEVICE_INFO_ID                 0x29
#define REQUEST_SETTINGS_ID            0x2A
#define COMMAND_ID                     0x32
#define RADIO_ID                       0x3A

#define UART_SYNC                      0xC8
#define SUBCOMMAND_CRSF                0x10
#define COMMAND_MODEL_SELECT_ID        0x05
#define SUBCOMMAND_CRSF_BIND           0x01

struct CrossfireSensor {
  const uint8_t id;
  const uint8_t subId;
  const char * name;
  const TelemetryUnit unit;
  const uint8_t precision;
};

enum CrossfireSensorIndexes {
  RX_RSSI1_INDEX,
  RX_RSSI2_INDEX,
  RX_QUALITY_INDEX,
  RX_SNR_INDEX,
  RX_ANTENNA_INDEX,
  RF_MODE_INDEX,
  TX_POWER_INDEX,
  TX_RSSI_INDEX,
  TX_QUALITY_INDEX,
  TX_SNR_INDEX,
  RX_RSSI_PERC_INDEX,
  RX_RF_POWER_INDEX,
  TX_RSSI_PERC_INDEX,
  TX_RF_POWER_INDEX,
  TX_FPS_INDEX,
  BATT_VOLTAGE_INDEX,
  BATT_CURRENT_INDEX,
  BATT_CAPACITY_INDEX,
  BATT_REMAINING_INDEX,
  GPS_LATITUDE_INDEX,
  GPS_LONGITUDE_INDEX,
  GPS_GROUND_SPEED_INDEX,
  GPS_HEADING_INDEX,
  GPS_ALTITUDE_INDEX,
  GPS_SATELLITES_INDEX,
  ATTITUDE_PITCH_INDEX,
  ATTITUDE_ROLL_INDEX,
  ATTITUDE_YAW_INDEX,
  FLIGHT_MODE_INDEX,
  VERTICAL_SPEED_INDEX,
  BARO_ALTITUDE_INDEX,
  AIRSPEED_INDEX,
  CF_RPM_INDEX,
  TEMP_INDEX,
  CELLS_INDEX,
  UNKNOWN_INDEX,
};

enum CrossfireFrames{
  CRSF_FRAME_CHANNEL,
  CRSF_FRAME_MODELID,
  CRSF_FRAME_MODELID_SENT
};

void registerCrossfireTelemetryCallback(void (*callback)(uint8_t, uint8_t*, uint8_t));
void runCrossfireTelemetryCallback(uint8_t command, uint8_t* data, uint8_t length);

struct CrossfireModuleStatus
{
    uint8_t major;
    uint8_t minor;
//    uint8_t revision;
    // char name[CRSF_NAME_MAXSIZE];
    bool queryCompleted;
    bool isELRS;
};

extern CrossfireModuleStatus crossfireModuleStatus;

void processCrossfireTelemetryData(uint8_t data);
void crossfireSetDefault(int index, uint16_t id, uint8_t subId);
bool isCrossfireOutputBufferAvailable();
uint8_t createCrossfireModelIDFrame(uint8_t * frame);

const uint32_t CROSSFIRE_BAUDRATES[] = {
  400000,
  115200,
  921600,
  1870000,
  3750000,
  5250000,
};
const uint8_t CROSSFIRE_PERIODS[] = {
  4,
  16,
  4,
  4,
  4,
  4,
};

#define CROSSFIRE_BAUDRATE    CROSSFIRE_BAUDRATES[g_eeGeneral.telemetryBaudrate]
#define CROSSFIRE_PERIOD      (CROSSFIRE_PERIODS[g_eeGeneral.telemetryBaudrate] * 1000)

#define CROSSFIRE_TELEM_MIRROR_BAUDRATE   115200

#define CRSF_ELRS_MIN_VER(maj, min) \
        (crossfireModuleStatus.isELRS \
         && crossfireModuleStatus.major >= maj)
/*       && (crossfireModuleStatus.major > maj \
          || (crossfireModuleStatus.major == maj \
           && crossfireModuleStatus.minor >= min))) */

#if !defined(LUA)
bool crossfireTelemetryPush(uint8_t command, uint8_t *data, uint32_t length);
#endif

#endif // _CROSSFIRE_H_
