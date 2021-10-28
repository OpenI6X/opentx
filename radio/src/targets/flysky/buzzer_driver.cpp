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
#include "buzzer_driver.h"

volatile BuzzerState buzzerState;
BuzzerToneFifo buzzerFifo = BuzzerToneFifo();

void audioKeyPress()
{
  if (g_eeGeneral.beepMode == e_mode_all) {
    playTone(BEEP_DEFAULT_FREQ, 40, 20, PLAY_NOW);
  }
}

void audioKeyError()
{
  if (g_eeGeneral.beepMode >= e_mode_nokeys) {
    playTone(BEEP_DEFAULT_FREQ, 160, 20, PLAY_NOW);
  }
}

void audioTrimPress(int value)
{
  if (g_eeGeneral.beepMode >= e_mode_nokeys) {
    value = limit(TRIM_MIN, value, TRIM_MAX) * 8 + 120*16;
    playTone(value, 40, 20, PLAY_NOW);
  }
}

void audioTimerCountdown(uint8_t timer, int value)
{
  if (g_model.timers[timer].countdownBeep == COUNTDOWN_BEEPS) {
    if (value == 0) {
      playTone(BEEP_DEFAULT_FREQ + 150, 300, 20, PLAY_NOW);
    }
    else if (value > 0 && value <= TIMER_COUNTDOWN_START(timer)) {
      playTone(BEEP_DEFAULT_FREQ + 150, 100, 20, PLAY_NOW);
    }
    else if (value == 30) {
      playTone(BEEP_DEFAULT_FREQ + 150, 120, 20, PLAY_REPEAT(2));
    }
    else if (value == 20) {
      playTone(BEEP_DEFAULT_FREQ + 150, 120, 20, PLAY_REPEAT(1));
    }
    else if (value == 10) {
      playTone(BEEP_DEFAULT_FREQ + 150, 120, 20, PLAY_NOW);
    }
  }
}

