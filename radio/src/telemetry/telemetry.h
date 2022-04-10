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

#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

#include "telemetry_holders.h"
#include "telemetry_sensors.h"

#if defined(TELEMETRY_FRSKY)
  // FrSky Telemetry
  #include "frsky.h"
#endif

#if defined(CROSSFIRE)
  #include "crossfire.h"
#endif
#if defined(MULTIMODULE)
  #include "spektrum.h"
  #include "flysky_ibus.h"
  #include "multi.h"
#endif
#if defined(PCBI6X)
  #include "flysky_ibus.h"
#endif

enum TelemetryProtocol
{
  TELEM_PROTO_FRSKY_D,
  TELEM_PROTO_FRSKY_SPORT,
  TELEM_PROTO_CROSSFIRE,
  TELEM_PROTO_SPEKTRUM,
  TELEM_PROTO_LUA,
  TELEM_PROTO_FLYSKY_IBUS,
};

void telemetryInit(uint8_t protocol);
void telemetryWakeup();
void telemetryReset();
void telemetryInterrupt10ms();
int setTelemetryValue(TelemetryProtocol protocol, uint16_t id, uint8_t subId, uint8_t instance, int32_t value, uint32_t unit, uint32_t prec);
int setTelemetryText(TelemetryProtocol protocol, uint16_t id, uint8_t subId, uint8_t instance, const char * text);

void delTelemetryIndex(uint8_t index);
int availableTelemetryIndex();
int lastUsedTelemetryIndex();
int32_t getTelemetryValue(uint8_t index, uint8_t & prec);
int32_t convertTelemetryValue(int32_t value, uint8_t unit, uint8_t prec, uint8_t destUnit, uint8_t destPrec);



enum TelemAnas {
  TELEM_ANA_A1,
  TELEM_ANA_A2,
  TELEM_ANA_A3,
  TELEM_ANA_A4,
  TELEM_ANA_COUNT
};

struct TelemetryData {
  TelemetryValueWithMin swr;          // TODO Min not needed
  TelemetryValueWithMin rssi;         // TODO Min not needed
  uint16_t xjtVersion;
  bool varioHighPrecision;
};
extern uint8_t telemetryProtocol;
extern TelemetryData telemetryData;
extern uint8_t telemetryStreaming; // >0 (true) == data is streaming in. 0 = no data detected for some time

#if defined(WS_HOW_HIGH)
extern uint8_t wshhStreaming;
#endif

enum TelemetryStates {
  TELEMETRY_INIT,
  TELEMETRY_OK,
  TELEMETRY_KO
};
extern uint8_t telemetryState;

#define TELEMETRY_TIMEOUT10ms          100 // 1 second
#define TELEMETRY_SERIAL_DEFAULT       0
#define TELEMETRY_SERIAL_8E2           1
#define TELEMETRY_SERIAL_WITHOUT_DMA   2
#define TELEMETRY_OUTPUT_FIFO_SIZE     16
#define TELEMETRY_AVERAGE_COUNT        3

#if defined(CROSSFIRE) || defined(MULTIMODULE)
#define TELEMETRY_RX_PACKET_SIZE       128
// multi module Spektrum telemetry is 18 bytes, FlySky is 37 bytes
#else
#define TELEMETRY_RX_PACKET_SIZE       19  // 9 bytes (full packet), worst case 18 bytes with byte-stuffing (+1)
#endif

extern uint8_t telemetryRxBuffer[TELEMETRY_RX_PACKET_SIZE];
extern uint8_t telemetryRxBufferCount;
extern uint8_t outputTelemetryBuffer[TELEMETRY_OUTPUT_FIFO_SIZE] __DMA;
extern uint8_t outputTelemetryBufferSize;
extern uint8_t outputTelemetryBufferTrigger;
extern uint8_t telemetryProtocol;


enum {
  TELEM_CELL_INDEX_LOWEST,
  TELEM_CELL_INDEX_1,
  TELEM_CELL_INDEX_2,
  TELEM_CELL_INDEX_3,
  TELEM_CELL_INDEX_4,
  TELEM_CELL_INDEX_5,
  TELEM_CELL_INDEX_6,
  TELEM_CELL_INDEX_HIGHEST,
  TELEM_CELL_INDEX_DELTA,
};

PACK(struct CellValue
{
  uint16_t value:15;
  uint16_t state:1;

  void set(uint16_t value)
  {
    if (value > 50) {
      this->value = value;
      this->state = 1;
    }
  }
});


#define TELEMETRY_STREAMING()           (telemetryData.rssi.value > 0)
#define TELEMETRY_RSSI()                (telemetryData.rssi.value)
#define TELEMETRY_RSSI_MIN()            (telemetryData.rssi.min)

#define TELEMETRY_CELL_VOLTAGE_MUTLIPLIER  1

#define TELEMETRY_GPS_SPEED_BP          telemetryData.hub.gpsSpeed_bp
#define TELEMETRY_GPS_SPEED_AP          telemetryData.hub.gpsSpeed_ap

