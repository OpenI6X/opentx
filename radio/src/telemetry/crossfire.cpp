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
  {LINK_ID,        0, ZSTR_RX_RSSI1,      UNIT_DBM,            0},
  {LINK_ID,        1, ZSTR_RX_RSSI2,      UNIT_DBM,            0},
  {LINK_ID,        2, ZSTR_RX_QUALITY,    UNIT_PERCENT,        0},
  {LINK_ID,        3, ZSTR_RX_SNR,        UNIT_DB,             0},
  {LINK_ID,        4, ZSTR_ANTENNA,       UNIT_RAW,            0},
  {LINK_ID,        5, ZSTR_RF_MODE,       UNIT_RAW,            0},
  {LINK_ID,        6, ZSTR_TX_POWER,      UNIT_MILLIWATTS,     0},
  {LINK_ID,        7, ZSTR_TX_RSSI,       UNIT_DBM,            0},
  {LINK_ID,        8, ZSTR_TX_QUALITY,    UNIT_PERCENT,        0},
  {LINK_ID,        9, ZSTR_TX_SNR,        UNIT_DB,             0},
  {LINK_RX_ID,     0, ZSTR_RX_RSSI_PERC,  UNIT_PERCENT,        0},
  {LINK_RX_ID,     1, ZSTR_RX_RF_POWER,   UNIT_DBM,            0},
  {LINK_TX_ID,     0, ZSTR_TX_RSSI_PERC,  UNIT_PERCENT,        0},
  {LINK_TX_ID,     1, ZSTR_TX_RF_POWER,   UNIT_DBM,            0},
  {LINK_TX_ID,     2, ZSTR_TX_FPS,        UNIT_HERTZ,          0},
  {BATTERY_ID,     0, ZSTR_BATT,          UNIT_VOLTS,          1},
  {BATTERY_ID,     1, ZSTR_CURR,          UNIT_AMPS,           1},
  {BATTERY_ID,     2, ZSTR_CAPACITY,      UNIT_MAH,            0},
  {BATTERY_ID,     3, ZSTR_BATT_PERCENT,  UNIT_PERCENT,        0},
  {GPS_ID,         0, ZSTR_GPS,           UNIT_GPS_LATITUDE,   0},
  {GPS_ID,         0, ZSTR_GPS,           UNIT_GPS_LONGITUDE,  0},
  {GPS_ID,         2, ZSTR_GSPD,          UNIT_KMH,            1},
  {GPS_ID,         3, ZSTR_HDG,           UNIT_DEGREE,         2},
  {GPS_ID,         4, ZSTR_GPSALT,        UNIT_METERS,         0},
  {GPS_ID,         5, ZSTR_SATELLITES,    UNIT_RAW,            0},
  {ATTITUDE_ID,    0, ZSTR_PITCH,         UNIT_RADIANS,        3},
  {ATTITUDE_ID,    1, ZSTR_ROLL,          UNIT_RADIANS,        3},
  {ATTITUDE_ID,    2, ZSTR_YAW,           UNIT_RADIANS,        3},
  {FLIGHT_MODE_ID, 0, ZSTR_FLIGHT_MODE,   UNIT_TEXT,           0},
  {CF_VARIO_ID,    0, ZSTR_VSPD,          UNIT_METERS_PER_SECOND, 2},
  {BARO_ALT_ID,    0, ZSTR_ALT,           UNIT_METERS,         2},
  {AIRSPEED_ID,    0, ZSTR_ASPD,         UNIT_KMH,             1},
  {CF_RPM_ID,      0, ZSTR_RPM,          UNIT_RPMS,            0},
  {TEMP_ID,        0, ZSTR_TEMP,         UNIT_TEMPERATURE,     1},
  {CELLS_ID,       0, ZSTR_CELLS,        UNIT_CELLS,           2},
  {VOLT_ARRAY_ID,  0, ZSTR_VOLT,         UNIT_VOLTS,           2},
  {0,              0, "UNKNOWN",          UNIT_RAW,            0},
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
  else if (id == AIRSPEED_ID)
    return crossfireSensors[AIRSPEED_INDEX];
  else if (id == CF_RPM_ID)
    return crossfireSensors[CF_RPM_INDEX];
  else if (id == TEMP_ID)
    return crossfireSensors[TEMP_INDEX];
  else if (id == CELLS_ID)
    return crossfireSensors[CELLS_INDEX];
  else if (id == VOLT_ARRAY_ID)
    return crossfireSensors[VOLT_ARRAY_INDEX];
  else
    return crossfireSensors[UNKNOWN_INDEX];
}

