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

// No include guards here, this file may be included many times in different namespaces
// i.e. BACKUP RAM Backup/Restore functions

#include <inttypes.h>

#include "board.h"
#include "dataconstants.h"
#include "definitions.h"

#if defined(PCBTARANIS)
#define N_TARANIS_FIELD(x)
#define TARANIS_FIELD(x) x;
#else
#define N_TARANIS_FIELD(x) x;
#define TARANIS_FIELD(x)
#endif

#if defined(PCBX9E)
#define TARANIS_PCBX9E_FIELD(x) x;
#else
#define TARANIS_PCBX9E_FIELD(x)
#endif

#if defined(PCBHORUS)
#define N_HORUS_FIELD(x)
#define HORUS_FIELD(x) x;
#else
#define N_HORUS_FIELD(x) x;
#define HORUS_FIELD(x)
#endif

#if defined(BACKUP)
#define NOBACKUP(...)
#else
#define NOBACKUP(...) __VA_ARGS__
#endif

#if defined(PCBTARANIS) || defined(PCBHORUS)
typedef uint16_t source_t;
#else
typedef uint8_t source_t;
#endif

/*
 * Mixer structure
 */

PACK(struct CurveRef {
  uint8_t type;
  int8_t value;
});

PACK(struct MixData {
  int16_t weight : 11;  // GV1=-1024, -GV1=1023
  uint16_t destCh : 5;
  uint16_t srcRaw : 10;  // srcRaw=0 means not used
  uint16_t carryTrim : 1;
  uint16_t mixWarn : 2;  // mixer warning
  uint16_t mltpx : 2;    // multiplex method: 0 means +=, 1 means *=, 2 means :=
  uint16_t spare : 1;
  int32_t offset : 14;
  int32_t swtch : 9;
  uint32_t flightModes : 9;
  CurveRef curve;
  uint8_t delayUp;
  uint8_t delayDown;
  uint8_t speedUp;
  uint8_t speedDown;
  NOBACKUP(char name[LEN_EXPOMIX_NAME]);
});

/*
 * Expo/Input structure
 */

PACK(struct ExpoData {
  uint16_t mode : 2;
  uint16_t scale : 14;
  uint16_t srcRaw : 10;
  int16_t carryTrim : 6;
  uint32_t chn : 5;
  int32_t swtch : 9;
  uint32_t flightModes : 9;
  int32_t weight : 8;
  int32_t spare : 1;
  NOBACKUP(char name[LEN_EXPOMIX_NAME]);
  int8_t offset;
  CurveRef curve;
});

/*
 * Limit structure
 */

PACK(struct LimitData {
  int32_t min : 11;
  int32_t max : 11;
  int32_t ppmCenter : 10;
  int16_t offset : 11;
  uint16_t symetrical : 1;
  uint16_t revert : 1;
  uint16_t spare : 3;
  int8_t curve;
  NOBACKUP(char name[LEN_CHANNEL_NAME]);
});

/*
 * LogicalSwitch structure
 */

PACK(struct LogicalSwitchData {
  uint8_t func;
  int32_t v1 : 10;
  int32_t v3 : 10;
  int32_t andsw : 9;       // TODO rename to xswtch
  uint32_t andswtype : 1;  // TODO rename to xswtchType (AND / OR)
  uint32_t spare : 2;      // anything else needed?
  int16_t v2;
  uint8_t delay;
  uint8_t duration;
});

/*
 * SpecialFunction structure
 */

#if defined(PCBTARANIS)
#define CFN_SPARE_TYPE int32_t
#else
#define CFN_SPARE_TYPE int16_t
#endif

PACK(struct CustomFunctionData {
  int16_t swtch : 9;
  uint16_t func : 7;
  PACK(union {
#if !defined(PCBI6X) // SDCARD + VOICE
    NOBACKUP(PACK(struct {
      char name[LEN_FUNCTION_NAME];
    }) play);
#endif
    PACK(struct {
      int16_t val;
      uint8_t mode;
      uint8_t param;
#if !defined(PCBI6X) // SDCARD + VOICE
      NOBACKUP(CFN_SPARE_TYPE spare);
#endif
    })
    all;

    NOBACKUP(PACK(struct {
      int32_t val1;
#if !defined(PCBI6X) // SDCARD + VOICE
      NOBACKUP(CFN_SPARE_TYPE val2);
#endif
    }) clear);
  });
  uint8_t active;
});

