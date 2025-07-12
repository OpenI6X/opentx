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
Fifo<BuzzerTone, 4> buzzerFifo;

void audioKeyPress()
{
  if (g_eeGeneral.beepMode == e_mode_all) {
    playTone(BEEP_DEFAULT_FREQ, 40, 20, PLAY_NOW);
  }
}

void audioKeyError()
{
  if (g_eeGeneral.beepMode >= e_mode_nokeys) {
//    playTone(BEEP_DEFAULT_FREQ, 160, 20, PLAY_NOW);
    audioEvent(AU_WARNING2);
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
#if defined(DFPLAYER)
  if (g_model.timers[timer].countdownBeep == COUNTDOWN_VOICE) {
    if (value >= 0 && value <= TIMER_COUNTDOWN_START(timer)) {
      playNumber(value, 0, 0, 0);
    }
    else if (value == 30 || value == 20) {
      playDuration(value, 0, 0);
    }
  } else
#endif // DFPLAYER
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

void audioEvent(unsigned int index)
{
  if (index == AU_NONE)
    return;

  if (g_eeGeneral.alarmsFlash) {
    flashCounter = FLASH_DURATION;
  }

  if (g_eeGeneral.beepMode >= e_mode_nokeys || (g_eeGeneral.beepMode >= e_mode_alarms && index <= AU_ERROR)) {
#if defined(DFPLAYER)
    if (index < AU_SPECIAL_SOUND_FIRST && isAudioFileReferenced(index)) {
      // dfPlayerQueueStopPlay(index); // really id until resolved by getAudioFileIndex
      dfPlayerQueuePlayFile(getAudioFileIndex(index));
      return;
    }
#endif
    switch (index) {
      case AU_INACTIVITY:
        playTone(2250, 80, 20, PLAY_REPEAT(2));
        break;
      case AU_TX_BATTERY_LOW:
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
      case AU_TELEMETRY_LOST:
        playTone(BEEP_DEFAULT_FREQ + 200, 40, 20);
      case AU_TRAINER_LOST:
        playTone(BEEP_DEFAULT_FREQ, 40, 20);
        playTone(BEEP_DEFAULT_FREQ - 200, 40, 20);
        break;
      case AU_TELEMETRY_BACK:
        playTone(BEEP_DEFAULT_FREQ - 200, 40, 20);
      case AU_TRAINER_BACK:
        playTone(BEEP_DEFAULT_FREQ, 40, 20);
        playTone(BEEP_DEFAULT_FREQ + 200, 40, 20);
        break;
      default:
        break;
    }
  }
}

static void setVolume(int8_t volume)
{
  volume += 2;
  switch (volume) {
    case 0: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 16; break;
    case 1: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 8; break;
    case 2: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 4; break;
    case 3: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 3; break;
    case 4: PWM_TIMER->CCR1 = PWM_TIMER->ARR / 2; break;
  }
}

static void setFrequency(uint32_t freq)
{
  PWM_TIMER->ARR = 1000000 / freq - 1; // freq below 16Hz will overflow 16bit ARR (never happen)
  if (PWM_TIMER->CNT > PWM_TIMER->ARR) // fixes vario noise on descent
    PWM_TIMER->CNT = 0;
}

static unsigned int getToneLength(uint16_t len)
{
  unsigned int result = len; // default
  if (g_eeGeneral.beepLength < 0) { // result /= (1-g_eeGeneral.beepLength);
    if (g_eeGeneral.beepLength == -1) // result /= (1+1);
      result /= 2;
    else // result /= (1+2);
      result = (result * 341) >> 10; // * 0,333 == /3
  }
  else if (g_eeGeneral.beepLength > 0) {
    result *= (1+g_eeGeneral.beepLength);
  }
  return result;
}

static void buzzerOn(uint32_t freq, int8_t volume)
{
  setFrequency(freq);
  setVolume(volume);
  PWM_TIMER->CR1 = TIM_CR1_CEN;
}

static void buzzerOff()
{
  PWM_TIMER->CR1 &= ~TIM_CR1_CEN;
  PWM_TIMER->CNT = 0;                     //
  PWM_TIMER->SR = (U16)~TIM_FLAG_Update;  // solves random hiss issue when timer stopped
}

void playTone(uint16_t freq, uint16_t len, uint16_t pause, uint8_t flags, int8_t freqIncr)
{
  if ((flags & PLAY_BACKGROUND) && !(flags & PLAY_NOW) 
    && (buzzerState.duration || (buzzerState.repeat > 0) || !buzzerFifo.isEmpty())) 
    return;

  if (!(flags & PLAY_NOW) && !(buzzerState.tone.flags & PLAY_BACKGROUND) && buzzerState.duration) {
    if (!(flags & PLAY_BACKGROUND))
      buzzerFifo.push(BuzzerTone(freq, len, pause, flags, freqIncr));
    return;
  }

  if (!(flags & PLAY_BACKGROUND)) { // should not affect vario
    freq += g_eeGeneral.speakerPitch * 15;
    len = getToneLength(len);
  }

  buzzerState.freq = freq;
  buzzerState.duration = len;
  buzzerState.pause = pause;
  buzzerState.repeat = flags & 0x0f;
  buzzerState.tone.freq = freq;
  buzzerState.tone.duration = len;
  buzzerState.tone.pause = pause;
  buzzerState.tone.flags = flags;
  buzzerState.tone.freqIncr = freqIncr;

  buzzerOn(freq, (flags & PLAY_BACKGROUND) ? g_eeGeneral.varioVolume : g_eeGeneral.beepVolume);
}

void buzzerHeartbeat()
{
  if (buzzerState.duration) {

    if (buzzerState.duration > 10) {
      buzzerState.duration -= 10; // ms

      if (buzzerState.tone.freqIncr) {
        uint32_t freqChange = BUZZER_BUFFER_DURATION * buzzerState.tone.freqIncr;
        buzzerState.freq += limit<uint32_t>(BEEP_MIN_FREQ, freqChange, BEEP_MAX_FREQ);

        buzzerOn(buzzerState.freq, g_eeGeneral.beepVolume);
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

        buzzerOn(buzzerState.freq, g_eeGeneral.beepVolume);
      }
    }
  } else {
    BuzzerTone tone;
    if (buzzerFifo.pop(tone)) {
        playTone(tone.freq, tone.duration, tone.pause, tone.flags, tone.freqIncr);
    }
  }
}
