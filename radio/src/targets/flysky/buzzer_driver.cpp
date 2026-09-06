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

struct AudioToneData {
  int8_t frequencyOffset; // 30Hz steps relative to BEEP_DEFAULT_FREQ
  uint8_t duration;       // 2ms units
  uint8_t pause;          // 2ms units
  uint8_t options;        // flags in bits 0..4, encoded frequency increment in bits 5..7
};

static_assert(sizeof(AudioToneData) == 4, "AudioToneData must remain compact");

template<int Frequency>
struct EncodedAudioFrequency {
  static_assert((Frequency - BEEP_DEFAULT_FREQ) % 30 == 0, "Audio frequency must use 30Hz steps");
  static_assert((Frequency - BEEP_DEFAULT_FREQ) / 30 >= -128 &&
                (Frequency - BEEP_DEFAULT_FREQ) / 30 <= 127, "Audio frequency is out of range");
  static constexpr int8_t value = (Frequency - BEEP_DEFAULT_FREQ) / 30;
};

template<int Time>
struct EncodedAudioTime {
  static_assert(Time >= 0 && Time <= 510 && Time % 2 == 0, "Audio time must use 2ms steps");
  static constexpr uint8_t value = Time / 2;
};

template<int Flags, int Increment>
struct EncodedAudioOptions {
  static_assert((Flags & ~0x1f) == 0, "Unsupported audio flags");
  static_assert(Increment >= -1 && Increment <= 6, "Audio frequency increment is out of range");
  static constexpr uint8_t value = Flags | ((Increment + 1) << 5);
};

#define AUDIO_TONE(freq, duration, pause, flags, increment) { \
  EncodedAudioFrequency<freq>::value, \
  EncodedAudioTime<duration>::value, \
  EncodedAudioTime<pause>::value, \
  EncodedAudioOptions<flags, increment>::value \
}

// Tone sequences are stored in AUDIO_SOUNDS order, from
// AU_SPECIAL_SOUND_BEEP1 through AU_SPECIAL_SOUND_ALARMC.
static constexpr AudioToneData specialSoundTones[] = {
  AUDIO_TONE(BEEP_DEFAULT_FREQ,        60,  20, 0,              0),  // BEEP1
  AUDIO_TONE(BEEP_DEFAULT_FREQ,       120,  20, 0,              0),  // BEEP2
  AUDIO_TONE(BEEP_DEFAULT_FREQ,       200,  20, 0,              0),  // BEEP3
  AUDIO_TONE(BEEP_DEFAULT_FREQ + 600, 120,  40, PLAY_REPEAT(2), 0),  // WARN1
  AUDIO_TONE(BEEP_DEFAULT_FREQ + 900, 120,  40, PLAY_REPEAT(2), 0),  // WARN2
  AUDIO_TONE(BEEP_DEFAULT_FREQ + 900,  80,  20, PLAY_REPEAT(2), 2),  // CHEEP
  AUDIO_TONE(BEEP_DEFAULT_FREQ + 1500, 40,  80, PLAY_REPEAT(10), 0), // RATATA
  AUDIO_TONE(BEEP_DEFAULT_FREQ + 1500, 40, 400, PLAY_REPEAT(2), 0),  // TICK
  AUDIO_TONE(450,                     160,  40, PLAY_REPEAT(2), 2),  // SIREN

  AUDIO_TONE(BEEP_DEFAULT_FREQ + 750,  40,  20, PLAY_REPEAT(10), 0), // RING
  AUDIO_TONE(BEEP_DEFAULT_FREQ + 750,  40,  80, PLAY_REPEAT(1),  0),
  AUDIO_TONE(BEEP_DEFAULT_FREQ + 750,  40,  20, PLAY_REPEAT(10), 0),

  AUDIO_TONE(2550,  80,  20, PLAY_REPEAT(2), -1), // SCIFI
  AUDIO_TONE(1950,  80,  20, PLAY_REPEAT(2),  1),
  AUDIO_TONE(2250,  80,  20, 0,               0),

  AUDIO_TONE(2250,  40,  20, PLAY_REPEAT(1), 0), // ROBOT
  AUDIO_TONE(1650, 120,  20, PLAY_REPEAT(1), 0),
  AUDIO_TONE(2550, 120,  20, PLAY_REPEAT(1), 0),

  AUDIO_TONE(BEEP_DEFAULT_FREQ + 1200, 40, 20, PLAY_REPEAT(2), 0), // CHIRP
  AUDIO_TONE(BEEP_DEFAULT_FREQ + 1620, 40, 20, PLAY_REPEAT(3), 0),

  AUDIO_TONE(1650, 80, 40, 0,              0), // TADA
  AUDIO_TONE(2850, 80, 40, 0,              0),
  AUDIO_TONE(3450, 60, 36, PLAY_REPEAT(2), 0),

  AUDIO_TONE(2550, 40,  80, PLAY_REPEAT(3), 0), // CRICKET
  AUDIO_TONE(2550, 40, 160, PLAY_REPEAT(1), 0),
  AUDIO_TONE(2550, 40,  80, PLAY_REPEAT(3), 0),

  AUDIO_TONE(1650, 30,  70, PLAY_REPEAT(2), 0), // ALARMC
  AUDIO_TONE(2250, 60, 160, PLAY_REPEAT(1), 0),
  AUDIO_TONE(1650, 60,  80, PLAY_REPEAT(2), 0),
  AUDIO_TONE(2250, 30, 170, PLAY_REPEAT(1), 0),
};

