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

const CrossfireSensor crossfireSensors[] = {
    {LINK_ID, 0, ZSTR_RX_RSSI1, UNIT_DB, 0},
    {LINK_ID, 1, ZSTR_RX_RSSI2, UNIT_DB, 0},
    {LINK_ID, 2, ZSTR_RX_QUALITY, UNIT_PERCENT, 0},
    {LINK_ID, 3, ZSTR_RX_SNR, UNIT_DB, 0},
    {LINK_ID, 4, ZSTR_ANTENNA, UNIT_RAW, 0},
    {LINK_ID, 5, ZSTR_RF_MODE, UNIT_RAW, 0},
    {LINK_ID, 6, ZSTR_TX_POWER, UNIT_MILLIWATTS, 0},
    {LINK_ID, 7, ZSTR_TX_RSSI, UNIT_DB, 0},
    {LINK_ID, 8, ZSTR_TX_QUALITY, UNIT_PERCENT, 0},
    {LINK_ID, 9, ZSTR_TX_SNR, UNIT_DB, 0},
    {LINK_RX_ID, 0, ZSTR_RX_RSSI_PERC, UNIT_PERCENT, 0},
    {LINK_RX_ID, 1, ZSTR_RX_RF_POWER, UNIT_DBM, 0},
    {LINK_TX_ID, 0, ZSTR_TX_RSSI_PERC, UNIT_PERCENT, 0},
    {LINK_TX_ID, 1, ZSTR_TX_RF_POWER, UNIT_DBM, 0},
    {LINK_TX_ID, 2, ZSTR_TX_FPS, UNIT_HERTZ, 0},
    {BATTERY_ID, 0, ZSTR_BATT, UNIT_VOLTS, 1},
    {BATTERY_ID, 1, ZSTR_CURR, UNIT_AMPS, 1},
    {BATTERY_ID, 2, ZSTR_CAPACITY, UNIT_MAH, 0},
    {BATTERY_ID, 3, ZSTR_BATT_PERCENT, UNIT_PERCENT, 0},
    {GPS_ID, 0, ZSTR_GPS, UNIT_GPS_LATITUDE, 0},
    {GPS_ID, 0, ZSTR_GPS, UNIT_GPS_LONGITUDE, 0},
    {GPS_ID, 2, ZSTR_GSPD, UNIT_KMH, 1},
    {GPS_ID, 3, ZSTR_HDG, UNIT_DEGREE, 2},
    {GPS_ID, 4, ZSTR_ALT, UNIT_METERS, 0},
    {GPS_ID, 5, ZSTR_SATELLITES, UNIT_RAW, 0},
    {ATTITUDE_ID, 0, ZSTR_PITCH, UNIT_RADIANS, 3},
    {ATTITUDE_ID, 1, ZSTR_ROLL, UNIT_RADIANS, 3},
    {ATTITUDE_ID, 2, ZSTR_YAW, UNIT_RADIANS, 3},
    {FLIGHT_MODE_ID, 0, ZSTR_FLIGHT_MODE, UNIT_TEXT, 0},
    {CF_VARIO_ID, 0, ZSTR_VSPD, UNIT_METERS_PER_SECOND, 2},
    {BARO_ALT_ID, 0, ZSTR_ALT, UNIT_METERS, 2},
    {0, 0, "UNKNOWN", UNIT_RAW, 0},
};

CrossfireModuleStatus crossfireModuleStatus = {0};

