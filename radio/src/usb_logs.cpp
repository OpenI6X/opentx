#include "opentx.h"

#include "usb_logs.h"
#include "serial.h"

uint8_t logDelay = 0;

static tmr10ms_t lastUsbLogTime = 0;
static bool usbHeaderSent = false;

static inline int8_t getSwitchLogState(uint8_t idx)
{
  if (IS_CONFIG_3POS(idx))
    return switchState(idx * 3) ? -1 : (switchState(idx * 3 + 2) ? 1 : 0);
  else
    return switchState(idx * 3) ? -1 : 1;
}

uint32_t getLogicalSwitchesStates(uint8_t first)
{
  uint32_t result = 0;
  for (uint32_t i = first; i < MAX_LOGICAL_SWITCHES; i++) {
    result |= (getSwitch(SWSRC_FIRST_LOGICAL_SWITCH + i) << i);
  }
  return result;
}

static void usbLogsWriteHeader()
{
  serialPrintf("Date,Time,");

  char label[TELEM_LABEL_LEN+7];
  for (int i = 0; i < MAX_TELEMETRY_SENSORS; i++) {
    if (isTelemetryFieldAvailable(i)) {
      TelemetrySensor & sensor = g_model.telemetrySensors[i];
      if (sensor.logs) {
        memset(label, 0, sizeof(label));
        zchar2str(label, sensor.label, TELEM_LABEL_LEN);
        uint8_t unit = sensor.unit;
        if (unit == UNIT_CELLS) unit = UNIT_VOLTS;
        if (UNIT_RAW < unit && unit < UNIT_FIRST_VIRTUAL) {
          strcat(label, "(");
          strncat(label, STR_VTELEMUNIT+1+3*unit, 3);
          strcat(label, ")");
        }
        strcat(label, ",");
        serialPrintf("%s", label);
      }
    }
  }

  for (uint32_t i = 1; i < NUM_STICKS+NUM_POTS + NUM_SLIDERS + 1; i++) {
    const char * p = STR_VSRCRAW + i * STR_VSRCRAW[0] + 2;
    for (uint8_t j = 0; j < STR_VSRCRAW[0] - 1; ++j) {
      if (!*p) break;
      serialPutc(*p++);
    }
    serialPutc(',');
  }

  serialPrintf("SA,SB,SC,SD,SE,SF,LSW,TxBat(V)\n");
//   for (int i = 0; i < NUM_SWITCHES; i++) {
//    if (SWITCH_EXISTS(i)) {
//      serialPrintf("S%c,", 'A' + i);
//    }
//  }
//  serialPrintf("LSW,TxBat(V)\n");
}

void usbLogsInit()
{
  lastUsbLogTime = 0;
  usbHeaderSent = false;
}

void usbLogsWrite()
{
  if (!isFunctionActive(FUNCTION_LOGS) || logDelay == 0) {
    usbHeaderSent = false;
    return;
  }

  if (isFunctionActive(FUNCTION_LOGS) && logDelay > 0) {
    tmr10ms_t tmr10ms = get_tmr10ms();
    if (lastUsbLogTime != 0 && (tmr10ms_t)(tmr10ms - lastUsbLogTime) < (tmr10ms_t)logDelay * 10) {
      return;
    }
    lastUsbLogTime = tmr10ms;

    if (!usbHeaderSent) {
      usbLogsWriteHeader();
      usbHeaderSent = true;
    }

    uint8_t hours = tmr10ms / 100 / 60 / 60;
    uint8_t minutes = (tmr10ms / 100 / 60) % 60;
    uint8_t seconds = (tmr10ms / 100) % 60;
    uint8_t g_ms100 = tmr10ms % 100;
    serialPrintf("2000-01-01,%02d:%02d:%02d.%02d0,", hours, minutes, seconds, g_ms100);

    for (int i = 0; i < MAX_TELEMETRY_SENSORS; i++) {
      if (isTelemetryFieldAvailable(i)) {
        TelemetrySensor & sensor = g_model.telemetrySensors[i];
        TelemetryItem & telemetryItem = telemetryItems[i];
        if (sensor.logs) {
          if (sensor.unit == UNIT_GPS) {
            if (telemetryItem.gps.longitude && telemetryItem.gps.latitude) {
              div_t qr = div((int)telemetryItem.gps.latitude, 1000000);
              if (telemetryItem.gps.latitude < 0) serialPutc('-');
              serialPrintf("%d.%06d ", abs(qr.quot), abs(qr.rem));
              qr = div((int)telemetryItem.gps.longitude, 1000000);
              if (telemetryItem.gps.longitude < 0) serialPutc('-');
              serialPrintf("%d.%06d,", abs(qr.quot), abs(qr.rem));
            } else {
              serialPutc(',');
            }
          // } else if (sensor.unit == UNIT_DATETIME) {
          //   serialPrintf("%4d-%02d-%02d %02d:%02d:%02d,", telemetryItem.datetime.year, telemetryItem.datetime.month, telemetryItem.datetime.day, telemetryItem.datetime.hour, telemetryItem.datetime.min, telemetryItem.datetime.sec);
          } else if (sensor.prec == 2) {
            div_t qr = div((int)telemetryItem.value, 100);
            if (telemetryItem.value < 0) serialPutc('-');
            serialPrintf("%d.%02d,", abs(qr.quot), abs(qr.rem));
          } else if (sensor.prec == 1) {
            div_t qr = div((int)telemetryItem.value, 10);
            if (telemetryItem.value < 0) serialPutc('-');
            serialPrintf("%d.%d,", abs(qr.quot), abs(qr.rem));
          } else {
            serialPrintf("%d,", telemetryItem.value);
          }
        }
      }
    }

    for (uint32_t i = 0; i < NUM_STICKS + NUM_POTS + NUM_SLIDERS; i++) {
      serialPrintf("%d,", calibratedAnalogs[i]);
    }

    for (int i = 0; i < NUM_SWITCHES; i++) {
  //    if (SWITCH_EXISTS(i)) {
        serialPrintf("%d,", getSwitchLogState(i));
  //    }
    }

    serialPrintf("0x%03X,", getLogicalSwitchesStates(0));

    div_t qr = div(g_vbat100mV, 10);
    serialPrintf("%d.%d\n", abs(qr.quot), abs(qr.rem));
  }
}
