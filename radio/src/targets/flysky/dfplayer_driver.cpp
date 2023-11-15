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

static tmr10ms_t dfplayerLastCmdTime = 0;
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

const char * audioNames =
  "\006"
  "acro  "
  "armed "
  "crowof"
  "crowon"
  "disarm"
  "engoff"
  "flapdn"
  "flapup"
  "fm-1  "
  "fm-2  "
  "fm-3  "
  "fm-4  "
  "fm-5  "
  "fm-6  "
  "fm-7  "
  "fm-8  "
  "fm-acr"
  "fm-ang"
  "fm-crs"
  "fm-flt"
  "fm-fst"
  "fm-hor"
  "fm-id1"
  "fm-id2"
  "fm-lch"
  "fm-lnd"
  "fm-nrm"
  "fm-png"
  "fm-pwr"
  "fm-rce"
  "fm-spd"
  "fm-stb"
  "fm-thl"
  "fm-thm"
  "fm-thr"
  "geardn"
  "gearup"
  "lnding"
  "lowbat"
  "lq    "
  "nrmmod"
  "rathi "
  "ratlow"
  "ratmed"
  "rfmode"
  "sigcrt"
  "siglow"
  "spdmod"
  "splrdn"
  "splrup"
  "thmmod"
  "thract"
  "thrcut"
  "tohigh"
  "tolow "
  "trnoff"
  "trnon "
  "vrioff"
  "vrion "
  "custm1"
  "custm2"
  "custm3"
  "custm4"
  "custm5"
  "custm6"
  "custm7"
  "custm8"
  "custm9";

// #define IS_MIN_CMD_DELAY_ELAPSED() (get_tmr10ms() - dfplayerLastCmdTime > 20) // 200 ms
#define IS_MIN_PLAY_DELAY_ELAPSED() (get_tmr10ms() - dfplayerLastCmdTime > 80) // 800 ms

static void dfplayerCommand(uint8_t cmd, uint16_t param = 0) {
    dfplayerLastCmdTime = get_tmr10ms();
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
        aux3SerialPutc(packet[i]);
    }
}

void dfplayerPlayFile(uint16_t number) {
    // dfPlayerCurrentId = id;
    dfplayerCommand(DFP_PLAY, number + 1); // +1 because first file is "zero" and dfplayer uses filesystem index, not filename
}

void dfplayerSetVolume(int8_t volume) {
    uint8_t volumes[5] = { 0, 12, 18, 20, 22 }; // allowed range: 0-30
    //RTOS_WAIT_MS(200);
    dfplayerCommand(DFP_SET_VOLUME, /*((2 + volume) * 6)*/volumes[2 + volume]);
}

// static void dfplayerStopPlay(void) {
//     dfplayerCommand(DFP_PAUSE);
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

    aux3SerialInit();
    dfplayerSetVolume(0);
    
    if (!globalData.unexpectedShutdown) {
        AUDIO_HELLO();
    }
}

bool dfPlayerBusy() {
    return (!IS_MIN_PLAY_DELAY_ELAPSED()) || !GPIO_ReadInputDataBit(DFPLAYER_GPIO_PORT, DFPLAYER_GPIO_PIN_BUSY); // low == playing
}

//bool dfplayerQueueHasPromptId(uint8_t id) {
//    uint8_t i = dfplayerFifo.ridx;
//    while (i != dfplayerFifo.widx) {
//        DfPlayerFragment & fragment = dfplayerFifo.fifo[i];
//        if (fragment.id == id) return true;
//        i = dfplayerFifo.nextIdx(i);
//    }
//    return false;
//}

bool isPlaying(uint8_t id) {
    return false; // (id == dfPlayerCurrentId) || dfplayerQueueHasPromptId(id);
}

char hex(uint8_t b) {
  return b > 9 ? b + 'A' - 10 : b + '0';
}

void debugAudioCall(char a, char b, uint16_t value) {
#if 0
    aux3SerialPutc(a);
    aux3SerialPutc(b);
    aux3SerialPutc(hex((value >> 4) & 0xf));
    aux3SerialPutc(hex(value & 0xf));
    aux3SerialPutc('\n');
#endif
}

void dfPlayerQueuePlayFile(uint16_t index) {
    debugAudioCall('q', 'P', index);
    dfplayerFifo.push(index);
}

//void dfPlayerQueueStopPlay(uint8_t id) {
//    debugAudioCall('q', 'S', id);
//    uint8_t i = dfplayerFifo.ridx;
//    while (i != dfplayerFifo.widx) {
//        DfPlayerFragment & fragment = dfplayerFifo.fifo[i];
//        if (fragment.id == id) {
//            fragment.index = 0;
//            fragment.id = FRAGMENT_EMPTY;
//        }
//        i = dfplayerFifo.nextIdx(i);
//    }
//}

void pushUnit(uint8_t unit, uint8_t idx, uint8_t id)
{
    // idx - plural / singular
    dfPlayerQueuePlayFile(unit + idx);
}

void pushPrompt(uint16_t prompt, uint8_t id)
{
    debugAudioCall('p', 'P', prompt);
    dfPlayerQueuePlayFile(prompt);
}

uint32_t getAudioFileIndex(uint32_t i) 
{
    if ((i <= AU_MODEL_STILL_POWERED) || (i >= AU_TRIM_MIDDLE && i <= AU_TRIM_MAX) || (i >= AU_TIMER1_ELAPSED && i <= DFPLAYER_LAST_FILE_INDEX)) {
        return DFPLAYER_SOUNDS_FILE_INDEX + i;
    }
    return 0;
}

bool isAudioFileReferenced(uint32_t i) 
{
    return (getAudioFileIndex(i) != 0);
}

void audioPlay(unsigned int index)
{
    debugAudioCall('a', 'P', index);
    if (g_eeGeneral.beepMode >= -1) {
    if (isAudioFileReferenced(index)) {
        dfPlayerQueuePlayFile(getAudioFileIndex(index));
    }
  }
}

void playModelEvent(uint8_t category, uint8_t index, event_t event)
{
    debugAudioCall('p', 'M', (category << 8) + index);
    if (IS_SILENCE_PERIOD_ELAPSED() && isAudioFileReferenced(index + event)) {
        dfPlayerQueuePlayFile(index);
    }
}
