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

RTOS_TASK_HANDLE menusTaskId;
RTOS_DEFINE_STACK(menusStack, MENUS_STACK_SIZE);

RTOS_TASK_HANDLE mixerTaskId;
RTOS_DEFINE_STACK(mixerStack, MIXER_STACK_SIZE);

#if defined(VOICE)
RTOS_TASK_HANDLE audioTaskId;
RTOS_DEFINE_STACK(audioStack, AUDIO_STACK_SIZE);
#endif

RTOS_MUTEX_HANDLE audioMutex;
RTOS_MUTEX_HANDLE mixerMutex;

void stackPaint() 
{
  menusStack.paint();
  mixerStack.paint();
#if defined(VOICE)
  audioStack.paint();
#endif
#if defined(CLI)
  cliStack.paint();
#endif
}

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
    case PROTOCOL_CHANNELS_AFHDS2A_SPI: // make AFHDS2A synchronous to make watchdog happy
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
  if (!s_pulses_paused) {
    DEBUG_TIMER_START(debugTimerTelemetryWakeup);
    telemetryWakeup();
    DEBUG_TIMER_STOP(debugTimerTelemetryWakeup);
  }

#if defined(SBUS_TRAINER)
  if (g_eeGeneral.auxSerialMode == UART_MODE_SBUS_TRAINER) {
    processSbusInput();
  }
#endif

#if defined(BLUETOOTH)
      bluetoothWakeup();
#endif
}

TASK_FUNCTION(mixerTask) 
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

      // mixer flag triggered?
      if (!mixerSchedulerWaitForTrigger(MIXER_FREQUENT_ACTIONS_PERIOD)) {
        break;
      }
    }

#if defined(DEBUG_MIXER_SCHEDULER)
    GPIO_SetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
    GPIO_ResetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
#endif
   
    // re-enable trigger
    mixerSchedulerClearTrigger();
    mixerSchedulerEnableTrigger();

#if defined(SIMU)
    if (pwrCheck() == e_power_off) {
      TASK_RETURN();
    }
#elif !defined(PCBI6X)
    if (isForcePowerOffRequested()) {
      pwrOff();
    }
#endif

    if (!s_pulses_paused) {
      uint16_t t0 = getTmr2MHz();

      DEBUG_TIMER_START(debugTimerMixer);
      RTOS_LOCK_MUTEX(mixerMutex);

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
      RTOS_UNLOCK_MUTEX(mixerMutex);
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
        wdt_reset();
        heartbeat = 0;
      }

      t0 = getTmr2MHz() - t0;
      if (t0 > maxMixerDuration)
        maxMixerDuration = t0;
    }
  }
}

#define MENU_TASK_PERIOD_TICKS (50 / RTOS_MS_PER_TICK)   // 50ms

#if defined(COLORLCD) && defined(CLI)
bool perMainEnabled = true;
#endif

TASK_FUNCTION(menusTask)
{
  opentxInit();

#if defined(PWR_BUTTON_PRESS)
  while (true) {
    uint32_t pwr_check = pwrCheck();
    if (pwr_check == e_power_off) {
      break;
    } else if (pwr_check == e_power_press) {
      RTOS_WAIT_TICKS(MENU_TASK_PERIOD_TICKS);
      continue;
    }
#else
  while (pwrCheck() != e_power_off) {
#endif
    uint32_t start = (uint32_t)RTOS_GET_TIME();
    DEBUG_TIMER_START(debugTimerPerMain);

    perMain();

    DEBUG_TIMER_STOP(debugTimerPerMain);
    // TODO remove completely massstorage from sky9x firmware
    uint32_t runtime = ((uint32_t)RTOS_GET_TIME() - start);
    // deduct the thread run-time from the wait, if run-time was more than
    // desired period, then skip the wait all together
    if (runtime < MENU_TASK_PERIOD_TICKS) {
      RTOS_WAIT_TICKS(MENU_TASK_PERIOD_TICKS - runtime);
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

#if !defined(PCBI6X) // no software controlled power on i6X
  drawSleepBitmap();
  opentxClose();
  boardOff();  // Only turn power off if necessary

  TASK_RETURN();
#endif
}

void tasksStart() {
  RTOS_INIT();

#if defined(CLI)
  cliStart();
#endif

  RTOS_CREATE_TASK(mixerTaskId, mixerTask, "Mixer", mixerStack, MIXER_STACK_SIZE, MIXER_TASK_PRIO);
  RTOS_CREATE_TASK(menusTaskId, menusTask, "Menus", menusStack, MENUS_STACK_SIZE, MENUS_TASK_PRIO);

#if !defined(SIMU) && defined(VOICE)
  RTOS_CREATE_TASK(audioTaskId, audioTask, "Audio", audioStack, AUDIO_STACK_SIZE, AUDIO_TASK_PRIO);
#endif

#if defined(VOICE)
  RTOS_CREATE_MUTEX(audioMutex);
#endif
  RTOS_CREATE_MUTEX(mixerMutex);

  RTOS_START();
}