/*
 * FlightMode structure
 */

PACK(struct trim_t {
  int16_t value : 11;
  uint16_t mode : 5;
});

typedef int16_t gvar_t;

#if MAX_ROTARY_ENCODERS > 0
#define FLIGHT_MODE_ROTARY_ENCODERS_FIELD int16_t rotaryEncoders[MAX_ROTARY_ENCODERS];
#else
#define FLIGHT_MODE_ROTARY_ENCODERS_FIELD
#endif

PACK(struct FlightModeData {
  trim_t trim[NUM_TRIMS];
  NOBACKUP(char name[LEN_FLIGHT_MODE_NAME]);
  int16_t swtch : 9;  // swtch of phase[0] is not used
  int16_t spare : 7;
  uint8_t fadeIn;
  uint8_t fadeOut;
  FLIGHT_MODE_ROTARY_ENCODERS_FIELD
  gvar_t gvars[MAX_GVARS];
});

/*
 * Curve structure
 */

PACK(struct CurveData {
  uint8_t type : 1;
  uint8_t smooth : 1;
  int8_t points : 6;  // describes number of points - 5
  NOBACKUP(char name[LEN_CURVE_NAME]);
});

/*
 * GVar structure
 */

PACK(struct GVarData {
  NOBACKUP(char name[LEN_GVAR_NAME]);
  uint32_t min : 12;
  uint32_t max : 12;
  uint32_t popup : 1;
  uint32_t prec : 1;
  uint32_t unit : 2;
  uint32_t spare : 4;
});

/*
 * Timer structure
 */

PACK(struct TimerData {
  int32_t mode : 9;  // timer trigger source -> off, abs, stk, stk%, sw/!sw, !m_sw/!m_sw
  uint32_t start : 23;
  int32_t value : 24;
  uint32_t countdownBeep : 2;
  uint32_t minuteBeep : 1;
  uint32_t persistent : 2;
  int32_t countdownStart : 2;
  uint32_t direction : 1;
  NOBACKUP(char name[LEN_TIMER_NAME]);
});

/*
 * Swash Ring structure
 */

PACK(struct SwashRingData {
  uint8_t type;
  uint8_t value;
  uint8_t collectiveSource;
  uint8_t aileronSource;
  uint8_t elevatorSource;
  int8_t collectiveWeight;
  int8_t aileronWeight;
  int8_t elevatorWeight;
});

#if MAX_SCRIPTS > 0
union ScriptDataInput {
  int16_t value;
  source_t source;
};

PACK(struct ScriptData {
  char file[LEN_SCRIPT_FILENAME];
  char name[LEN_SCRIPT_NAME];
  ScriptDataInput inputs[MAX_SCRIPT_INPUTS];
});
#endif

/*
 * Frsky Telemetry structure
 */
PACK(struct RssiAlarmData {
  int8_t disabled : 1;
  int8_t spare : 1;
  int8_t warning : 6;
  int8_t spare2 : 2;
  int8_t critical : 6;
  inline int8_t getWarningRssi() { return 45 + warning; }
  inline int8_t getCriticalRssi() { return 42 + critical; }
});

typedef int16_t ls_telemetry_value_t;

#if !defined(COLORLCD)
PACK(struct FrSkyBarData {
  source_t source;
  ls_telemetry_value_t barMin;  // minimum for bar display
  ls_telemetry_value_t barMax;  // ditto for max display (would usually = ratio)
});

PACK(struct FrSkyLineData {
  source_t sources[NUM_LINE_ITEMS];
});

#if defined(PCBTARANIS)
PACK(struct TelemetryScriptData {
  char file[LEN_SCRIPT_FILENAME];
  int16_t inputs[MAX_TELEM_SCRIPT_INPUTS];
});
#endif

union FrSkyScreenData {
  FrSkyBarData bars[4];
  FrSkyLineData lines[4];
#if defined(PCBTARANIS)
  TelemetryScriptData script;
#endif
};
#endif