#define TELEMETRY_ABSOLUTE_GPS_ALT      (telemetryData.hub.gpsAltitude)
#define TELEMETRY_RELATIVE_GPS_ALT      (telemetryData.hub.gpsAltitude + telemetryData.hub.gpsAltitudeOffset)
#define TELEMETRY_RELATIVE_GPS_ALT_BP   (TELEMETRY_RELATIVE_GPS_ALT / 100)

#define TELEMETRY_RELATIVE_BARO_ALT_BP  (telemetryData.hub.baroAltitude / 100)
#define TELEMETRY_RELATIVE_BARO_ALT_AP  (telemetryData.hub.baroAltitude % 100)

#define TELEMETRY_BARO_ALT_PREPARE()    div_t baroAltitudeDivision = div(getConvertedTelemetryValue(telemetryData.hub.baroAltitude, UNIT_DIST), 100)
#define TELEMETRY_BARO_ALT_FORMAT       "%c%d.%02d,"
#define TELEMETRY_BARO_ALT_ARGS         telemetryData.hub.baroAltitude < 0 ? '-' : ' ', abs(baroAltitudeDivision.quot), abs(baroAltitudeDivision.rem),
#define TELEMETRY_GPS_ALT_FORMAT        "%c%d.%02d,"
#define TELEMETRY_GPS_ALT_ARGS          telemetryData.hub.gpsAltitude < 0 ? '-' : ' ', abs(telemetryData.hub.gpsAltitude / 100), abs(telemetryData.hub.gpsAltitude % 100),
#define TELEMETRY_SPEED_UNIT            (IS_IMPERIAL_ENABLE() ? SPEED_UNIT_IMP : SPEED_UNIT_METR)
#define TELEMETRY_GPS_SPEED_FORMAT      "%d,"
#define TELEMETRY_GPS_SPEED_ARGS        telemetryData.hub.gpsSpeed_bp,

#define TELEMETRY_CELLS_ARGS          telemetryData.hub.cellsSum / 10, telemetryData.hub.cellsSum % 10, TELEMETRY_CELL_VOLTAGE(0)/100, TELEMETRY_CELL_VOLTAGE(0)%100, TELEMETRY_CELL_VOLTAGE(1)/100, TELEMETRY_CELL_VOLTAGE(1)%100, TELEMETRY_CELL_VOLTAGE(2)/100, TELEMETRY_CELL_VOLTAGE(2)%100, TELEMETRY_CELL_VOLTAGE(3)/100, TELEMETRY_CELL_VOLTAGE(3)%100, TELEMETRY_CELL_VOLTAGE(4)/100, TELEMETRY_CELL_VOLTAGE(4)%100, TELEMETRY_CELL_VOLTAGE(5)/100, TELEMETRY_CELL_VOLTAGE(5)%100, TELEMETRY_CELL_VOLTAGE(6)/100, TELEMETRY_CELL_VOLTAGE(6)%100, TELEMETRY_CELL_VOLTAGE(7)/100, TELEMETRY_CELL_VOLTAGE(7)%100, TELEMETRY_CELL_VOLTAGE(8)/100, TELEMETRY_CELL_VOLTAGE(8)%100, TELEMETRY_CELL_VOLTAGE(9)/100, TELEMETRY_CELL_VOLTAGE(9)%100, TELEMETRY_CELL_VOLTAGE(10)/100, TELEMETRY_CELL_VOLTAGE(10)%100, TELEMETRY_CELL_VOLTAGE(11)/100, TELEMETRY_CELL_VOLTAGE(11)%100,
#define TELEMETRY_CELLS_FORMAT        "%d.%d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,%d.%02d,"
#define TELEMETRY_CELLS_LABEL         "Cell volts,Cell 1,Cell 2,Cell 3,Cell 4,Cell 5,Cell 6,Cell 7,Cell 8,Cell 9,Cell 10,Cell 11,Cell 12,"

#define TELEMETRY_CURRENT_FORMAT        "%d.%d,"
#define TELEMETRY_CURRENT_ARGS          telemetryData.hub.current / 10, telemetryData.hub.current % 10,
#define TELEMETRY_VFAS_FORMAT           "%d.%d,"
#define TELEMETRY_VFAS_ARGS             telemetryData.hub.vfas / 10, telemetryData.hub.vfas % 10,
#define TELEMETRY_VSPEED_FORMAT         "%c%d.%02d,"
#define TELEMETRY_VSPEED_ARGS           telemetryData.hub.varioSpeed < 0 ? '-' : ' ', abs(telemetryData.hub.varioSpeed / 100), abs(telemetryData.hub.varioSpeed % 100),
#define TELEMETRY_ASPEED_FORMAT         "%d.%d,"
#define TELEMETRY_ASPEED_ARGS           telemetryData.hub.airSpeed / 10, telemetryData.hub.airSpeed % 10,
#define TELEMETRY_OPENXSENSOR()         (0)

