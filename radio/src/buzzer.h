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

#ifndef _BUZZER_H_
#define _BUZZER_H_


extern bool isPlaying();
extern void audioEvent(unsigned int index);

#if defined(HAPTIC)
extern uint8_t hapticTick;
#endif /* HAPTIC */

void buzzerEvent(unsigned int index);

#if defined(DFPLAYER)
#define AUDIO_ERROR_MESSAGE(e)   audioEvent(e)
#define AUDIO_TIMER_MINUTE(t)    playDuration(t, 0, 0)

#define AUDIO_KEY_PRESS()        audioKeyPress()
#define AUDIO_KEY_ERROR()        audioKeyError()

#define AUDIO_HELLO()            //audioPlay(AUDIO_HELLO)
#define AUDIO_BYE()              //
#define AUDIO_WARNING1()         audioEvent(AU_WARNING1)
#define AUDIO_WARNING2()         audioEvent(AU_WARNING2)
#define AUDIO_TX_BATTERY_LOW()   audioEvent(AU_TX_BATTERY_LOW)
#define AUDIO_ERROR()            audioEvent(AU_ERROR)
#define AUDIO_TIMER_COUNTDOWN(idx, val) audioTimerCountdown(idx, val)
#define AUDIO_TIMER_ELAPSED(idx) audioEvent(182/*AU_TIMER1_ELAPSED*/+idx) // 
#define AUDIO_INACTIVITY()       audioEvent(AU_INACTIVITY)
#define AUDIO_MIX_WARNING(x)     audioEvent(AU_MIX_WARNING_1+x-1)
#define AUDIO_POT_MIDDLE(x)      audioEvent(AU_STICK1_MIDDLE+x)
#define AUDIO_TRIM_MIDDLE()      audioEvent(179 /*AU_TRIM_MIDDLE*/)
#define AUDIO_TRIM_MIN()         audioEvent(181 /*AU_TRIM_MIN*/)
#define AUDIO_TRIM_MAX()         audioEvent(180 /*AU_TRIM_MAX*/)
#define AUDIO_TRIM_PRESS(val)    audioTrimPress(val)
#define AUDIO_PLAY(p)            audioEvent(p)
#define AUDIO_VARIO(fq, t, p, f) playTone(fq, t, p, f)
#define AUDIO_RSSI_ORANGE()      audioEvent(AU_RSSI_ORANGE)
#define AUDIO_RSSI_RED()         audioEvent(AU_RSSI_RED)
#define AUDIO_RAS_RED()          audioEvent(AU_RAS_RED)
#define AUDIO_TELEMETRY_LOST()   audioEvent(AU_TELEMETRY_LOST)
#define AUDIO_TELEMETRY_BACK()   audioEvent(AU_TELEMETRY_BACK)
#define AUDIO_TRAINER_LOST()     //audioEvent(AU_TRAINER_LOST)
#define AUDIO_TRAINER_BACK()     //audioEvent(AU_TRAINER_BACK)

enum AutomaticPromptsCategories {
  SYSTEM_AUDIO_CATEGORY,
  MODEL_AUDIO_CATEGORY,
  PHASE_AUDIO_CATEGORY,
  SWITCH_AUDIO_CATEGORY,
  LOGICAL_SWITCH_AUDIO_CATEGORY,
};

enum AutomaticPromptsEvents {
  AUDIO_EVENT_OFF,
  AUDIO_EVENT_ON,
  AUDIO_EVENT_MID,
};

enum {
  // IDs for special functions [0:64]
  // IDs for global functions [64:128]
  ID_PLAY_PROMPT_BASE = 179,
  ID_PLAY_FROM_SD_MANAGER = 255,
};

void dfPlayerQueuePlayFile(uint16_t);
void dfPlayerQueueStopPlay(uint16_t);
void pushPrompt(uint16_t prompt, uint8_t id=0);
void pushUnit(uint8_t unit, uint8_t idx, uint8_t id);
void playModelName();