#if defined(COLORLCD)
PACK(struct FrSkyTelemetryData {  // TODO EEPROM change, rename to VarioData
  uint8_t varioSource : 7;
  uint8_t varioCenterSilent : 1;
  int8_t varioCenterMax;
  int8_t varioCenterMin;
  int8_t varioMin;
  int8_t varioMax;
});
#else
// TODO remove this also on Taranis
PACK(struct FrSkyTelemetryData {
  uint8_t voltsSource;
  uint8_t altitudeSource;
  uint8_t screensType;                             // 2bits per screen (None/Gauges/Numbers/Script)
  FrSkyScreenData screens[MAX_TELEMETRY_SCREENS];  // TODO EEPROM change should not be here anymore
  uint8_t varioSource : 7;
  uint8_t varioCenterSilent : 1;
  int8_t varioCenterMax;
  int8_t varioCenterMin;
  int8_t varioMin;
  int8_t varioMax;
});
#endif

/*
 * Telemetry Sensor structure
 */

PACK(struct TelemetrySensor {
  union {
    uint16_t id;  // data identifier, for FrSky we can reuse existing ones. Source unit is derived from type.
    NOBACKUP(uint16_t persistentValue);
  };
  union {
    uint8_t instance;  // instance ID to allow handling multiple instances of same value type, for FrSky can be the physical ID of the sensor
    NOBACKUP(uint8_t formula);
  };
  char label[TELEM_LABEL_LEN];  // user defined label
  uint8_t subId;
  uint8_t type : 1;             // 0=custom / 1=calculated
  uint8_t spare1:1;
  uint8_t unit : 6;             // user can choose what unit to display each value in
  uint8_t prec : 2;
  uint8_t autoOffset : 1;
  uint8_t filter : 1;
  uint8_t logs : 1;
  uint8_t persistent : 1;
  uint8_t onlyPositive : 1;
  uint8_t spare2:1;
  union {
    NOBACKUP(PACK(struct {
      uint16_t ratio;
      int16_t offset;
    }) custom);
    NOBACKUP(PACK(struct {
      uint8_t source;
      uint8_t index;
      uint16_t spare;
    }) cell);
    NOBACKUP(PACK(struct {
      int8_t sources[4];
    }) calc);
    NOBACKUP(PACK(struct {
      uint8_t source;
      uint8_t spare[3];
    }) consumption);
    NOBACKUP(PACK(struct {
      uint8_t gps;
      uint8_t alt;
      uint16_t spare;
    }) dist);
    uint32_t param;
  };
  NOBACKUP(
      void init(const char *label, uint8_t unit = UNIT_RAW, uint8_t prec = 0);
      void init(uint16_t id);
      bool isAvailable() const;
      int32_t getValue(int32_t value, uint8_t unit, uint8_t prec) const;
      bool isConfigurable() const;
      bool isPrecConfigurable() const;
      int32_t getPrecMultiplier() const;
      int32_t getPrecDivisor() const);
});

/*
 * Module structure
 */

// Only used in case switch and if statements as "virtual" protocol
#define MM_RF_CUSTOM_SELECTED 0xff

PACK(struct ModuleData {
  uint8_t type; // : 4; // use full byte to reduce code size on PCBI6X
  int8_t rfProtocol; //  : 4
  uint8_t channelsStart;
  int8_t channelsCount;      // 0=8 channels
  uint8_t failsafeMode : 4;  // only 3 bits used
  uint8_t subType : 3;
  uint8_t invertedSerial : 1;  // telemetry serial inverted from standard
  int16_t failsafeChannels[MAX_OUTPUT_CHANNELS];

  union {
    struct {
      int8_t delay : 6;
      uint8_t pulsePol : 1;
      uint8_t outputType : 1;  // false = open drain, true = push pull
      int8_t frameLength;
    } ppm;
    NOBACKUP(struct {
      uint8_t rfProtocolExtra : 2;
      uint8_t spare1 : 3;
      uint8_t customProto : 1;
      uint8_t autoBindMode : 1;
      uint8_t lowPowerMode : 1;
      int8_t optionValue;
    } multi);
    NOBACKUP(struct {
      uint8_t spare1 : 6;
      uint8_t noninverted : 1;
      uint8_t spare2 : 1;
      int8_t refreshRate;  // definition as framelength for ppm (* 5 + 225 = time in 1/10 ms)
    } sbus);
    NOBACKUP(struct {
      uint16_t servoFreq; // 50 - 400
    } afhds2a);
    NOBACKUP(struct {
      uint8_t crsfArmingMode:1;
      int16_t crsfArmingTrigger:10;
      int16_t spare1:5;
    } crsf);
  };

  // Helper functions to set both of the rfProto protocol at the same time
  NOBACKUP(inline uint8_t getMultiProtocol(bool returnCustom) {
    if (returnCustom && multi.customProto)
      return MM_RF_CUSTOM_SELECTED;
    return ((uint8_t)(rfProtocol & 0x0f)) + (multi.rfProtocolExtra << 4);
  })

  NOBACKUP(inline void setMultiProtocol(uint8_t proto) {
    rfProtocol = (uint8_t)(proto & 0x0f);
    multi.rfProtocolExtra = (proto & 0x30) >> 4;
  })
});

