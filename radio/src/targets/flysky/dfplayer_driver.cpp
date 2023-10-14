/*
 * Copyright (C) OpenI6X
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

/**
 * Add 1K resistor to dfplayer rx to reduce noise
*/
#include "opentx.h"
#include "board.h"

tmr10ms_t dfplayerLastPlayTime = 0;
Fifo<uint16_t, 16> dfplayerFifo;

static constexpr uint8_t DFP_PLAY         = 0x03; // root,         file: 0-2999, plays by filesystem order, fastest
static constexpr uint8_t DFP_PLAY_MP3     = 0x12; // folder: mp3,  file: 0-2999, plays by filename, too slow
static constexpr uint8_t DFP_PLAY_FOLDER  = 0x14; // folder: 0-15, file: 0-2999, plays by filename, too slow
static constexpr uint8_t DFP_PLAY_FOLDER2 = 0x0F; // folder: 1-99, file: 0- 255, plays by filename, too slow
static constexpr uint8_t DFP_PAUSE        = 0x0E;
static constexpr uint8_t DFP_SET_VOLUME   = 0x06; // volume: 0-30
static constexpr uint8_t DFP_STANDBY      = 0x0A;
static constexpr uint8_t DFP_WAKEUP       = 0x0B;
static constexpr uint8_t DFP_RESET        = 0x0C;

static void dfplayerCommand(uint8_t cmd, uint16_t param = 0) {
    uint8_t packet[8];
    packet[0] = 0x7E;         // start
    packet[1] = 0xFF;         // version
    packet[2] = 0x06;         // len
    packet[3] = cmd;          // cmd
    packet[4] = 0x00;         // feedback
    packet[5] = param >> 8;   // param1
    packet[6] = param & 0xFF; // param2
    packet[7] = 0xEF;         // end
    for (uint8_t i = 0; i < 8; i++) {
        aux2SerialPutc(packet[i]);
    }
}

// Use folder because files in root are not handled by name, but order in filesystem
void dfplayerPlayFile(uint16_t number) {
    dfplayerLastPlayTime = get_tmr10ms();
    // const uint8_t folder = 1;
    dfplayerCommand(DFP_PLAY, number + 1); // +1 because first file is "zero"
}

static void dfplayerSetVolume(uint8_t volume) {
//    uint8_t volumes[5] = { 0, 6, 12, 18, 24 }; // allowed range: 0-30
    dfplayerCommand(DFP_SET_VOLUME, ((2 + volume) * 6)/*volumes[2 + volume]*/);
}

static void dfplayerStopPlay(void) {
    dfplayerCommand(DFP_PAUSE);
}

// static void dfplayerStandby(void) {
//     dfplayerCommand(DFP_STANDBY);
// }

// static void dfplayerWakeup(void) {
//     dfplayerCommand(DFP_WAKEUP);
// }

void dfplayerInit() {
    // setup BUSY pin
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = DFPLAYER_GPIO_PIN_BUSY;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_Init(DFPLAYER_GPIO_PORT, &GPIO_InitStructure);

    // wait for module to init 1.5s
    RTOS_WAIT_MS(1500);
    // wait for openTX init for this value to be ready, now it is 0
    // -> call init after eeprom data is ready
    dfplayerSetVolume(g_eeGeneral.wavVolume);
}

#define IS_MIN_DELAY_PERIOD_ELAPSED() (get_tmr10ms() - dfplayerLastPlayTime > 80) // 800 ms

bool isPlaying() {
    return (!IS_MIN_DELAY_PERIOD_ELAPSED()) || !GPIO_ReadInputDataBit(DFPLAYER_GPIO_PORT, DFPLAYER_GPIO_PIN_BUSY); // low == playing
}

char hex(uint8_t b) {
  return b > 9 ? b + 'A' - 10 : b + '0';
}

void debugAudioCall(char a, char b, uint16_t value) {
    // aux2SerialPutc(a);
    // aux2SerialPutc(b);
    // aux2SerialPutc(hex((value >> 4) & 0xf));
    // aux2SerialPutc(hex(value & 0xf));
    // aux2SerialPutc('\n');
}

void dfPlayerQueuePlayFile(uint16_t index) {
    debugAudioCall('q', 'P', index);
    dfplayerFifo.push(index);
}

void dfPlayerQueueStopPlay(uint16_t index) {
    debugAudioCall('q', 'S', index);
    // todo remove from queue by prompt
    // if playing prompt is the same then stop
    if (isPlaying()) {
        dfplayerStopPlay();
    }
}

// Wysyłany indeks jest niestety bez form pojedynczych/mnogich z TelemetryUnit
// wszystkie jednostki maja takie wersje wiec wystarczy robić * 2 dla plural i tak stworzyc index plików
void pushUnit(uint8_t unit, uint8_t idx, uint8_t id)
{
    // unit - unit
    // idx - plural / singular
    if (idx == 0) debugAudioCall('p', 'u', unit);
    else debugAudioCall('p', 'U', unit);

    // EN_PROMPT_UNITS_BASE + (TelemetryUnit[x]*2) , idx (singular/plural)
    // EN_PROMPT_UNITS_BASE) * 2 + idx
    dfPlayerQueuePlayFile(unit + idx);
}

void pushPrompt(uint16_t prompt, uint8_t id)
{
    debugAudioCall('p', 'P', prompt);
    dfPlayerQueuePlayFile(prompt);
}

#define DFPLAYER_LAST_FILE_INDEX 500
bool isAudioFileReferenced(uint32_t i) { // check if index in valid range
    return i < DFPLAYER_LAST_FILE_INDEX;
}

void audioPlay(unsigned int index, uint8_t id)
{
    debugAudioCall('a', 'P', index);
  if (g_eeGeneral.beepMode >= -1) {
    // if (isAudioFileReferenced(index)) {
        dfPlayerQueuePlayFile(index); // + audio filenames offset
//     }
  }
    
}

// order from AUDIO_SOUNDS
// offset must be added in buzzerSound, not here
void audioEvent(unsigned int index)
{
    debugAudioCall('a', 'E', index);
    buzzerEvent(index); // rename to audioEvent
}

/*
 - 
 - 3x = SWITCH_AUDIO_CATEGORY + 0-1
*/
void playModelEvent(uint8_t category, uint8_t index, event_t event)
{
    debugAudioCall('p', 'M', (category << 8) + index);
    if (IS_SILENCE_PERIOD_ELAPSED() && isAudioFileReferenced((category << 24) + (index << 16) + event)) {
      dfPlayerQueuePlayFile(index); // + audio filenames offset
    }
}