void playModelEvent(uint8_t category, uint8_t index, event_t event=0);
#define PLAY_PHASE_OFF(phase)         playModelEvent(PHASE_AUDIO_CATEGORY, phase, AUDIO_EVENT_OFF)
#define PLAY_PHASE_ON(phase)          playModelEvent(PHASE_AUDIO_CATEGORY, phase, AUDIO_EVENT_ON)
#define PLAY_SWITCH_MOVED(sw)         playModelEvent(SWITCH_AUDIO_CATEGORY, sw)
#define PLAY_LOGICAL_SWITCH_OFF(sw)   playModelEvent(LOGICAL_SWITCH_AUDIO_CATEGORY, sw, AUDIO_EVENT_OFF)
#define PLAY_LOGICAL_SWITCH_ON(sw)    playModelEvent(LOGICAL_SWITCH_AUDIO_CATEGORY, sw, AUDIO_EVENT_ON)
#define PLAY_MODEL_NAME()
#define START_SILENCE_PERIOD()

#define PROMPT_CUSTOM_BASE      0
// #define PROMPT_I18N_BASE        256
// #define PROMPT_SYSTEM_BASE      480

/*
    Use OpenTX/EdgeTX sound packs. ERFly6 have different convention with some fragments joined together
*/
// enum DfplayerEnglishPrompts {
//   DFPLAYER_EN_PROMPT_NUMBERS_BASE = 0,
//   DFPLAYER_EN_PROMPT_ZERO = DFPLAYER_EN_PROMPT_NUMBERS_BASE+0,       //02-99
//   DFPLAYER_EN_PROMPT_HUNDRED = DFPLAYER_EN_PROMPT_NUMBERS_BASE+100,  //100,200 .. 900
//   DFPLAYER_EN_PROMPT_THOUSAND = DFPLAYER_EN_PROMPT_NUMBERS_BASE+109, //1000
//   DFPLAYER_EN_PROMPT_AND = DFPLAYER_EN_PROMPT_NUMBERS_BASE+110,
//   DFPLAYER_EN_PROMPT_MINUS = DFPLAYER_EN_PROMPT_NUMBERS_BASE+111,
//   DFPLAYER_EN_PROMPT_POINT = DFPLAYER_EN_PROMPT_NUMBERS_BASE+112,
//   DFPLAYER_EN_PROMPT_UNITS_BASE = 200,
//   DFPLAYER_EN_PROMPT_POINT_BASE = 167, //.0 - .9
// };

#define I18N_PLAY_FUNCTION(lng, x, ...) void lng ## _ ## x(__VA_ARGS__, uint8_t id)
#define PLAY_FUNCTION(x, ...)           void x(__VA_ARGS__, uint8_t id)

#define PUSH_CUSTOM_PROMPT(p, id)       pushPrompt(PROMPT_CUSTOM_BASE+(p))
#define PUSH_NUMBER_PROMPT(p)           pushPrompt((EN_PROMPT_NUMBERS_BASE + p), id)
#define PUSH_UNIT_PROMPT(p, i)          pushUnit((EN_PROMPT_UNITS_BASE + ((p - 1/*RAW index*/) * 2)), (i), id)
#define PLAY_NUMBER(n, u, a)            playNumber((n), (u), (a), id)
#define PLAY_DURATION(d, att)           playDuration((d), (att), id) 		
#define PLAY_TIME                       1
#define IS_PLAY_TIME()                  (flags&PLAY_TIME)
#define IS_PLAYING(id)                  isPlaying()
#define PLAY_VALUE(v, id)        		playValue((v), (id))
#define PLAY_FILE(f, flags, id)         dfPlayerQueuePlayFile((f))
// #define STOP_PLAY(id)            audioQueue.stopPlay((id))
// #define AUDIO_RESET()            audioQueue.stopAll()
#define AUDIO_FLUSH()               //audioQueue.flush()

#define setScaledVolume(v)

#else // buzzer

extern void pushPrompt(uint16_t prompt);
extern void pushUnit(uint8_t unit, uint8_t idx, uint8_t id);

#define AUDIO_ERROR_MESSAGE(e)   buzzerEvent(e)
#define AUDIO_TIMER_MINUTE(t)    

#define AUDIO_KEY_PRESS()        audioKeyPress()
#define AUDIO_KEY_ERROR()        audioKeyError()