/*
 * Model structure
 */

typedef uint16_t BeepANACenter;

#if LEN_BITMAP_NAME > 0
#define MODEL_HEADER_BITMAP_FIELD NOBACKUP(char bitmap[LEN_BITMAP_NAME]);
#else
#define MODEL_HEADER_BITMAP_FIELD
#endif

PACK(struct ModelHeader {
  char name[LEN_MODEL_NAME];  // must be first for eeLoadModelName
  uint8_t modelId[NUM_MODULES];
  MODEL_HEADER_BITMAP_FIELD
});

#if defined(COLORLCD)
typedef uint16_t swconfig_t;
typedef uint32_t swarnstate_t;
#elif defined(PCBX9E)
typedef uint64_t swconfig_t;
typedef uint64_t swarnstate_t;
typedef uint32_t swarnenable_t;
#elif defined(PCBTARANIS)
typedef uint16_t swconfig_t;
typedef uint16_t swarnstate_t;
typedef uint8_t swarnenable_t;
#elif defined(PCBI6X)
typedef uint8_t swconfig_t;
typedef uint8_t swarnstate_t;
typedef uint8_t swarnenable_t;
#else
typedef uint8_t swarnstate_t;
typedef uint8_t swarnenable_t;
#endif

#if defined(COLORLCD)
#define SWITCHES_WARNING_DATA \
  NOBACKUP(swarnstate_t switchWarningState);
#else
#define SWITCHES_WARNING_DATA      \
  swarnstate_t switchWarningState; \
  swarnenable_t switchWarningEnable;
#endif

#define MODEL_GVARS_DATA GVarData gvars[MAX_GVARS];
#define TELEMETRY_DATA                \
  NOBACKUP(FrSkyTelemetryData frsky); \
  NOBACKUP(RssiAlarmData rssiAlarms);

#if defined(PCBHORUS)
#include "gui/480x272/layout.h"
#include "gui/480x272/topbar.h"
#define LAYOUT_NAME_LEN 10
PACK(struct CustomScreenData {
  char layoutName[LAYOUT_NAME_LEN];
  Layout::PersistentData layoutData;
});
#define CUSTOM_SCREENS_DATA                                  \
  NOBACKUP(CustomScreenData screenData[MAX_CUSTOM_SCREENS]); \
  NOBACKUP(Topbar::PersistentData topbarData);               \
  NOBACKUP(uint8_t view);
#elif defined(PCBTARANIS)
#define CUSTOM_SCREENS_DATA \
  NOBACKUP(uint8_t view);
#else
#define CUSTOM_SCREENS_DATA
// TODO other boards could have their custom screens here as well
#endif

#if defined(PCBX12S)
#define MODELDATA_EXTRA                                  \
  NOBACKUP(uint8_t spare : 3);                           \
  NOBACKUP(uint8_t trainerMode : 3);                     \
  NOBACKUP(uint8_t potsWarnMode : 2);                    \
  ModuleData moduleData[NUM_MODULES + 1];                \
  NOBACKUP(ScriptData scriptsData[MAX_SCRIPTS]);         \
  NOBACKUP(char inputNames[MAX_INPUTS][LEN_INPUT_NAME]); \
  NOBACKUP(uint8_t potsWarnEnabled);                     \
  NOBACKUP(int8_t potsWarnPosition[NUM_POTS + NUM_SLIDERS]);