const CrossfireSensor &getCrossfireSensor(uint8_t id, uint8_t subId) {
  if (id == LINK_ID)
    return crossfireSensors[RX_RSSI1_INDEX + subId];
  else if (id == LINK_RX_ID)
    return crossfireSensors[RX_RSSI_PERC_INDEX + subId];
  else if (id == LINK_TX_ID)
    return crossfireSensors[TX_RSSI_PERC_INDEX + subId];
  else if (id == BATTERY_ID)
    return crossfireSensors[BATT_VOLTAGE_INDEX + subId];
  else if (id == GPS_ID)
    return crossfireSensors[GPS_LATITUDE_INDEX + subId];
  else if (id == CF_VARIO_ID)
    return crossfireSensors[VERTICAL_SPEED_INDEX];
  else if (id == ATTITUDE_ID)
    return crossfireSensors[ATTITUDE_PITCH_INDEX + subId];
  else if (id == FLIGHT_MODE_ID)
    return crossfireSensors[FLIGHT_MODE_INDEX];
  else if (id == BARO_ALT_ID)
    return crossfireSensors[BARO_ALTITUDE_INDEX];
  else
    return crossfireSensors[UNKNOWN_INDEX];
}

void processCrossfireTelemetryValue(uint8_t index, int32_t value) {
  const CrossfireSensor &sensor = crossfireSensors[index];
  setTelemetryValue(PROTOCOL_TELEMETRY_CROSSFIRE, sensor.id, 0, sensor.subId, value, sensor.unit, sensor.precision);
}

bool checkCrossfireTelemetryFrameCRC() {
  uint8_t len = telemetryRxBuffer[1];
#if defined(PCBI6X)
  uint8_t crc = crc8_hw(&telemetryRxBuffer[2], len - 1);
#else
  uint8_t crc = crc8(&telemetryRxBuffer[2], len - 1);
#endif
  return (crc == telemetryRxBuffer[len + 1]);
}

template <int N>
bool getCrossfireTelemetryValue(uint8_t index, int32_t &value) {
  bool result = false;
  uint8_t *byte = &telemetryRxBuffer[index];
  value = (*byte & 0x80) ? -1 : 0;
  for (uint32_t i = 0; i < N; i++) {
    value <<= 8;
    if (*byte != 0xff) {
      result = true;
    }
    value += *byte++;
  }
  return result;
}