void buzzerEvent(unsigned int index)
{
  // TRACE("buzzerEvent %u", index);
  if (index == AU_NONE)
    return;


  if (g_eeGeneral.alarmsFlash) {
    flashCounter = FLASH_DURATION;
  }

  if (g_eeGeneral.beepMode >= e_mode_nokeys || (g_eeGeneral.beepMode >= e_mode_alarms && index <= AU_ERROR)) {
    switch (index) {
      case AU_INACTIVITY:
        playTone(2250, 80, 20, PLAY_REPEAT(2));
        break;
      case AU_TX_BATTERY_LOW:
#if defined(PCBSKY9X)
      case AU_TX_MAH_HIGH:
      case AU_TX_TEMP_HIGH:
#endif
        playTone(1950, 160, 20, PLAY_REPEAT(2), 1);
        playTone(2550, 160, 20, PLAY_REPEAT(2), -1);
        break;
      case AU_THROTTLE_ALERT:
      case AU_SWITCH_ALERT:
      case AU_ERROR:
        playTone(BEEP_DEFAULT_FREQ, 200, 20, PLAY_NOW);
        break;
      case AU_TRIM_MIDDLE:
        playTone(120*16, 80, 20, PLAY_NOW);
        break;
      case AU_TRIM_MIN:
        playTone(TRIM_MIN*8 + 120*16, 80, 20, PLAY_NOW);
        break;
      case AU_TRIM_MAX:
        playTone(TRIM_MAX*8 + 120*16, 80, 20, PLAY_NOW);
        break;
      case AU_WARNING1:
        playTone(BEEP_DEFAULT_FREQ, 80, 20, PLAY_NOW);
        break;
      case AU_WARNING2:
        playTone(BEEP_DEFAULT_FREQ, 160, 20, PLAY_NOW);
        break;
      case AU_WARNING3:
        playTone(BEEP_DEFAULT_FREQ, 200, 20, PLAY_NOW);
        break;
        // TO.DO remove all these ones
      case AU_STICK1_MIDDLE:
      case AU_STICK2_MIDDLE:
      case AU_STICK3_MIDDLE:
      case AU_STICK4_MIDDLE:
      case AU_POT1_MIDDLE:
      case AU_POT2_MIDDLE:
        playTone(BEEP_DEFAULT_FREQ + 1500, 80, 20, PLAY_NOW);
        break;
      case AU_MIX_WARNING_1:
        playTone(BEEP_DEFAULT_FREQ + 1440, 48, 30);
        break;
      case AU_MIX_WARNING_2:
        playTone(BEEP_DEFAULT_FREQ + 1560, 48, 30, PLAY_REPEAT(1));
        break;
      case AU_MIX_WARNING_3:
        playTone(BEEP_DEFAULT_FREQ + 1680, 48, 30, PLAY_REPEAT(2));
        break;
      case AU_TIMER1_ELAPSED:
      case AU_TIMER2_ELAPSED:
      case AU_TIMER3_ELAPSED:
        playTone(BEEP_DEFAULT_FREQ + 150, 300, 20, PLAY_NOW);
        break;
      case AU_RSSI_ORANGE:
        playTone(BEEP_DEFAULT_FREQ + 1500, 800, 20, PLAY_NOW);
        break;
      case AU_RSSI_RED:
        playTone(BEEP_DEFAULT_FREQ + 1800, 800, 20, PLAY_REPEAT(1) | PLAY_NOW);
        break;
      case AU_RAS_RED:
        playTone(450, 160, 40, PLAY_REPEAT(2), 1);
        break;
      case AU_SPECIAL_SOUND_BEEP1:
        playTone(BEEP_DEFAULT_FREQ, 60, 20);
        break;
      case AU_SPECIAL_SOUND_BEEP2:
        playTone(BEEP_DEFAULT_FREQ, 120, 20);
        break;
      case AU_SPECIAL_SOUND_BEEP3:
        playTone(BEEP_DEFAULT_FREQ, 200, 20);
        break;
      case AU_SPECIAL_SOUND_WARN1:
        playTone(BEEP_DEFAULT_FREQ + 600, 120, 40, PLAY_REPEAT(2));
        break;
      case AU_SPECIAL_SOUND_WARN2:
        playTone(BEEP_DEFAULT_FREQ + 900, 120, 40, PLAY_REPEAT(2));
        break;
      case AU_SPECIAL_SOUND_CHEEP:
        playTone(BEEP_DEFAULT_FREQ + 900, 80, 20, PLAY_REPEAT(2), 2);
        break;
      case AU_SPECIAL_SOUND_RING:
        playTone(BEEP_DEFAULT_FREQ + 750, 40, 20, PLAY_REPEAT(10));
        playTone(BEEP_DEFAULT_FREQ + 750, 40, 80, PLAY_REPEAT(1));
        playTone(BEEP_DEFAULT_FREQ + 750, 40, 20, PLAY_REPEAT(10));
        break;
      case AU_SPECIAL_SOUND_SCIFI:
        playTone(2550, 80, 20, PLAY_REPEAT(2), -1);
        playTone(1950, 80, 20, PLAY_REPEAT(2), 1);
        playTone(2250, 80, 20, 0);
        break;
      case AU_SPECIAL_SOUND_ROBOT:
        playTone(2250, 40, 20, PLAY_REPEAT(1));
        playTone(1650, 120, 20, PLAY_REPEAT(1));
        playTone(2550, 120, 20, PLAY_REPEAT(1));
        break;
      case AU_SPECIAL_SOUND_CHIRP:
        playTone(BEEP_DEFAULT_FREQ + 1200, 40, 20, PLAY_REPEAT(2));
        playTone(BEEP_DEFAULT_FREQ + 1620, 40, 20, PLAY_REPEAT(3));
        break;
      case AU_SPECIAL_SOUND_TADA:
        playTone(1650, 80, 40);
        playTone(2850, 80, 40);
        playTone(3450, 60, 36, PLAY_REPEAT(2));
        break;
      case AU_SPECIAL_SOUND_CRICKET:
        playTone(2550, 40, 80, PLAY_REPEAT(3));
        playTone(2550, 40, 160, PLAY_REPEAT(1));
        playTone(2550, 40, 80, PLAY_REPEAT(3));
        break;
      case AU_SPECIAL_SOUND_SIREN:
        playTone(450, 160, 40, PLAY_REPEAT(2), 2);
        break;
      case AU_SPECIAL_SOUND_ALARMC:
        playTone(1650, 30, 70, PLAY_REPEAT(2));
        playTone(2250, 60, 160, PLAY_REPEAT(1));
        playTone(1650, 60, 80, PLAY_REPEAT(2));
        playTone(2250, 30, 170, PLAY_REPEAT(1));
        break;
      case AU_SPECIAL_SOUND_RATATA:
        playTone(BEEP_DEFAULT_FREQ + 1500, 40, 80, PLAY_REPEAT(10));
        break;
      case AU_SPECIAL_SOUND_TICK:
        playTone(BEEP_DEFAULT_FREQ + 1500, 40, 400, PLAY_REPEAT(2));
        break;
      case AU_TRAINER_LOST:
      case AU_TELEMETRY_LOST:
        playTone(BEEP_DEFAULT_FREQ + 200, 40, 20);
        playTone(BEEP_DEFAULT_FREQ, 40, 20);
        playTone(BEEP_DEFAULT_FREQ - 200, 40, 20);
        break;
      case AU_TRAINER_BACK:
      case AU_TELEMETRY_BACK:
        playTone(BEEP_DEFAULT_FREQ - 200, 40, 20);
        playTone(BEEP_DEFAULT_FREQ, 40, 20);
        playTone(BEEP_DEFAULT_FREQ + 200, 40, 20);
        break;
      default:
        break;
    }
  }
}