void processCrossfireTelemetryValue(uint8_t index, int32_t value) {
  const CrossfireSensor &sensor = crossfireSensors[index];
  setTelemetryValue(PROTOCOL_TELEMETRY_CROSSFIRE, sensor.id, 0, sensor.subId, value, sensor.unit, sensor.precision);
}

template <int N>
bool getCrossfireTelemetryValue(const uint8_t *rxBuffer, uint8_t index, int32_t &value) {
  bool result = false;
  const uint8_t *byte = &rxBuffer[index];
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

// rxBuffer: one complete, CRC-checked frame of rxBufferCount bytes
// <Device address 0><Frame length 1><Type 2><Payload 3><CRC>
void processCrossfireTelemetryFrame(uint8_t *rxBuffer, uint8_t rxBufferCount) {

  if (telemetryState == TELEMETRY_INIT && moduleState[EXTERNAL_MODULE].counter != CRSF_FRAME_MODELID_SENT) {
    moduleState[EXTERNAL_MODULE].counter = CRSF_FRAME_MODELID;
  }

  uint8_t crsfPayloadLen = rxBuffer[1];
  uint8_t id = rxBuffer[2];
  int32_t value;
  switch (id) {
    case CF_VARIO_ID:
      if (getCrossfireTelemetryValue<2>(rxBuffer, 3, value))
        processCrossfireTelemetryValue(VERTICAL_SPEED_INDEX, value);
      break;

    case GPS_ID:
      if (getCrossfireTelemetryValue<4>(rxBuffer, 3, value))
        processCrossfireTelemetryValue(GPS_LATITUDE_INDEX, value / 10);
      if (getCrossfireTelemetryValue<4>(rxBuffer, 7, value))
        processCrossfireTelemetryValue(GPS_LONGITUDE_INDEX, value / 10);
      if (getCrossfireTelemetryValue<2>(rxBuffer, 11, value))
        processCrossfireTelemetryValue(GPS_GROUND_SPEED_INDEX, value);
      if (getCrossfireTelemetryValue<2>(rxBuffer, 13, value))
        processCrossfireTelemetryValue(GPS_HEADING_INDEX, value);
      if (getCrossfireTelemetryValue<2>(rxBuffer, 15, value))
        processCrossfireTelemetryValue(GPS_ALTITUDE_INDEX, value - 1000);
      if (getCrossfireTelemetryValue<1>(rxBuffer, 17, value))
        processCrossfireTelemetryValue(GPS_SATELLITES_INDEX, value);
      break;

    case BARO_ALT_ID:
      if (getCrossfireTelemetryValue<2>(rxBuffer, 3, value)) {
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
      // but support including ELRS VARIO if the declared payload length is 6 bytes or more
      if (crsfPayloadLen > 5 && getCrossfireTelemetryValue<2>(rxBuffer, 5, value))
        processCrossfireTelemetryValue(VERTICAL_SPEED_INDEX, value);
      break;

    case AIRSPEED_ID:
      if (getCrossfireTelemetryValue<2>(rxBuffer, 3, value)) {
        // Airspeed in 0.1 * km/h (hectometers/h)
        // Converstion to KMH is done through PREC1
        processCrossfireTelemetryValue(AIRSPEED_INDEX, value);
      }
      break;

    case CF_RPM_ID:
    {
      getCrossfireTelemetryValue<1>(rxBuffer, 3, value);
      uint8_t sensorID = value;
      for(uint8_t i = 0; i * 3 < (crsfPayloadLen - 4);  i++) {
        getCrossfireTelemetryValue<3>(rxBuffer, 4 + i * 3, value);
        const CrossfireSensor & sensor = crossfireSensors[CF_RPM_INDEX];
        setTelemetryValue(PROTOCOL_TELEMETRY_CROSSFIRE, sensor.id + (sensorID << 8), 0, i,
                          value, sensor.unit, sensor.precision);
      }
      break;
    }

    case TEMP_ID:
    {
      getCrossfireTelemetryValue<1>(rxBuffer, 3, value);
      uint8_t sensorID = value;
      for(uint8_t i = 0; i * 2 < (crsfPayloadLen - 4);  i++) {
        getCrossfireTelemetryValue<2>(rxBuffer, 4 + i * 2, value);
        const CrossfireSensor & sensor = crossfireSensors[TEMP_INDEX];
        setTelemetryValue(PROTOCOL_TELEMETRY_CROSSFIRE, sensor.id + (sensorID << 8), 0, i,
                          value, sensor.unit, sensor.precision);
      }
      break;
    }

    case CELLS_ID:
    {
      getCrossfireTelemetryValue<1>(rxBuffer, 3, value);
      uint8_t sensorID = value;

     if (sensorID < 128) {
        // Treating frame as Cells sensor
        // We can handle only up to 6 cells
        for(uint8_t i = 0; i * 2 < min(6 * 2, crsfPayloadLen - 4);  i++) {
          getCrossfireTelemetryValue<2>(rxBuffer, 4 + i * 2, value);
          const CrossfireSensor & sensor = crossfireSensors[CELLS_INDEX];
          setTelemetryValue(PROTOCOL_TELEMETRY_CROSSFIRE, sensor.id + (sensorID << 8), 0, 0,
                          i << 16 | value / 10, sensor.unit, sensor.precision);
        }
      } else {
        // Treating frame as Voltage sensor array
        for(uint8_t i = 0; i * 2 < (crsfPayloadLen - 4);  i++) {
          value = (rxBuffer[4 + i * 2] << 8) + rxBuffer[4 + i * 2 + 1];
          const CrossfireSensor & sensor = crossfireSensors[VOLT_ARRAY_INDEX];
          setTelemetryValue(PROTOCOL_TELEMETRY_CROSSFIRE, sensor.id + (sensorID << 8), 0, i,
                                    value / 10, sensor.unit, sensor.precision);
        }
      }
      break;
    }

    case LINK_ID:
      for (unsigned int i = 0; i <= TX_SNR_INDEX; i++) {
        if (getCrossfireTelemetryValue<1>(rxBuffer, 3 + i, value)) {
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
      if (getCrossfireTelemetryValue<1>(rxBuffer, 4, value))
        processCrossfireTelemetryValue(RX_RSSI_PERC_INDEX, value);
      if (getCrossfireTelemetryValue<1>(rxBuffer, 7, value))
        processCrossfireTelemetryValue(TX_RF_POWER_INDEX, value);
      break;

    case LINK_TX_ID:
      if (getCrossfireTelemetryValue<1>(rxBuffer, 4, value))
        processCrossfireTelemetryValue(TX_RSSI_PERC_INDEX, value);
      if (getCrossfireTelemetryValue<1>(rxBuffer, 7, value))
        processCrossfireTelemetryValue(RX_RF_POWER_INDEX, value);
      if (getCrossfireTelemetryValue<1>(rxBuffer, 8, value))
        processCrossfireTelemetryValue(TX_FPS_INDEX, value * 10);
      break;

    case BATTERY_ID:
      if (getCrossfireTelemetryValue<2>(rxBuffer, 3, value))
        processCrossfireTelemetryValue(BATT_VOLTAGE_INDEX, value);
      if (getCrossfireTelemetryValue<2>(rxBuffer, 5, value))
        processCrossfireTelemetryValue(BATT_CURRENT_INDEX, value);
      if (getCrossfireTelemetryValue<3>(rxBuffer, 7, value))
        processCrossfireTelemetryValue(BATT_CAPACITY_INDEX, value);
      if (getCrossfireTelemetryValue<1>(rxBuffer, 10, value))
        processCrossfireTelemetryValue(BATT_REMAINING_INDEX, value);
      break;

    case ATTITUDE_ID:
      if (getCrossfireTelemetryValue<2>(rxBuffer, 3, value))
        processCrossfireTelemetryValue(ATTITUDE_PITCH_INDEX, value / 10);
      if (getCrossfireTelemetryValue<2>(rxBuffer, 5, value))
        processCrossfireTelemetryValue(ATTITUDE_ROLL_INDEX, value / 10);
      if (getCrossfireTelemetryValue<2>(rxBuffer, 7, value))
        processCrossfireTelemetryValue(ATTITUDE_YAW_INDEX, value / 10);
      break;

    case FLIGHT_MODE_ID:
    {
      const CrossfireSensor &sensor = crossfireSensors[FLIGHT_MODE_INDEX];
      auto textLength = min<int>(16, rxBuffer[1]);
      rxBuffer[textLength] = '\0';
      setTelemetryText(PROTOCOL_TELEMETRY_CROSSFIRE, sensor.id, 0, sensor.subId, (const char *)rxBuffer + 3);
      break;
    }

    case RADIO_ID:
      if (rxBuffer[3] == 0xEA     // radio address
          && rxBuffer[5] == 0x10  // timing correction frame
      ) {
        uint32_t update_interval;
        int32_t offset;
        if (getCrossfireTelemetryValue<4>(rxBuffer, 6, (int32_t &)update_interval) && getCrossfireTelemetryValue<4>(rxBuffer, 10, offset)) {
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
      if (luaInputTelemetryFifo && luaInputTelemetryFifo->hasSpace(rxBufferCount - 2)) {
        for (uint32_t i = 1; i < rxBufferCount - 1; i++) {
          // destination address and CRC are skipped
          luaInputTelemetryFifo->push(rxBuffer[i]);
        }
      }
#else
      if (id == DEVICE_INFO_ID && rxBuffer[4] == CSRF_ADDRESS_TX) {
        uint8_t nameSize = rxBuffer[1] - 18;
        // strncpy((char *)&crossfireModuleStatus.name, (const char *)&rxBuffer[5], CRSF_NAME_MAXSIZE);
        // crossfireModuleStatus.name[CRSF_NAME_MAXSIZE -1] = 0; // For some reason, GH din't like strlcpy
        if (strncmp((const char *) &rxBuffer[5 + nameSize], "ELRS", 4) == 0)
          crossfireModuleStatus.isELRS = true;
        crossfireModuleStatus.major = rxBuffer[14 + nameSize];
        crossfireModuleStatus.minor = rxBuffer[15 + nameSize];
        // crossfireModuleStatus.revision = rxBuffer[16 + nameSize];
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
      runCrossfireTelemetryCallback(rxBuffer[2], rxBuffer + 2, rxBuffer[1] - 1);
#endif
      break;
  }
}

bool isCrossfireOutputBufferAvailable() {
  return outputTelemetryBufferSize == 0;
}


#define MIN_FRAME_LEN         3                       // Min size of the buffer needed to begin processing (HDR + LEN + 1)
#define MAX_FRAME_LEN         64                      // A whole CRSF packet including header and length can not exceed 64 bytes
#define MIN_PAYLOAD_LEN       3                       // Min value for the LEN field (TYPE + 1 payload + CRC)
#define MAX_PAYLOAD_LEN       (MAX_FRAME_LEN-2)       // Max value for the LEN field (MAX - HDR - LEN)

bool _checkFrameCRC(uint8_t* rxBuffer) {
  uint8_t len = rxBuffer[1];
  uint8_t crc = crc8(&rxBuffer[2], len - 1);
  return (crc == rxBuffer[len + 1]);
}

static bool _lenIsSane(uint8_t len)
{
  // Validate that the declared len in the packet is valid for a CRSF frame
  return (len >= MIN_PAYLOAD_LEN && len <= MAX_PAYLOAD_LEN);
}

static bool _validHdr(uint8_t* buf)
{
  // All CRSF packets should start with UART_SYNC, but CSRF_ADDRESS_RADIO is also accepted
  // for older modules which used the incorrect "destination address" start byte
  return buf[0] == CSRF_ADDRESS_RADIO || buf[0] == UART_SYNC;
}

// From EdgeTX (pulses/crossfire.cpp)
static uint8_t* _processFrames(uint8_t* buf, uint8_t& len)
{
  uint8_t* p_buf = buf;
  while (len >= MIN_FRAME_LEN) {
    // Bad telemetry packets frequently are just missing a byte, with the start
    // of a new packet contained in the data. To avoid dropping multiple packets
    // for one missed byte, CRC errors or invalid length errors MUST just advance
    // the buffer by one instead of pkt_len or throwing it all out
    uint8_t declared_len = p_buf[1];
    if (!_validHdr(p_buf) || !_lenIsSane(declared_len)) {
      p_buf++; len--;
      continue;
    }

    uint32_t pkt_len = declared_len + 2;
    if (pkt_len > (uint32_t)len) {
      // Need to wait for more data
      break;
    }

    if (!_checkFrameCRC(p_buf)) {
      TRACE("[XF] CRC error");
      p_buf++; len--;
      continue;
    }

    TELEMETRY_STATS_INC(telemetryStatsFrames);
    processCrossfireTelemetryFrame(p_buf, pkt_len);

    p_buf += pkt_len;
    len -= pkt_len;
  }

  return p_buf;
}

// EdgeTX crossfireProcessFrame(), reassembly path only: the caller has already
// copied the frame_len new bytes from the RX DMA FIFO to buf + *p_len (no
// separate frame[] buffer), so there is nothing to memcpy here. buf keeps an
// incomplete frame (< MAX_FRAME_LEN bytes) for the next call.
//
// Deliberate difference to EdgeTX: EdgeTX drops the whole chunk when nothing
// is pending and its first byte is not a frame start ("invalid frame start").
// Here _processFrames() resyncs over such bytes instead, so a stray byte in
// front of a frame (e.g. caught at the half-duplex turnaround) does not cost
// the frame behind it. This keeps the behaviour of the former byte parser,
// which skipped invalid start bytes one by one (host test: 10 % stray bytes
// -> EdgeTX rule loses ~10 % of the frames, this version none).
void crossfireProcessFrame(uint8_t* buf, uint8_t* p_len, uint8_t frame_len)
{
  uint8_t& len = *p_len;
  len += frame_len;

  uint8_t* p_buf = _processFrames(buf, len);
  if ((len > 0) && (p_buf != buf)) {
    memmove(buf, p_buf, len);
  }
}

void crossfireSetDefault(int index, uint16_t id, uint8_t subId) 
{
  TelemetrySensor &telemetrySensor = g_model.telemetrySensors[index];

  telemetrySensor.id = id;
  telemetrySensor.instance = subId;

  const CrossfireSensor &sensor = getCrossfireSensor(id, subId);
  TelemetryUnit unit = sensor.unit;
  if (unit == UNIT_GPS_LATITUDE || unit == UNIT_GPS_LONGITUDE)
    unit = UNIT_GPS;
  uint8_t prec = min<uint8_t>(2, sensor.precision);
  telemetrySensor.init(sensor.name, unit, prec);
#if defined(SDCARD) || defined(USB_LOGS)
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
    telemetryOutputPushByte(CSRF_ADDRESS_TX);
    telemetryOutputPushByte(2 + length);  // 1(COMMAND) + data length + 1(CRC)
    telemetryOutputPushByte(command);     // COMMAND
    for (uint32_t i = 0; i < length; i++) {
      telemetryOutputPushByte(data[i]);
    }

    telemetryOutputPushByte(crc8(outputTelemetryBuffer + 2, 1 + length));

    telemetryOutputSetTrigger(command);
    return true;
  } else {
    return false;
  }
}
#endif