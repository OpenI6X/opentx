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

#ifndef _PULSES_ARM_H_
#define _PULSES_ARM_H_

#include "pulses/pxx1.h"
#include "pulses/pxx2.h"

#if NUM_MODULES == 2
#define MODULES_INIT(...) __VA_ARGS__, __VA_ARGS__
#else
#define MODULES_INIT(...) __VA_ARGS__
#endif

extern uint8_t s_pulses_paused;

PACK(struct ModuleState {
  uint8_t protocol; // :4
//  uint8_t paused; // :1 not used?
  uint8_t mode; // :3
  uint16_t counter;
});

extern ModuleState moduleState[NUM_MODULES];

template <class T>
struct PpmPulsesData {
  T pulses[20];
  T* ptr;
};

#if defined(PPM_PIN_SERIAL)
PACK(struct Dsm2SerialPulsesData {
  uint8_t pulses[64];
  uint8_t* ptr;
  uint8_t serialByte;
  uint8_t serialBitCount;
  uint16_t _alignment;
});
#else
#define MAX_PULSES_TRANSITIONS 300
PACK(struct Dsm2TimerPulsesData {
  pulse_duration_t pulses[MAX_PULSES_TRANSITIONS];
  pulse_duration_t* ptr;
  uint16_t rest;
  uint8_t index;
});
#endif

#define PPM_DEF_PERIOD               225 /* 22.5ms */
#define PPM_STEP_SIZE                5 /*0.5ms*/
#define PPM_PERIOD_HALF_US(module)   ((g_model.moduleData[module].ppm.frameLength * PPM_STEP_SIZE + PPM_DEF_PERIOD) * 200) /*half us*/
#define PPM_PERIOD(module) (PPM_PERIOD_HALF_US(module) / 2)                                       /*us*/
#define DSM2_BAUDRATE 125000
#define DSM2_PERIOD 22 /*ms*/
#define SBUS_BAUDRATE 100000
#define SBUS_MIN_PERIOD              60  /*6.0ms 1/10ms*/
#define SBUS_MAX_PERIOD              325 /*Overflows uint16_t if set higher*/
#define SBUS_DEF_PERIOD              225
#define SBUS_STEPSIZE                5   /* SBUS Step Size 0.5ms */
#define SBUS_PERIOD_HALF_US          ((g_model.moduleData[EXTERNAL_MODULE].sbus.refreshRate * SBUS_STEPSIZE + SBUS_DEF_PERIOD) * 200) /*half us*/
#define SBUS_PERIOD (SBUS_PERIOD_HALF_US / 2000)                                                     /*ms*/
#define MULTIMODULE_BAUDRATE 100000
#define MULTIMODULE_PERIOD 7 /*ms*/

#define CROSSFIRE_FRAME_MAXLEN 64
PACK(struct CrossfirePulsesData {
  uint8_t pulses[CROSSFIRE_FRAME_MAXLEN];
  uint8_t length;
});

// TODO Move AFHDS2A pulses here?
union InternalModulePulsesData {
#if defined(TARANIS_INTERNAL_PPM)
  PpmPulsesData<pulse_duration_t> ppm;
#endif
} __ALIGNED(4);

union ExternalModulePulsesData {
  PpmPulsesData<pulse_duration_t> ppm;
#if defined(CROSSFIRE)
  CrossfirePulsesData crossfire;
#endif
} __ALIGNED(4);

//TRACE("sizeof ModulePulsesData %d",sizeof(ModulePulsesData));

/* The __ALIGNED keyword is required to align the struct inside the modulePulsesData below,
 * which is also defined to be __DMA  (which includes __ALIGNED) aligned.
 * Arrays in C/C++ are always defined to be *contiguously*. The first byte of the second element is therefore always
 * sizeof(ModulePulsesData). __ALIGNED is required for sizeof(ModulePulsesData) to be a multiple of the alignment.
 */

extern InternalModulePulsesData intmodulePulsesData;
extern ExternalModulePulsesData extmodulePulsesData;

union TrainerPulsesData {
  PpmPulsesData<trainer_pulse_duration_t> ppm;
};

extern TrainerPulsesData trainerPulsesData;
extern const uint16_t CRCTable[];

bool setupPulses(uint8_t module);
void setupPulsesCrossfire();
void setupPulsesDSM2();
void setupPulsesMultimodule();
void setupPulsesSbus();
void setupPulsesPPMExternalModule();
void setupPulsesPPMTrainer();
void sendByteDsm2(uint8_t b);
void putDsm2Flush();
void putDsm2SerialBit(uint8_t bit);
void sendByteSbus(uint8_t byte);