#elif defined(PCBX10)
#define MODELDATA_EXTRA                                      \
  NOBACKUP(uint8_t spare : 3);                               \
  NOBACKUP(uint8_t trainerMode : 3);                         \
  NOBACKUP(uint8_t potsWarnMode : 2);                        \
  ModuleData moduleData[NUM_MODULES + 1];                    \
  NOBACKUP(ScriptData scriptsData[MAX_SCRIPTS]);             \
  NOBACKUP(char inputNames[MAX_INPUTS][LEN_INPUT_NAME]);     \
  NOBACKUP(uint8_t potsWarnEnabled);                         \
  NOBACKUP(int8_t potsWarnPosition[NUM_POTS + NUM_SLIDERS]); \
  NOBACKUP(uint8_t potsWarnSpares[NUM_DUMMY_ANAS]);
#elif defined(PCBTARANIS)
#define MODELDATA_EXTRA                        \
  uint8_t spare : 3;                           \
  uint8_t trainerMode : 3;                     \
  uint8_t potsWarnMode : 2;                    \
  ModuleData moduleData[NUM_MODULES + 1];      \
  ScriptData scriptsData[MAX_SCRIPTS];         \
  char inputNames[MAX_INPUTS][LEN_INPUT_NAME]; \
  uint8_t potsWarnEnabled;                     \
  int8_t potsWarnPosition[NUM_POTS + NUM_SLIDERS];
#elif defined(PCBI6X)
#define MODELDATA_EXTRA                            \
  NOBACKUP(uint8_t spare : 3);                     \
  NOBACKUP(uint8_t trainerMode : 3);               \
  uint8_t potsWarnMode : 2;                        \
  ModuleData moduleData[NUM_MODULES + 1];          \
  char inputNames[MAX_INPUTS][LEN_INPUT_NAME];     \
  uint8_t potsWarnEnabled;                         \
  int8_t potsWarnPosition[NUM_POTS + NUM_SLIDERS]; \
  uint8_t rxBattAlarms[2];
#else
#define MODELDATA_EXTRA
#endif

PACK(struct ModelData {
  ModelHeader header;
  TimerData timers[MAX_TIMERS];
  uint8_t telemetryProtocol : 3;
  uint8_t thrTrim : 1;  // Enable Throttle Trim
  uint8_t noGlobalFunctions : 1;
  uint8_t displayTrims : 2;
  uint8_t ignoreSensorIds : 1;
  int8_t trimInc : 3;  // Trim Increments
  uint8_t disableThrottleWarning : 1;
  uint8_t displayChecklist : 1;
  uint8_t extendedLimits : 1;
  uint8_t extendedTrims : 1;
  uint8_t throttleReversed : 1;
  BeepANACenter beepANACenter;
  MixData mixData[MAX_MIXERS];
  LimitData limitData[MAX_OUTPUT_CHANNELS];
  ExpoData expoData[MAX_EXPOS];

  CurveData curves[MAX_CURVES];
  int8_t points[MAX_CURVE_POINTS];

  LogicalSwitchData logicalSw[MAX_LOGICAL_SWITCHES];
  CustomFunctionData customFn[MAX_SPECIAL_FUNCTIONS];
  SwashRingData swashR;
  FlightModeData flightModeData[MAX_FLIGHT_MODES];

  NOBACKUP(uint8_t thrTraceSrc);

  SWITCHES_WARNING_DATA

  MODEL_GVARS_DATA

  TELEMETRY_DATA

  MODELDATA_EXTRA

  NOBACKUP(TelemetrySensor telemetrySensors[MAX_TELEMETRY_SENSORS];)

  TARANIS_PCBX9E_FIELD(uint8_t toplcdTimer)

  CUSTOM_SCREENS_DATA
});

/*
 * Radio structure
 */

#if XPOTS_MULTIPOS_COUNT > 0
PACK(struct StepsCalibData {
  uint8_t count;
  uint8_t steps[XPOTS_MULTIPOS_COUNT - 1];
});
#endif

PACK(struct CalibData {
  int16_t mid;
  int16_t spanNeg;
  int16_t spanPos;
});

PACK(struct TrainerMix {
  uint8_t srcChn : 6;  // 0-7 = ch1-8
  uint8_t mode : 2;    // off,add-mode,subst-mode
  int8_t studWeight;
});

PACK(struct TrainerData {
  int16_t calib[4];
  NOBACKUP(TrainerMix mix[4]);
});