#define TELEMETRY_CELL_VOLTAGE(k)         (telemetryData.hub.cellVolts[k] * TELEMETRY_CELL_VOLTAGE_MUTLIPLIER)
#define TELEMETRY_MIN_CELL_VOLTAGE        (telemetryData.hub.minCellVolts * TELEMETRY_CELL_VOLTAGE_MUTLIPLIER)

#define IS_DISTANCE_UNIT(unit)         ((unit) == UNIT_METERS || (unit) == UNIT_FEET)
#define IS_SPEED_UNIT(unit)            ((unit) >= UNIT_KTS && (unit) <= UNIT_MPH)

#define IS_FRSKY_D_PROTOCOL()          (telemetryProtocol == PROTOCOL_FRSKY_D)
#if defined (MULTIMODULE)
#define IS_D16_MULTI()                 ((g_model.moduleData[EXTERNAL_MODULE].getMultiProtocol(false) == MM_RF_PROTO_FRSKY) && (g_model.moduleData[EXTERNAL_MODULE].subType == MM_RF_FRSKY_SUBTYPE_D16 || g_model.moduleData[EXTERNAL_MODULE].subType == MM_RF_FRSKY_SUBTYPE_D16_8CH))
#define IS_FRSKY_SPORT_PROTOCOL()      (telemetryProtocol == PROTOCOL_FRSKY_SPORT || (telemetryProtocol == PROTOCOL_MULTIMODULE && IS_D16_MULTI()))
#else
#define IS_FRSKY_SPORT_PROTOCOL()      (telemetryProtocol == PROTOCOL_FRSKY_SPORT)
#endif
#define IS_SPEKTRUM_PROTOCOL()         (telemetryProtocol == PROTOCOL_SPEKTRUM)

inline uint8_t modelTelemetryProtocol()
{
#if defined(CROSSFIRE)
  if (g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_CROSSFIRE) {
    return PROTOCOL_PULSES_CROSSFIRE;
  }
#endif
     
  if (!IS_INTERNAL_MODULE_ENABLED() && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_PPM) {
    return g_model.telemetryProtocol;
  }
  
#if defined(MULTIMODULE)
  if (!IS_INTERNAL_MODULE_ENABLED() && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_MULTIMODULE) {
    return PROTOCOL_MULTIMODULE;
  }
#endif

#if defined(PCBI6X)
  if (IS_INTERNAL_MODULE_ENABLED()) {
    return PROTOCOL_FLYSKY_IBUS;
  }
#endif
  // default choice
  return PROTOCOL_FRSKY_SPORT;
}

#if defined(LOG_TELEMETRY) && !defined(SIMU)
void logTelemetryWriteStart();
void logTelemetryWriteByte(uint8_t data);
#define LOG_TELEMETRY_WRITE_START()    logTelemetryWriteStart()
#define LOG_TELEMETRY_WRITE_BYTE(data) logTelemetryWriteByte(data)
#else
#define LOG_TELEMETRY_WRITE_START()
#define LOG_TELEMETRY_WRITE_BYTE(data)
#endif



inline void telemetryOutputPushByte(uint8_t byte)
{
  outputTelemetryBuffer[outputTelemetryBufferSize++] = byte;
}

inline void telemetryOutputSetTrigger(uint8_t byte)
{
  outputTelemetryBufferTrigger = byte;
}

#if defined(LUA)
#define LUA_TELEMETRY_INPUT_FIFO_SIZE  256
extern Fifo<uint8_t, LUA_TELEMETRY_INPUT_FIFO_SIZE> * luaInputTelemetryFifo;
#endif

#if defined(STM32)
  #if defined(PCBI6X)
    #define IS_TELEMETRY_INTERNAL_MODULE() (true)
  #else
    #define IS_TELEMETRY_INTERNAL_MODULE() (g_model.moduleData[INTERNAL_MODULE].type == MODULE_TYPE_XJT)
  #endif
#else
  #define IS_TELEMETRY_INTERNAL_MODULE() (false)
#endif

// Module pulse synchronization
struct ModuleSyncStatus
{
  // feedback input: last received values
  uint16_t  refreshRate; // in us
  int16_t   inputLag;    // in us

  tmr10ms_t lastUpdate;  // in 10ms
  int16_t   currentLag;  // in us
  
  inline bool isValid() {
    // 2 seconds
    return (get_tmr10ms() - lastUpdate < 200);
  }

  // Set feedback from RF module
  void update(uint16_t newRefreshRate, int16_t newInputLag);

  // Get computed settings for scheduler
  uint16_t getAdjustedRefreshRate();

  // Status string for the UI
  void getRefreshString(char* refreshText);

  ModuleSyncStatus();
};

ModuleSyncStatus& getModuleSyncStatus(uint8_t moduleIdx);

#endif // _TELEMETRY_H_