void setVolume(uint8_t volume)
{
  switch (volume) {
    case 0: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 16; break;
    case 1: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 8; break;
    case 2: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 4; break;
    case 3: PWM_TIMER->CCR1 = (PWM_TIMER->ARR / 4) + (PWM_TIMER->ARR / 8); break;
    case 4: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 2; break;
  }
}

void setSampleRate(uint32_t frequency)
{
  uint32_t timer = 1000000 / frequency - 1 ;

  PWM_TIMER->CR1 &= ~TIM_CR1_CEN ;
  PWM_TIMER->CNT = 0 ;
  PWM_TIMER->ARR = limit<uint32_t>(2, timer, 65535) ;
  PWM_TIMER->CR1 |= TIM_CR1_CEN ;
}

inline unsigned int getToneLength(uint16_t len)
{
  unsigned int result = len; // default
  if (g_eeGeneral.beepLength < 0) {
    if (g_eeGeneral.beepLength == -1)
        result /= 2; // let compiler replace with shift instead of soft div on M0
    else
        result = (result * 341) >> 10;
  }
  else if (g_eeGeneral.beepLength > 0) {
    result *= (1+g_eeGeneral.beepLength);
  }
  return result;
}

inline void buzzerOn()
{
  PWM_TIMER->CR1 = TIM_CR1_CEN;
}

inline void buzzerOff()
{
  PWM_TIMER->CR1 &= ~TIM_CR1_CEN;
  PWM_TIMER->CNT = 0;                     //
  PWM_TIMER->SR = (U16)~TIM_FLAG_Update;  // solves random hiss issue when timer stopped
}

void playTone(uint16_t freq, uint16_t len, uint16_t pause, uint8_t flags, int8_t freqIncr)
{
  if (!(flags & PLAY_NOW) && buzzerState.duration) {
    // skip PLAY_BACKGROUND if other tone is playing
    if (!(flags & PLAY_BACKGROUND) && !buzzerFifo.full())
      buzzerFifo.push(BuzzerTone(freq, len, pause, flags, freqIncr));
    return;
  } else if ((flags & PLAY_NOW) && (buzzerState.repeat > 0)) { // push back to queue
    if (!buzzerFifo.full()) {
      if (buzzerState.duration - len < buzzerState.tone.duration / 2) {
        buzzerState.repeat--;
      }
      buzzerFifo.push(BuzzerTone(
        buzzerState.tone.freq,
        buzzerState.tone.duration,
        buzzerState.tone.pause,
        buzzerState.repeat,
        buzzerState.tone.freqIncr));
    }
  }

  uint8_t repeat = flags & 0x0f;
  freq += g_eeGeneral.speakerPitch * 15;
  len = getToneLength(len);
  
  buzzerState.freq = freq;
  buzzerState.duration = len;
  buzzerState.pause = pause;
  buzzerState.repeat = repeat;
  buzzerState.tone.freq = freq;
  buzzerState.tone.duration = len;
  buzzerState.tone.pause = pause;
  buzzerState.tone.flags = repeat;
  buzzerState.tone.freqIncr = freqIncr;

  setSampleRate(buzzerState.freq);
  setVolume(g_eeGeneral.beepVolume + 2);
  buzzerOn();
}

void buzzerHeartbeat()
{
  if (buzzerState.duration) {

    if (buzzerState.duration > 10) {
      buzzerState.duration -= 10; // ms

      if (buzzerState.tone.freqIncr) {
        uint32_t freqChange = BUZZER_BUFFER_DURATION * buzzerState.tone.freqIncr;
        buzzerState.freq += limit<uint16_t>(BEEP_MIN_FREQ, freqChange, BEEP_MAX_FREQ);
        setSampleRate(buzzerState.freq);
        setVolume(g_eeGeneral.beepVolume + 2);
      }
    }
    else {
      buzzerState.duration = 0;

      buzzerOff();

      if (buzzerState.pause) {
        buzzerState.duration = buzzerState.pause;
        buzzerState.pause = 0;
      } 
      else if (buzzerState.repeat) {
        buzzerState.repeat--;
        buzzerState.freq = buzzerState.tone.freq;
        buzzerState.duration = buzzerState.tone.duration;
        buzzerState.pause = buzzerState.tone.pause;

        setSampleRate(buzzerState.freq);
        setVolume(g_eeGeneral.beepVolume + 2);
        buzzerOn();
      }
    }
  } else if (!buzzerFifo.empty()) {
    uint8_t nextIdx = buzzerFifo.get();
    playTone(
      buzzerFifo.tones[nextIdx].freq, 
      buzzerFifo.tones[nextIdx].duration, 
      buzzerFifo.tones[nextIdx].pause, 
      buzzerFifo.tones[nextIdx].flags, 
      buzzerFifo.tones[nextIdx].freqIncr
      );
  }
}