void processCrossfireTelemetryFrame() {

  if (telemetryState == TELEMETRY_INIT && moduleState[EXTERNAL_MODULE].counter != CRSF_FRAME_MODELID_SENT) {
    moduleState[EXTERNAL_MODULE].counter = CRSF_FRAME_MODELID;
  }

  uint8_t crsfPayloadLen = telemetryRxBuffer[1];
  uint8_t id = telemetryRxBuffer[2];
  int32_t value;
  switch (id) {
    case CF_VARIO_ID:
      if (getCrossfireTelemetryValue<2>(3, value))
        processCrossfireTelemetryValue(VERTICAL_SPEED_INDEX, value);
      break;

    case GPS_ID:
      if (getCrossfireTelemetryValue<4>(3, value))
        processCrossfireTelemetryValue(GPS_LATITUDE_INDEX, value / 10);
      if (getCrossfireTelemetryValue<4>(7, value))
        processCrossfireTelemetryValue(GPS_LONGITUDE_INDEX, value / 10);
      if (getCrossfireTelemetryValue<2>(11, value))
        processCrossfireTelemetryValue(GPS_GROUND_SPEED_INDEX, value);
      if (getCrossfireTelemetryValue<2>(13, value))
        processCrossfireTelemetryValue(GPS_HEADING_INDEX, value);
      if (getCrossfireTelemetryValue<2>(15, value))
        processCrossfireTelemetryValue(GPS_ALTITUDE_INDEX, value - 1000);
      if (getCrossfireTelemetryValue<1>(17, value))
        processCrossfireTelemetryValue(GPS_SATELLITES_INDEX, value);
      break;

    case BARO_ALT_ID:
      if (getCrossfireTelemetryValue<2>(3, value)) {
        if (value & 0x8000) {
          // Altitude in meters
          value &= ~(0x8000);
          value *= 100; // cm
        } else {
          // Altitude in decimeters + 10000dm
          value -= 10000;
          value *= 10;
        }
        processCrossfireTelemetryValue(BARO_ALTITUDE_INDEX, value);
      }
      // Length of TBS BARO_ALT has 4 payload bytes with just 2 bytes of altitude
      // but support including VARIO if the declared payload length is 6 bytes or more
      if (crsfPayloadLen > 5 && getCrossfireTelemetryValue<2>(5, value))
        processCrossfireTelemetryValue(VERTICAL_SPEED_INDEX, value);
      break;

    case LINK_ID:
      for (unsigned int i = 0; i <= TX_SNR_INDEX; i++) {
        if (getCrossfireTelemetryValue<1>(3 + i, value)) {
          if (i == TX_POWER_INDEX) {
            static const int32_t power_values[] = {0, 10, 25, 100, 500, 1000, 2000, 250, 50};
            value = ((unsigned)value < DIM(power_values) ? power_values[value] : 0);
          }
          processCrossfireTelemetryValue(i, value);
          if (i == RX_QUALITY_INDEX) {
            if (value) {
              telemetryData.rssi.set(value);
              telemetryStreaming = TELEMETRY_TIMEOUT10ms;
            } else {
              telemetryData.rssi.reset();
              telemetryStreaming = 0;
            }
          }
        }
      }
      break;

    case LINK_RX_ID:
      if (getCrossfireTelemetryValue<1>(4, value))
        processCrossfireTelemetryValue(RX_RSSI_PERC_INDEX, value);
      if (getCrossfireTelemetryValue<1>(7, value))
        processCrossfireTelemetryValue(TX_RF_POWER_INDEX, value);
      break;

    case LINK_TX_ID:
      if (getCrossfireTelemetryValue<1>(4, value))
        processCrossfireTelemetryValue(TX_RSSI_PERC_INDEX, value);
      if (getCrossfireTelemetryValue<1>(7, value))
        processCrossfireTelemetryValue(RX_RF_POWER_INDEX, value);
      if (getCrossfireTelemetryValue<1>(8, value))
        processCrossfireTelemetryValue(TX_FPS_INDEX, value * 10);
      break;

    case BATTERY_ID:
      if (getCrossfireTelemetryValue<2>(3, value))
        processCrossfireTelemetryValue(BATT_VOLTAGE_INDEX, value);
      if (getCrossfireTelemetryValue<2>(5, value))
        processCrossfireTelemetryValue(BATT_CURRENT_INDEX, value);
      if (getCrossfireTelemetryValue<3>(7, value))
        processCrossfireTelemetryValue(BATT_CAPACITY_INDEX, value);
      if (getCrossfireTelemetryValue<1>(10, value))
        processCrossfireTelemetryValue(BATT_REMAINING_INDEX, value);
      break;

    case ATTITUDE_ID:
      if (getCrossfireTelemetryValue<2>(3, value))
        processCrossfireTelemetryValue(ATTITUDE_PITCH_INDEX, value / 10);
      if (getCrossfireTelemetryValue<2>(5, value))
        processCrossfireTelemetryValue(ATTITUDE_ROLL_INDEX, value / 10);
      if (getCrossfireTelemetryValue<2>(7, value))
        processCrossfireTelemetryValue(ATTITUDE_YAW_INDEX, value / 10);
      break;

    case FLIGHT_MODE_ID: {
      const CrossfireSensor &sensor = crossfireSensors[FLIGHT_MODE_INDEX];
      auto textLength = min<int>(16, telemetryRxBuffer[1]);
      telemetryRxBuffer[textLength] = '\0';
      setTelemetryText(PROTOCOL_TELEMETRY_CROSSFIRE, sensor.id, 0, sensor.subId, (const char *)telemetryRxBuffer + 3);
      break;
    }

    case RADIO_ID:
      if (telemetryRxBuffer[3] == 0xEA     // radio address
          && telemetryRxBuffer[5] == 0x10  // timing correction frame
      ) {
        uint32_t update_interval;
        int32_t offset;
        if (getCrossfireTelemetryValue<4>(6, (int32_t &)update_interval) && getCrossfireTelemetryValue<4>(10, offset)) {
          // values are in 10th of micro-seconds
          update_interval /= 10;
          offset /= 10;

          // TRACE("[XF] Rate: %d, Lag: %d", update_interval, offset);
          getModuleSyncStatus(EXTERNAL_MODULE).update(update_interval, offset);
        }
      }
      break;
    default:
#if defined(LUA)
      if (luaInputTelemetryFifo && luaInputTelemetryFifo->hasSpace(telemetryRxBufferCount - 2)) {
        for (uint32_t i = 1; i < telemetryRxBufferCount - 1; i++) {
          // destination address and CRC are skipped
          luaInputTelemetryFifo->push(telemetryRxBuffer[i]);
        }
      }
#else
      if (id == DEVICE_INFO_ID && telemetryRxBuffer[4] == MODULE_ADDRESS) {
        uint8_t nameSize = telemetryRxBuffer[1] - 18;
        // strncpy((char *)&crossfireModuleStatus.name, (const char *)&telemetryRxBuffer[5], CRSF_NAME_MAXSIZE);
        // crossfireModuleStatus.name[CRSF_NAME_MAXSIZE -1] = 0; // For some reason, GH din't like strlcpy
        if (strncmp((const char *) &telemetryRxBuffer[5 + nameSize], "ELRS", 4) == 0)
          crossfireModuleStatus.isELRS = true;
        crossfireModuleStatus.major = telemetryRxBuffer[14 + nameSize];
        crossfireModuleStatus.minor = telemetryRxBuffer[15 + nameSize];
        // crossfireModuleStatus.revision = telemetryRxBuffer[16 + nameSize];
        crossfireModuleStatus.queryCompleted = true;
      }

      ModuleData *md = &g_model.moduleData[EXTERNAL_MODULE];

      if (!CRSF_ELRS_MIN_VER(4, 0) &&
          (md->crsf.crsfArmingMode != ARMING_MODE_CH5 || md->crsf.crsfArmingMode != SWSRC_NONE)) {
        md->crsf.crsfArmingMode = ARMING_MODE_CH5;
        md->crsf.crsfArmingTrigger = SWSRC_NONE;

        storageDirty(EE_MODEL);
      }

      // <Device address 0><Frame length 1><Type 2><Payload 3><CRC>
      // destination address and CRC are skipped
      runCrossfireTelemetryCallback(telemetryRxBuffer[2], telemetryRxBuffer + 2, telemetryRxBuffer[1] - 1);
#endif
      break;
  }
}

