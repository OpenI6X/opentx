#include "opentx.h"

#include "usb_logs.h"
#include "serial.h"
#include "strhelpers.h"

#define FAKE_RTCLOCK // some tools expect a date and time in the logs, so we fake it here

static void usbLogPuts(const char * s)
{
  while (*s)
    serialPutc(*s++);
}

static void usbLogPutsN(const char * s, size_t n)
{
  while (n-- && *s)
    serialPutc(*s++);
}

static void usbLogUnsigned(uint32_t value, uint8_t minDigits = 0, uint8_t radix = 10)
{
  char buf[12];
  char * end = strAppendUnsigned(buf, value, 0, radix);
  uint8_t len = (uint8_t)(end - buf);
  while (len < minDigits) {
    serialPutc('0');
    len++;
  }
  usbLogPuts(buf);
}

static void usbLogSigned(int32_t value, uint8_t minDigits = 0, uint8_t radix = 10)
{
  if (value < 0) {
    serialPutc('-');
    value = -value;
  }
  usbLogUnsigned((uint32_t)value, minDigits, radix);
}

uint8_t logDelay = 0;

static tmr10ms_t lastUsbLogTime = 0;
static bool usbHeaderSent = false;

static inline int8_t getSwitchLogState(uint8_t idx)
{
  return switchState(idx * 3) ? -1 : ((!IS_CONFIG_3POS(idx) || switchState(idx * 3 + 2)) ? 1 : 0);
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
#if defined(FAKE_RTCLOCK)
  usbLogPuts("Date,Time,");
#else
  usbLogPuts("Time,");
#endif

  char label[TELEM_LABEL_LEN + 6];
  for (int i = 0; i < MAX_TELEMETRY_SENSORS; i++) {
    if (isTelemetryFieldAvailable(i)) {
      TelemetrySensor & sensor = g_model.telemetrySensors[i];
      if (sensor.logs) {
        memset(label, 0, sizeof(label));
        zchar2str(label, sensor.label, TELEM_LABEL_LEN);
        usbLogPuts(label);
        uint8_t unit = sensor.unit;
        if (unit == UNIT_CELLS) unit = UNIT_VOLTS;
        if (UNIT_RAW < unit && unit < UNIT_FIRST_VIRTUAL) {
          const char * unitStr = STR_VTELEMUNIT + 1 + 3 * unit;
          serialPutc('(');
          usbLogPutsN(unitStr, 3);
          serialPutc(')');
        }
        serialPutc(',');
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

  usbLogPuts("SA,SB,SC,SD,SE,SF,LSW,");
  for (uint8_t channel = 0; channel < MAX_OUTPUT_CHANNELS; channel++) {
    usbLogPuts("CH");
    usbLogUnsigned(channel + 1);
    usbLogPuts("(us),");
  }
  usbLogPuts("TxBat(V)\n");
}

void usbLogsInit()
{
  lastUsbLogTime = 0;
  usbHeaderSent = false;
}

static void logValue(int32_t value, uint8_t prec)
{
  if (prec == 0) {
    usbLogSigned(value);
    return;
  }
  int32_t divisor = 1;
  for (uint8_t i = 0; i < prec; i++)
    divisor *= 10;
  div_t qr = div((int)value, (int)divisor);
  usbLogSigned(qr.quot);
  serialPutc('.');
  usbLogUnsigned((uint32_t)abs(qr.rem), prec);
}

#if defined(FAKE_RTCLOCK)
static void logTime(uint32_t currentTime)
{
  div_t qr = div(currentTime, 60);
  uint8_t seconds = qr.rem;
  qr = div(qr.quot, 60);
  usbLogUnsigned(qr.quot, 2); // hours
  serialPutc(':');
  usbLogUnsigned(qr.rem, 2); // minutes
  serialPutc(':');
  usbLogUnsigned(seconds, 2);
}
#endif

void usbLogsWrite()
{
  if (!isFunctionActive(FUNCTION_LOGS) || logDelay == 0) {
    usbHeaderSent = false;
    return;
  }

  tmr10ms_t tmr10ms = get_tmr10ms();
  if (lastUsbLogTime != 0 && (tmr10ms_t)(tmr10ms - lastUsbLogTime) < (tmr10ms_t)logDelay * 10) {
    return;
  }
  lastUsbLogTime = tmr10ms;

  if (!usbHeaderSent) {
    usbLogsWriteHeader();
    usbHeaderSent = true;
    return; // skip first log line after header to do not overload buffer
  }

#if defined(FAKE_RTCLOCK)
  usbLogPuts("2000-01-01,");
  logTime(g_eeGeneral.globalTimer + sessionTimer);
  serialPutc('.');
  usbLogUnsigned((tmr10ms + 500) % 100, 2); // ms
  usbLogPuts("0,");
#else
  usbLogUnsigned(tmr10ms);
  serialPutc(',');
#endif

  for (int i = 0; i < MAX_TELEMETRY_SENSORS; i++) {
    if (isTelemetryFieldAvailable(i)) {
      TelemetrySensor & sensor = g_model.telemetrySensors[i];
      TelemetryItem telemetryItem;

      if (sensor.logs) {
        if (TELEMETRY_STREAMING() && !telemetryItems[i].isOld())
          telemetryItem = telemetryItems[i];

        if (sensor.unit == UNIT_GPS) {
          if (telemetryItem.gps.longitude && telemetryItem.gps.latitude) {
            logValue((int)telemetryItem.gps.latitude, 6);
            serialPutc(' ');
            logValue((int)telemetryItem.gps.longitude, 6);
          }
        // } else if (sensor.unit == UNIT_DATETIME) {
        //   datetime logging disabled
        } else if (sensor.unit == UNIT_TEXT) {
          serialPutc('"');
          usbLogPuts(telemetryItem.text);
          serialPutc('"');
        } else { // numeric
          logValue(telemetryItem.value, sensor.prec);
        }
        serialPutc(',');
      }
    }
  }

  for (uint32_t i = 0; i < NUM_STICKS + NUM_POTS + NUM_SLIDERS; i++) {
    usbLogSigned(calibratedAnalogs[i]);
    serialPutc(',');
  }

  for (int i = 0; i < NUM_SWITCHES; i++) {
  //    if (SWITCH_EXISTS(i)) {
        usbLogSigned(getSwitchLogState(i));
        serialPutc(',');
  //    }
  }

  usbLogPuts("0x");
  usbLogUnsigned(getLogicalSwitchesStates(0), 3, 16);
  serialPutc(',');

  for (uint8_t channel = 0; channel < MAX_OUTPUT_CHANNELS; channel++) {
    usbLogSigned(PPM_CENTER + channelOutputs[channel] / 2); // in us
    serialPutc(',');
  }

  logValue(g_vbat100mV, 1);
  serialPutc('\n');
}
