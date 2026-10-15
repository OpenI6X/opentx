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

#include "mixer_scheduler.h"
#include "opentx.h"

task_handle_t menusTaskId;
TASK_DEFINE_STACK(menusStack, MENUS_STACK_SIZE);

task_handle_t mixerTaskId;
TASK_DEFINE_STACK(mixerStack, MIXER_STACK_SIZE);

#if defined(VOICE)
task_handle_t audioTaskId;
TASK_DEFINE_STACK(audioStack, AUDIO_STACK_SIZE);
#endif

mutex_handle_t audioMutex;
mutex_handle_t mixerMutex;

volatile uint16_t timeForcePowerOffPressed = 0;

bool isForcePowerOffRequested() 
{
  if (pwrOffPressed()) {
    if (timeForcePowerOffPressed == 0) {
      timeForcePowerOffPressed = get_tmr10ms();
    } else {
      uint16_t delay = (uint16_t)get_tmr10ms() - timeForcePowerOffPressed;
      if (delay > 1000 /*10s*/) {
        return true;
      }
    }
  } else {
    resetForcePowerOffRequest();
  }
  return false;
}

bool isModuleSynchronous(uint8_t moduleIdx) 
{
  switch (moduleState[moduleIdx].protocol) {
    case PROTOCOL_CHANNELS_CROSSFIRE:
    case PROTOCOL_CHANNELS_NONE:
#if defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_MULTIMODULE:
#endif
#if defined(AFHDS2A)
    case PROTOCOL_CHANNELS_AFHDS2A_SPI: // make AFHDS2A synchronous to make watchdog happy
#endif
    // case PROTOCOL_CHANNELS_SBUS:
      return true;
  }
  return false;
}

void sendSynchronousPulses(uint8_t runMask) 
{
#if defined(HARDWARE_INTERNAL_MODULE)
  if ((runMask & (1 << INTERNAL_MODULE)) && isModuleSynchronous(INTERNAL_MODULE)) {
    // Updates heartbeat - keep watchdog happy,
    // for AFHDS2A returns true because return value is used in INTMODULE_TIMER_IRQHandler,
    // but we don't want intmoduleSendNextFrame for it.
    if (setupPulsesInternalModule())
      ;// intmoduleSendNextFrame();
  }
#endif

#if defined(HARDWARE_EXTERNAL_MODULE)
  if ((runMask & (1 << EXTERNAL_MODULE)) && isModuleSynchronous(EXTERNAL_MODULE)) {
    if (setupPulsesExternalModule())
      extmoduleSendNextFrame();
  }
#endif
}

constexpr uint8_t MIXER_FREQUENT_ACTIONS_PERIOD = 5 /*ms*/;
constexpr uint8_t MIXER_MAX_PERIOD = MAX_REFRESH_RATE / 1000 /*ms*/;

void execMixerFrequentActions()
{
#if defined(BLUETOOTH)
      bluetoothWakeup();
#endif
}

void mixerTask()
{
  s_pulses_paused = true;

  mixerSchedulerInit();

  mixerSchedulerStart();

  while (true) {
    int timeout = 0;
    for (; timeout < MIXER_MAX_PERIOD; timeout += MIXER_FREQUENT_ACTIONS_PERIOD) {

      // run periodicals before waiting for the trigger
      // to keep the delay short
      execMixerFrequentActions();

      // mixer triggered (task notification)?
      if (!mixerSchedulerWaitForTrigger(MIXER_FREQUENT_ACTIONS_PERIOD)) {
        break;
      }
    }
   
    // re-enable trigger
    mixerSchedulerEnableTrigger();

#if defined(SIMU)
    if (pwrCheck() == e_power_off) {
      return;
    }
#elif !defined(PCBI6X)
    if (isForcePowerOffRequested()) {
      pwrOff();
    }
#endif

    if (!s_pulses_paused) {
      uint16_t t0 = getTmr2MHz();

      DEBUG_TIMER_START(debugTimerMixer);
      mutex_lock(&mixerMutex);

      doMixerCalculations();

#if defined(HARDWARE_INTERNAL_MODULE) && defined(HARDWARE_EXTERNAL_MODULE)
      sendSynchronousPulses((1 << INTERNAL_MODULE) | (1 << EXTERNAL_MODULE));
#elif defined(HARDWARE_INTERNAL_MODULE)
      sendSynchronousPulses((1 << INTERNAL_MODULE));
#elif defined(HARDWARE_EXTERNAL_MODULE)
      sendSynchronousPulses(1 << EXTERNAL_MODULE);
#endif

      doMixerPeriodicUpdates();

      DEBUG_TIMER_START(debugTimerMixerCalcToUsage);
      DEBUG_TIMER_SAMPLE(debugTimerMixerIterval);
      mutex_unlock(&mixerMutex);
      DEBUG_TIMER_STOP(debugTimerMixer);

#if defined(STM32) && !defined(SIMU)
      if (getSelectedUsbMode() == USB_JOYSTICK_MODE) {
        usbJoystickUpdate();
      }
#endif

      /**
       * Workaround for PCBI6X:
       * When HEART_WDT_CHECK (int + ext module) == 7
       * then it fails if heartbeat is up to 3 on only internal module,
       * because PPM init fails for some users.
       */
      if (heartbeat == HEART_WDT_CHECK || heartbeat == 3) {
        WDG_RESET();
        heartbeat = 0;
      }

      t0 = getTmr2MHz() - t0;
      if (t0 > maxMixerDuration)
        maxMixerDuration = t0;
    }
  }
}