bool isCrossfireOutputBufferAvailable() {
  return outputTelemetryBufferSize == 0;
}

bool crossfireLenIsSane(uint8_t len)
{
  // packet len must be at least 3 bytes (type+payload+crc) and 2 bytes < MAX (hdr+len)
  return (len > 2 && len < TELEMETRY_RX_PACKET_SIZE-1);
}

void crossfireTelemetrySeekStart(uint8_t *rxBuffer, uint8_t &rxBufferCount)
{
  // Bad telemetry packets frequently are just truncated packets, with the start
  // of a new packet contained in the data. This causes multiple packet drops as
  // the parser tries to resync.
  // Search through the rxBuffer for a sync byte, shift the contents if found
  // and reduce rxBufferCount
  for (uint32_t idx=1; idx<rxBufferCount; ++idx) {
    uint8_t data = rxBuffer[idx];
    if (data == RADIO_ADDRESS || data == UART_SYNC) {
      uint8_t remain = rxBufferCount - idx;
      // If there's at least 2 bytes, check the length for validity too
      if (remain > 1 && !crossfireLenIsSane(rxBuffer[idx+1]))
        continue;

      TRACE("Found 0x%02x with %u remain", data, remain);
      // copy the data to the front of the buffer
      for (uint8_t src=idx; src<rxBufferCount; ++src) {
        rxBuffer[src-idx] = rxBuffer[src];
      }

      rxBufferCount = remain;
      return;
    } // if found sync
  }

  // Not found, clear the buffer
  rxBufferCount = 0;
}