// Exclusive end offset of every sequence in specialSoundTones.
static constexpr uint8_t specialSoundEnds[] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 15, 18, 20, 23, 26, 30
};

static_assert(DIM(specialSoundEnds) == AU_SPECIAL_SOUND_LAST - AU_SPECIAL_SOUND_FIRST,
              "Missing special sound sequence");
static_assert(specialSoundEnds[DIM(specialSoundEnds) - 1] == DIM(specialSoundTones),
              "Invalid special sound sequence offsets");

#undef AUDIO_TONE

static bool playSpecialSound(unsigned int index)
{
  if (index < AU_SPECIAL_SOUND_FIRST || index >= AU_SPECIAL_SOUND_LAST) {
    return false;
  }

  uint8_t sound = index - AU_SPECIAL_SOUND_FIRST;
  uint8_t first = (sound == 0 ? 0 : specialSoundEnds[sound - 1]);
  uint8_t end = specialSoundEnds[sound];

  while (first < end) {
    const AudioToneData & tone = specialSoundTones[first++];
    playTone(BEEP_DEFAULT_FREQ + 30 * tone.frequencyOffset,
             2 * tone.duration,
             2 * tone.pause,
             tone.options & 0x1f,
             (tone.options >> 5) - 1);
  }
  return true;
}

static void playRegularSound(unsigned int index)
{
#if !defined(DFPLAYER)
  if (index == AU_INACTIVITY) {
    playTone(2250, 80, 20, PLAY_REPEAT(2));
    return;
  }
  if (index == AU_TX_BATTERY_LOW) {
    playTone(1950, 160, 20, PLAY_REPEAT(2), 1);
    playTone(2550, 160, 20, PLAY_REPEAT(2), -1);
    return;
  }
  if (index >= AU_TRIM_MIDDLE && index <= AU_TRIM_MAX) {
    uint16_t frequency = (index == AU_TRIM_MIDDLE ? 120 * 16 + 100 :
                         (index == AU_TRIM_MIN ? TRIM_MIN * 8 + 120 * 16 :
                                                TRIM_MAX * 8 + 120 * 16));
    playTone(frequency, index == AU_TRIM_MIDDLE ? 120 : 80, 20, PLAY_NOW);
    return;
  }
#endif

  if (index == AU_THROTTLE_ALERT || index == AU_SWITCH_ALERT || index == AU_ERROR) {
    playTone(BEEP_DEFAULT_FREQ, 200, 20, PLAY_NOW);
    return;
  }
  if (index >= AU_WARNING1 && index <= AU_WARNING3) {
    uint16_t duration = (index == AU_WARNING1 ? 80 : (index == AU_WARNING2 ? 160 : 200));
    playTone(BEEP_DEFAULT_FREQ, duration, 20, PLAY_NOW);
    return;
  }
  if (index >= AU_STICK1_MIDDLE && index <= AU_POT2_MIDDLE) {
    playTone(BEEP_DEFAULT_FREQ + 1500, 80, 20, PLAY_NOW);
    return;
  }
  if (index >= AU_MIX_WARNING_1 && index <= AU_MIX_WARNING_3) {
    uint8_t warning = index - AU_MIX_WARNING_1;
    playTone(BEEP_DEFAULT_FREQ + 1440 + 120 * warning, 48, 30, PLAY_REPEAT(warning));
    return;
  }

#if !defined(DFPLAYER)
  if (index >= AU_TIMER1_ELAPSED && index <= AU_TIMER3_ELAPSED) {
    playTone(BEEP_DEFAULT_FREQ + 150, 300, 20, PLAY_NOW);
    return;
  }
  if (index == AU_RSSI_ORANGE || index == AU_RSSI_RED) {
    playTone(BEEP_DEFAULT_FREQ + (index == AU_RSSI_ORANGE ? 1500 : 1800),
             800, 20, index == AU_RSSI_ORANGE ? PLAY_NOW : PLAY_REPEAT(1) | PLAY_NOW);
    return;
  }
  if (index >= AU_TELEMETRY_LOST && index <= AU_TRAINER_BACK) {
    bool back = (index == AU_TELEMETRY_BACK || index == AU_TRAINER_BACK);
    if (index == AU_TELEMETRY_LOST || index == AU_TELEMETRY_BACK) {
      playTone(BEEP_DEFAULT_FREQ + (back ? -200 : 200), 40, 20);
    }
    playTone(BEEP_DEFAULT_FREQ, 40, 20);
    playTone(BEEP_DEFAULT_FREQ + (back ? 200 : -200), 40, 20);
  }
#endif
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
    if (playSpecialSound(index)) {
      return;
    }
    playRegularSound(index);
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

    if (buzzerState.duration > BUZZER_SAMPLE_DURATION) {
      buzzerState.duration -= BUZZER_SAMPLE_DURATION;

      if (buzzerState.tone.freqIncr) {
        uint32_t freqChange = BUZZER_SAMPLE_DURATION * buzzerState.tone.freqIncr;
        buzzerState.freq = limit<uint32_t>(BEEP_MIN_FREQ, buzzerState.freq + freqChange, BEEP_MAX_FREQ);

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