#define EXTRA_GENERAL_FIELDS_ARM                       \
  NOBACKUP(uint8_t backlightBright);                   \
  NOBACKUP(uint32_t globalTimer);                      \
  NOBACKUP(uint8_t bluetoothBaudrate : 4);             \
  NOBACKUP(uint8_t bluetoothMode : 4);                 \
  NOBACKUP(uint8_t countryCode);                       \
  NOBACKUP(uint8_t imperial : 1);                      \
  NOBACKUP(uint8_t jitterFilter : 1); /* 0 - active */ \
  NOBACKUP(uint8_t disableRssiPoweroffAlarm : 1);      \
  NOBACKUP(uint8_t USBMode : 2);                       \
  NOBACKUP(uint8_t spare : 1);                         \
  NOBACKUP(uint8_t ppmunit : 2);                       \
  NOBACKUP(char ttsLanguage[2]);                       \
  NOBACKUP(int8_t beepVolume : 4);                     \
  NOBACKUP(int8_t wavVolume : 4);                      \
  NOBACKUP(int8_t varioVolume : 4);                    \
  NOBACKUP(int8_t backgroundVolume : 4);               \
  NOBACKUP(int8_t varioPitch);                         \
  NOBACKUP(int8_t varioRange);                         \
  NOBACKUP(int8_t varioRepeat);                        \
  CustomFunctionData customFn[MAX_SPECIAL_FUNCTIONS];

#if defined(PCBHORUS)
#define EXTRA_GENERAL_FIELDS                                                                   \
  EXTRA_GENERAL_FIELDS_ARM                                                                     \
  NOBACKUP(uint8_t auxSerialMode : 4);                                                           \
  uint8_t slidersConfig : 4;                                                                   \
  uint32_t switchConfig;                                                                       \
  uint8_t potsConfig; /* two bits per pot */                                                   \
  NOBACKUP(char switchNames[NUM_SWITCHES][LEN_SWITCH_NAME]);                                   \
  NOBACKUP(char anaNames[NUM_STICKS + NUM_POTS + NUM_SLIDERS + NUM_DUMMY_ANAS][LEN_ANA_NAME]); \
  NOBACKUP(char currModelFilename[LEN_MODEL_FILENAME + 1]);                                    \
  NOBACKUP(uint8_t spare : 1);                                                                 \
  NOBACKUP(uint8_t blOffBright : 7);                                                           \
  NOBACKUP(char bluetoothName[LEN_BLUETOOTH_NAME]);
#elif defined(PCBTARANIS) || defined(PCBI6X)
#if defined(BLUETOOTH)
#define BLUETOOTH_FIELDS \
  uint8_t spare;         \
  char bluetoothName[LEN_BLUETOOTH_NAME];
#else
#define BLUETOOTH_FIELDS
#endif
#define EXTRA_GENERAL_FIELDS                                        \
  EXTRA_GENERAL_FIELDS_ARM                                          \
  uint8_t auxSerialMode : 4;                                        \
  uint8_t slidersConfig : 4;                                        \
  uint8_t potsConfig; /* two bits per pot */                        \
  swarnstate_t switchUnlockStates;                                  \
  swconfig_t switchConfig;                                          \
  char switchNames[NUM_SWITCHES][LEN_SWITCH_NAME];                  \
  char anaNames[NUM_STICKS + NUM_POTS + NUM_SLIDERS][LEN_ANA_NAME]; \
  uint8_t receiverId[16][4]; /* AFHDS2A RxNum */                    \
  BLUETOOTH_FIELDS
#endif

#if defined(PCBHORUS)
#include "gui/480x272/theme.h"
#define THEME_NAME_LEN 8
#define THEME_DATA                          \
  NOBACKUP(char themeName[THEME_NAME_LEN]); \
  NOBACKUP(Theme::PersistentData themeData);
#else
#define THEME_DATA
#endif

#if defined(BUZZER)
#define BUZZER_FIELD NOBACKUP(int8_t buzzerMode : 2);  // -2=quiet, -1=only alarms, 0=no keys, 1=all (only used on AVR radios without audio hardware)
#else
#define BUZZER_FIELD NOBACKUP(int8_t spareRadio : 2);
#endif