#if defined(HUBSAN)
void Hubsan_Init();
#endif

inline void startPulses() {
//  TRACE_NOCRLF("sP");
  s_pulses_paused = false;

#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
  setupPulses(INTERNAL_MODULE);
  setupPulses(EXTERNAL_MODULE);
#else
  setupPulses(EXTERNAL_MODULE);
#endif

#if defined(HUBSAN)
  Hubsan_Init();
#endif
}

inline bool pulsesStarted() { return moduleState[0].protocol != 255; }
inline void pausePulses() { s_pulses_paused = true; }
inline void resumePulses() { s_pulses_paused = false; }

#define SEND_FAILSAFE_NOW(idx) moduleState[idx].counter = 1

inline void SEND_FAILSAFE_1S() {
  for (int i = 0; i < NUM_MODULES; i++) {
    moduleState[i].counter = 100;
  }
}

// Assign failsafe values using the current channel outputs
// for channels not set previously to HOLD or NOPULSE
void setCustomFailsafe(uint8_t moduleIndex);

#if defined(PCBXLITE) && !defined(MODULE_R9M_FULLSIZE)
#define LEN_R9M_REGION "\006"
#define TR_R9M_REGION \
    "FCC\0  "         \
    "EU\0   "         \
    "868MHz"          \
    "915MHz"
#define LEN_R9M_FCC_POWER_VALUES "\010"
#define LEN_R9M_LBT_POWER_VALUES "\015"
#define TR_R9M_FCC_POWER_VALUES "(100 mW)"
#define TR_R9M_LBT_POWER_VALUES \
    "25 mW 8ch\0   "            \
    "25 mW 16ch\0  "            \
    "100mW no tele"

enum R9MFCCPowerValues {
  R9M_FCC_POWER_100 = 0,
  R9M_FCC_POWER_MAX = R9M_FCC_POWER_100
};

enum R9MLBTPowerValues {
  R9M_LBT_POWER_25 = 0,
  R9M_LBT_POWER_25_16,
  R9M_LBT_POWER_100,
  R9M_LBT_POWER_MAX = R9M_LBT_POWER_100
};

#define BIND_TELEM_ALLOWED(idx) (!(IS_TELEMETRY_INTERNAL_MODULE() && moduleIdx == EXTERNAL_MODULE) && (!isModuleR9M_LBT(idx) || g_model.moduleData[idx].pxx.power < R9M_LBT_POWER_100))
#define BIND_CH9TO16_ALLOWED(idx) (!isModuleR9M_LBT(idx) || g_model.moduleData[idx].pxx.power != R9M_LBT_POWER_25)

#else

#define LEN_R9M_REGION "\006"
#define TR_R9M_REGION \
    "FCC\0  "         \
    "EU\0   "         \
    "868MHz"          \
    "915MHz"
#define LEN_R9M_FCC_POWER_VALUES "\013"
#define LEN_R9M_LBT_POWER_VALUES "\013"
#define TR_R9M_FCC_POWER_VALUES \
    "10 mW\0     "              \
    "100 mW\0    "              \
    "500 mW\0    "              \
    "Auto <= 1 W"
#define TR_R9M_LBT_POWER_VALUES \
    "25 mW 8ch\0 "              \
    "25 mW 16ch\0"              \
    "200 mW 16ch"               \
    "500 mW 16ch"

enum R9MFCCPowerValues {
  R9M_FCC_POWER_10 = 0,
  R9M_FCC_POWER_100,
  R9M_FCC_POWER_500,
  R9M_FCC_POWER_1000,
  R9M_FCC_POWER_MAX = R9M_FCC_POWER_1000
};

enum R9MLBTPowerValues {
  R9M_LBT_POWER_25 = 0,
  R9M_LBT_POWER_25_16,
  R9M_LBT_POWER_200,
  R9M_LBT_POWER_500,
  R9M_LBT_POWER_MAX = R9M_LBT_POWER_500
};

#define BIND_TELEM_ALLOWED(idx) (!(IS_TELEMETRY_INTERNAL_MODULE() && moduleIdx == EXTERNAL_MODULE) && (!isModuleR9M_LBT(idx) || g_model.moduleData[idx].pxx.power < R9M_LBT_POWER_200))
#define BIND_CH9TO16_ALLOWED(idx) (!isModuleR9M_LBT(idx) || g_model.moduleData[idx].pxx.power != R9M_LBT_POWER_25)
#endif
#endif  // _PULSES_ARM_H_
