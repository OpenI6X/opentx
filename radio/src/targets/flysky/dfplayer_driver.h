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

#ifndef _DFPLAYER_DRIVER_H_
#define _DFPLAYER_DRIVER_H_

#if defined(__cplusplus)

#define DFPLAYER_SOUNDS_FILE_INDEX  179 // AUDIO_SOUNDS
#define DFPLAYER_CUSTOM_FILE_INDEX  216 // 216 to 267 + 34 user custom ones
#define DFPLAYER_LAST_FILE_INDEX    (271 + 9) // 271 + 9 custom placeholders 

/* struct DfPlayerFragment {
    uint16_t index;
    uint8_t id;
    DfPlayerFragment(uint16_t index, uint8_t id) : index(index), id(id) {};
    DfPlayerFragment() {};
}; */

extern const char * audioNames;

void dfplayerPlayFile(uint16_t number);
void dfplayerInit(void);
void dfplayerSetVolume(int8_t);
bool dfPlayerBusy(void);
void dfPlayerQueuePlayFile(uint16_t);
// void dfPlayerQueueStopPlay(uint8_t id);

#endif // __cplusplus

#endif // _DFPLAYER_DRIVER_H_