PACK(struct RadioData {
  NOBACKUP(uint8_t version);
  NOBACKUP(uint16_t variant); // not used on PCBI6X, reuse for swconfig_t?
  CalibData calib[NUM_CALIBRATED_ANALOGS];
  NOBACKUP(uint16_t chkSum);
  N_HORUS_FIELD(int8_t currModel);
  N_HORUS_FIELD(uint8_t contrast);
  NOBACKUP(uint8_t vBatWarn);
  NOBACKUP(int8_t txVoltageCalibration);
  NOBACKUP(int8_t backlightMode);
  NOBACKUP(TrainerData trainer);
  NOBACKUP(uint8_t view);  // index of view in main screen
  BUZZER_FIELD
  NOBACKUP(uint8_t fai : 1);
  NOBACKUP(int8_t beepMode : 2);  // -2=quiet, -1=only alarms, 0=no keys, 1=all
  NOBACKUP(uint8_t alarmsFlash : 1);
  NOBACKUP(uint8_t disableMemoryWarning : 1);
  NOBACKUP(uint8_t disableAlarmWarning : 1);
  uint8_t stickMode : 2;
  NOBACKUP(int8_t timezone : 5);
  NOBACKUP(uint8_t adjustRTC : 1);
  NOBACKUP(uint8_t inactivityTimer);
  uint8_t telemetryBaudrate : 3;
  int8_t splashMode : 3;
  NOBACKUP(int8_t hapticMode : 2);  // -2=quiet, -1=only alarms, 0=no keys, 1=all
  int8_t switchesDelay;
  NOBACKUP(uint8_t lightAutoOff);
  NOBACKUP(uint8_t templateSetup);  // RETA order for receiver channels
  NOBACKUP(int8_t PPM_Multiplier);
  NOBACKUP(int8_t hapticLength);
  N_HORUS_FIELD(N_TARANIS_FIELD(uint8_t reNavigation));
  N_HORUS_FIELD(N_TARANIS_FIELD(uint8_t stickReverse));
  NOBACKUP(int8_t beepLength : 3);
  NOBACKUP(int8_t hapticStrength : 3);
  NOBACKUP(uint8_t gpsFormat : 1);
  NOBACKUP(uint8_t unexpectedShutdown : 1);
  NOBACKUP(uint8_t speakerPitch);
  NOBACKUP(int8_t speakerVolume);
  NOBACKUP(int8_t vBatMin);
  NOBACKUP(int8_t vBatMax);

  EXTRA_GENERAL_FIELDS

  THEME_DATA
});

#undef SWITCHES_WARNING_DATA
#undef MODEL_GVARS_DATA
#undef TELEMETRY_DATA
#undef MODELDATA_EXTRA
#undef CUSTOM_SCREENS_DATA
#undef EXTRA_GENERAL_FIELDS
#undef THEME_DATA
#undef NOBACKUP

#if !defined(BACKUP)
/* Compile time check to test structure size has not changed *
   Changing the size of one of the eeprom structs may cause wrong data to
   be loaded. Error out if the struct size changes.
   This function tries not avoid checking or using the defines
   other than the CPU arch and board type so changes in other
   defines also trigger the struct size changes */

template <typename ToCheck, size_t expectedSize, size_t realSize = sizeof(ToCheck)>
void check_size() {
  static_assert(expectedSize == realSize, "struct size changed");
}