#define AUDIO_HELLO()            PUSH_SYSTEM_PROMPT(AUDIO_HELLO)
#define AUDIO_BYE()              
#define AUDIO_WARNING1()         buzzerEvent(AU_WARNING1)
#define AUDIO_WARNING2()         buzzerEvent(AU_WARNING2)
#define AUDIO_TX_BATTERY_LOW()   buzzerEvent(AU_TX_BATTERY_LOW)
#define AUDIO_ERROR()            buzzerEvent(AU_ERROR)
#define AUDIO_TIMER_COUNTDOWN(idx, val) audioTimerCountdown(idx, val)
#define AUDIO_TIMER_ELAPSED(idx) buzzerEvent(AU_TIMER1_ELAPSED+idx)
#define AUDIO_INACTIVITY()       buzzerEvent(AU_INACTIVITY)
#define AUDIO_MIX_WARNING(x)     buzzerEvent(AU_MIX_WARNING_1+x-1)
#define AUDIO_POT_MIDDLE(x)      buzzerEvent(AU_STICK1_MIDDLE+x)
#define AUDIO_TRIM_MIDDLE()      buzzerEvent(AU_TRIM_MIDDLE)
#define AUDIO_TRIM_MIN()         buzzerEvent(AU_TRIM_MIN)
#define AUDIO_TRIM_MAX()         buzzerEvent(AU_TRIM_MAX)
#define AUDIO_TRIM_PRESS(val)    audioTrimPress(val)
#define AUDIO_PLAY(p)            buzzerEvent(p)
#define AUDIO_VARIO(fq, t, p, f) playTone(fq, t, p, f)
#define AUDIO_RSSI_ORANGE()      buzzerEvent(AU_RSSI_ORANGE)
#define AUDIO_RSSI_RED()         buzzerEvent(AU_RSSI_RED)
#define AUDIO_RAS_RED()          buzzerEvent(AU_RAS_RED)
#define AUDIO_TELEMETRY_LOST()   buzzerEvent(AU_TELEMETRY_LOST)
#define AUDIO_TELEMETRY_BACK()   buzzerEvent(AU_TELEMETRY_BACK)
#define AUDIO_TRAINER_LOST()     //buzzerEvent(AU_TRAINER_LOST)
#define AUDIO_TRAINER_BACK()     //buzzerEvent(AU_TRAINER_BACK)

#define AUDIO_RESET()
#define AUDIO_FLUSH()

#define PLAY_PHASE_OFF(phase)
#define PLAY_PHASE_ON(phase)
#define PLAY_SWITCH_MOVED(sw)
#define PLAY_LOGICAL_SWITCH_OFF(sw)
#define PLAY_LOGICAL_SWITCH_ON(sw)
#define PLAY_MODEL_NAME()
#define START_SILENCE_PERIOD()

#define PROMPT_CUSTOM_BASE      0
#define PROMPT_I18N_BASE        256
#define PROMPT_SYSTEM_BASE      480

#define I18N_PLAY_FUNCTION(lng, x, ...) void lng ## _ ## x(__VA_ARGS__, uint8_t id)
#define PLAY_FUNCTION(x, ...)           void x(__VA_ARGS__, uint8_t id)

#define PUSH_CUSTOM_PROMPT(p, id)       pushPrompt(PROMPT_CUSTOM_BASE+(p))
#define PUSH_NUMBER_PROMPT(p)           pushPrompt(PROMPT_I18N_BASE+(p))
#define PUSH_SYSTEM_PROMPT(p)           pushPrompt(PROMPT_SYSTEM_BASE+(p))
#define PLAY_NUMBER(n, u, a)            
#define PUSH_UNIT_PROMPT(p, i)   		
#define PLAY_DURATION(d, att)    		{}
#define PLAY_TIME
#define IS_PLAY_TIME()                  (0)
#define IS_PLAYING(id)                  isPlaying()
#define PLAY_VALUE(v, id)        		playValue((v), (id))

#define setScaledVolume(v)

#endif // DFPLAYER

#endif // _BUZZER_H_