#define MENU_TASK_PERIOD (50)  // 50ms

#if defined(COLORLCD) && defined(CLI)
bool perMainEnabled = true;
#endif

void menusTask()
{
  opentxInit();

#if defined(PWR_BUTTON_PRESS) || defined(PWR_BUTTON_EMULATED)
  while (true) {
    uint32_t pwr_check = pwrCheck();
    if (pwr_check == e_power_off) {
      break;
    } else if (pwr_check == e_power_press) {
      sleep_ms(MENU_TASK_PERIOD);
      continue;
    }
#else
  while (pwrCheck() != e_power_off) {
#endif
    uint32_t start = time_get_ms();
    DEBUG_TIMER_START(debugTimerPerMain);

    perMain();

    DEBUG_TIMER_STOP(debugTimerPerMain);
    // TODO remove completely massstorage from sky9x firmware
    uint32_t runtime = (time_get_ms() - start);
    // deduct the thread run-time from the wait, if run-time was more than
    // desired period, then skip the wait all together
    if (runtime < MENU_TASK_PERIOD) {
      sleep_ms(MENU_TASK_PERIOD - runtime);
    }

#if !defined(PCBI6X) // no software controlled power on i6X
    resetForcePowerOffRequest();
#endif
  }

#if defined(PCBX9E)
  toplcdOff();
#endif

#if defined(PCBHORUS)
  ledOff();
#endif

#if defined(PWR_BUTTON_PRESS) || defined(PWR_BUTTON_EMULATED)
  drawSleepBitmap();
  opentxClose();
  boardOff();

  return;
#endif
}

void tasksStart() {
  // Before anything can start it (telemetry ISRs are enabled later, from tasks)
  timer_create(&telemetryProcessFrameTimer, telemetryProcessFrame, 0, false);  // one-shot, asap

#if defined(CLI)
  // cliStart(); // now handled by handleCli()
#endif

  // Order of creation determines priority (first = highest).
  task_create(&mixerTaskId, mixerTask, "Mixer", mixerStack, MIXER_STACK_SIZE);
  // Timer task (software timer callbacks, os/timer.h): below the mixer,
  // above menus. The telemetry parser runs here (telemetryProcessFrameTimer), so
  // it is never delayed by a long menus pass (LCD refresh, model load, EEPROM
  // write) and the 128 B CRSF DMA FIFO cannot overrun; it preempts menus
  // exactly as it did inside the mixer task (same concurrency with the ELRS
  // tool callback as before). Also runs in SIMU, where it polls.
  timer_init();
  task_create(&menusTaskId, menusTask, "Menus", menusStack, MENUS_STACK_SIZE);

#if !defined(SIMU) && defined(VOICE)
  task_create(&audioTaskId, audioTask, "Audio", audioStack, AUDIO_STACK_SIZE);
#endif


#if defined(VOICE)
  mutex_create(&audioMutex);
#endif
  mutex_create(&mixerMutex);

  RTOS_START();
}