static inline void check_struct() {
#define CHKSIZE(x, y) check_size<struct x, y>()
#define CHKTYPE(x, y) check_size<x, y>()

  CHKSIZE(CurveRef, 2);

  /* Difference between Taranis/Horus is LEN_EXPOMIX_NAME */
  /* LEN_FUNCTION_NAME is the difference in CustomFunctionData */

#if defined(PCBX7) || defined(PCBXLITE)
  CHKSIZE(MixData, 20);
  CHKSIZE(ExpoData, 17);
  CHKSIZE(LimitData, 11);
  CHKSIZE(LogicalSwitchData, 9);
  CHKSIZE(CustomFunctionData, 11);
  CHKSIZE(FlightModeData, 28 + 2 * NUM_TRIMS);
  CHKSIZE(TimerData, 11);
  CHKSIZE(SwashRingData, 8);
  CHKSIZE(FrSkyBarData, 6);
  CHKSIZE(FrSkyLineData, 4);
  CHKTYPE(union FrSkyScreenData, 24);
  CHKSIZE(FrSkyTelemetryData, 104);
  CHKSIZE(ModelHeader, 12);
  CHKSIZE(CurveData, 4);
#elif defined(PCBTARANIS)
  CHKSIZE(MixData, 22);
  CHKSIZE(ExpoData, 19);
  CHKSIZE(LimitData, 13);
  CHKSIZE(LogicalSwitchData, 9);
  CHKSIZE(CustomFunctionData, 11);
  CHKSIZE(FlightModeData, 40);
  CHKSIZE(TimerData, 16);
  CHKSIZE(SwashRingData, 8);
  CHKSIZE(FrSkyBarData, 6);
  CHKSIZE(FrSkyLineData, 6);
  CHKTYPE(union FrSkyScreenData, 24);
  CHKSIZE(FrSkyTelemetryData, 104);
  CHKSIZE(ModelHeader, 24);
  CHKSIZE(CurveData, 4);
#elif defined(PCBHORUS)
  CHKSIZE(MixData, 20);
  CHKSIZE(ExpoData, 17);
  CHKSIZE(LimitData, 13);
  CHKSIZE(CustomFunctionData, 9);
  CHKSIZE(FlightModeData, 44);
  CHKSIZE(TimerData, 16);
  CHKSIZE(SwashRingData, 8);
  CHKSIZE(FrSkyTelemetryData, 5);
  CHKSIZE(ModelHeader, 27);
  CHKSIZE(CurveData, 4);
  CHKSIZE(CustomScreenData, 610);
  CHKSIZE(Topbar::PersistentData, 216);
#elif defined(PCBI6X)
  CHKSIZE(LimitData, 11);
  CHKSIZE(MixData, 20);
  CHKSIZE(ExpoData, 17);
  CHKSIZE(CustomFunctionData, 7);
  CHKSIZE(FlightModeData, 36);
  CHKSIZE(TimerData, 11);
  CHKSIZE(SwashRingData, 8);
  CHKSIZE(FrSkyBarData, 5);
  CHKSIZE(FrSkyLineData, 2);
  CHKSIZE(FrSkyTelemetryData, 88);
  CHKSIZE(ModelHeader, 12);
  CHKTYPE(CurveData, 4);
#else
  // Common for all variants
  CHKSIZE(LimitData, 5);
  CHKSIZE(SwashRingData, 3);
  CHKSIZE(FrSkyBarData, 3);
  CHKSIZE(FrSkyLineData, 2);
  CHKSIZE(FrSkyTelemetryData, 43);
  CHKSIZE(ModelHeader, 11);
  CHKTYPE(CurveData, 1);

  CHKSIZE(MixData, 9);
  CHKSIZE(ExpoData, 4);

  CHKSIZE(CustomFunctionData, 3);
  CHKSIZE(TimerData, 3);

  CHKSIZE(FlightModeData, 30);
  CHKSIZE(RadioData, 85);

#endif /* board specific ifdefs*/

  CHKSIZE(LogicalSwitchData, 9);
  CHKSIZE(TelemetrySensor, 14);

  CHKSIZE(ModuleData, 7 + (2 * MAX_OUTPUT_CHANNELS));

  CHKSIZE(GVarData, 7);

  CHKSIZE(RssiAlarmData, 2);
  CHKSIZE(TrainerData, 16);

#if defined(PCBI6X)
  CHKSIZE(RadioData, 318);
  CHKSIZE(ModelData, 2848);
#elif defined(PCBXLITE)
  CHKSIZE(RadioData, 844);
  CHKSIZE(ModelData, 6025);
#elif defined(PCBX7)
  CHKSIZE(RadioData, 850);
  CHKSIZE(ModelData, 6025);
#elif defined(PCBX9E)
  CHKSIZE(RadioData, 952);
  CHKSIZE(ModelData, 6520);
#elif defined(PCBX9D)
  CHKSIZE(RadioData, 872);
  CHKSIZE(ModelData, 6507);
#elif defined(PCBHORUS)
  CHKSIZE(RadioData, 847);
  CHKSIZE(ModelData, 9380);
#endif

#undef CHKSIZE
#undef CHKSIZEUNION
}
#endif /* BACKUP */