void processCrossfireTelemetryData(uint8_t data) {

#if !defined(DEBUG) && defined(USB_SERIAL)
  if (getSelectedUsbMode() == USB_SERIAL_MODE) {
    usbSerialPutc(data);
  }
#endif

#if defined(AUX_SERIAL)
  if (g_eeGeneral.auxSerialMode == UART_MODE_TELEMETRY_MIRROR) {
    auxSerialPutc(data);
  }
#endif

  if (telemetryRxBufferCount == 0 && data != RADIO_ADDRESS) {
    TRACE("[XF] addr 0x%02X err", data);
    return;
  }

  if (telemetryRxBufferCount == 1 && !crossfireLenIsSane(data)) {
    TRACE("[XF] len 0x%02X err", data);
    telemetryRxBufferCount = 0;
    return;
  }

  if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
    telemetryRxBuffer[telemetryRxBufferCount++] = data;
  } else {
    TRACE("[XF] arr size %d err", telemetryRxBufferCount);
    telemetryRxBufferCount = 0;
  }

  // telemetryRxBuffer[1] holds the packet length-2, check if the whole packet was received
  while (telemetryRxBufferCount > 4 && (telemetryRxBuffer[1]+2) == telemetryRxBufferCount) {
    if (checkCrossfireTelemetryFrameCRC()) {
      processCrossfireTelemetryFrame();
      telemetryRxBufferCount = 0;
    }
    else {
      TRACE("[XF] CRC err");
      crossfireTelemetrySeekStart(telemetryRxBuffer, telemetryRxBufferCount); // adjusts telemetryRxBufferCount
    }
  }
}

void crossfireSetDefault(int index, uint8_t id, uint8_t subId) {
  TelemetrySensor &telemetrySensor = g_model.telemetrySensors[index];

  telemetrySensor.id = id;
  telemetrySensor.instance = subId;

  const CrossfireSensor &sensor = getCrossfireSensor(id, subId);
  TelemetryUnit unit = sensor.unit;
  if (unit == UNIT_GPS_LATITUDE || unit == UNIT_GPS_LONGITUDE)
    unit = UNIT_GPS;
  uint8_t prec = min<uint8_t>(2, sensor.precision);
  telemetrySensor.init(sensor.name, unit, prec);
#if defined(SDCARD) // no sdcard logs on i6X
  if (id == LINK_ID) {
    telemetrySensor.logs = true;
  }
#endif
  storageDirty(EE_MODEL);
}

#if !defined(LUA)
/**
 * Skip luaInputTelemetryFifo and luaCrossfireTelemetryPop() to save RAM and provide synchronous API instead
 */
void (*crossfireTelemetryCallback)(uint8_t, uint8_t*, uint8_t);

void registerCrossfireTelemetryCallback(void (*callback)(uint8_t, uint8_t*, uint8_t)) {
  crossfireTelemetryCallback = callback;
}

inline void runCrossfireTelemetryCallback(uint8_t command, uint8_t* data, uint8_t length) {
  if (crossfireTelemetryCallback != nullptr) {
    crossfireTelemetryCallback(command, data, length);
  }
}

bool crossfireTelemetryPush(uint8_t command, uint8_t *data, uint32_t length) {
  // TRACE("crsfPush %x", command);
  if (isCrossfireOutputBufferAvailable()) {
    telemetryOutputPushByte(MODULE_ADDRESS);
    telemetryOutputPushByte(2 + length);  // 1(COMMAND) + data length + 1(CRC)
    telemetryOutputPushByte(command);     // COMMAND
    for (uint32_t i = 0; i < length; i++) {
      telemetryOutputPushByte(data[i]);
    }
#if defined(PCBI6X)
    telemetryOutputPushByte(crc8_hw(outputTelemetryBuffer + 2, 1 + length));
#else
    telemetryOutputPushByte(crc8(outputTelemetryBuffer + 2, 1 + length));
#endif
    telemetryOutputSetTrigger(command);
    return true;
  } else {
    return false;
  }
}
#endif