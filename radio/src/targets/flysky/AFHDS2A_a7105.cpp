/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Multiprotocol is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Multiprotocol.  If not, see <http://www.gnu.org/licenses/>.
 */
// Last sync with hexfet new_protocols/flysky_a7105.c dated 2015-09-28
#include "../../telemetry/flysky_ibus.h"
#include "iface_a7105.h"
#include "opentx.h"

#define AFHDS2A_HUB_TELEMETRY
//#define AFHDS2A_NUMFREQ			16

extern int8_t s_editMode;

static uint8_t num_ch;

inline uint32_t GetChipID(void) {
  return (uint32_t)(READ_REG(*((uint32_t *)UID_BASE))) ^
         (uint32_t)(READ_REG(*((uint32_t *)(UID_BASE + 4U)))) ^
         (uint32_t)(READ_REG(*((uint32_t *)(UID_BASE + 8U))));
}
void EnableGIO(void) {
  EXTI->PR |= RF_GIO2_PIN;
  SET_BIT(EXTI->IMR, RF_GIO2_PIN);
}
void DisableGIO(void) {
  CLEAR_BIT(EXTI->IMR, RF_GIO2_PIN);
}

static void AFHDS2A_calc_channels() {
  uint8_t idx = 0;
  uint32_t rnd = ID.MProtocol_id;
  uint8_t i;
  while (idx < AFHDS2A_NUMFREQ) {
    uint8_t band_no = ((((idx << 1) | ((idx >> 1) & 0b01)) + ID.rx_tx_addr[3]) & 0b11);
    rnd = rnd * 0x0019660D + 0x3C6EF35F;  // Randomization

    uint8_t next_ch = band_no * 41 + 1 + ((rnd >> idx) % 41);  // Channel range: 1..164

    for (i = 0; i < idx; i++) {
      // Keep the distance 5 between the channels
      uint8_t distance;
      if (next_ch > hopping_frequency[i])
        distance = next_ch - hopping_frequency[i];
      else
        distance = hopping_frequency[i] - next_ch;

      if (distance < 5)
        break;
    }

    if (i != idx)
      continue;

    hopping_frequency[idx++] = next_ch;
  }
}

static void AFHDS2A_build_bind_packet(uint8_t * packet) {
  uint8_t ch;
  uint8_t phase = RadioState & 0x0F;
  memcpy(&packet[1], ID.rx_tx_addr, 4);
  memset(&packet[5], 0xff, 4);
  packet[10] = 0x00;
  for (ch = 0; ch < AFHDS2A_NUMFREQ; ch++)
    packet[11 + ch] = hopping_frequency[ch];
  memset(&packet[27], 0xff, 10);
  packet[37] = 0x00;
  switch (phase) {
    case AFHDS2A_BIND1:
      packet[0] = 0xbb;
      packet[9] = 0x01;
      break;
    case AFHDS2A_BIND2:
    case AFHDS2A_BIND3:
    case AFHDS2A_BIND4:
      packet[0] = 0xbc;
      if (phase == AFHDS2A_BIND4) {
        memcpy(&packet[5], &g_eeGeneral.receiverId[g_model.header.modelId[INTERNAL_MODULE]], 4);
        memset(&packet[11], 0xff, 16);
      }
      packet[9] = phase - 1;
      if (packet[9] > 0x02)
        packet[9] = 0x02;
      packet[27] = 0x01;
      packet[28] = 0x80;
      break;
  }
}

void AFHDS2A_build_packet(uint8_t * packet, const uint8_t type) {
  memcpy(&packet[1], ID.rx_tx_addr, sizeof(ID.rx_tx_addr));
  memcpy(&packet[5], &g_eeGeneral.receiverId[g_model.header.modelId[INTERNAL_MODULE]], 4);
  switch (type) {
    case AFHDS2A_PACKET_STICKS:
      packet[0] = 0x58;
      for (uint32_t ch = 0; ch < num_ch; ++ch) {
        // channelOutputs: -1024 to 1024
#if defined(AFHDS2A_LQI_CH)
        const uint16_t channelMicros = (ch == (AFHDS2A_LQI_CH - 1)) ? 
                                           (1000 + 10 * telemetryData.rssi.value) : 
                                           (channelOutputs[ch] / 2 + PPM_CH_CENTER(ch));
#else
        const uint16_t channelMicros = channelOutputs[ch] / 2 + PPM_CH_CENTER(ch);
#endif
        if (ch < 14) {
            packet[9 + ch * 2] = channelMicros & 0xFF;
            packet[10 + ch * 2] = (channelMicros >> 8) & 0x0F;
        } else {
            packet[10 + (ch - 14) * 6] |= (channelMicros ) << 4;
            packet[12 + (ch - 14) * 6] |= (channelMicros ) & 0xF0;
            packet[14 + (ch - 14) * 6] |= (channelMicros >> 4) & 0xF0;
        }
      }
      break;
    case AFHDS2A_PACKET_FAILSAFE:
      packet[0] = 0x56;
      for (uint8_t ch = 0; ch < num_ch; ch++) {
        if (g_model.moduleData[INTERNAL_MODULE].failsafeMode == FAILSAFE_CUSTOM &&
            g_model.moduleData[INTERNAL_MODULE].failsafeChannels[ch] < FAILSAFE_CHANNEL_HOLD) {
          const uint16_t failsafeMicros = g_model.moduleData[INTERNAL_MODULE].failsafeChannels[ch] / 2 + PPM_CH_CENTER(ch);
          packet[9 + ch * 2] = failsafeMicros & 0xff;
          packet[10 + ch * 2] = (failsafeMicros >> 8) & 0xff;
        } else {  // no values
          packet[9 + ch * 2] = 0xff;
          packet[10 + ch * 2] = 0xff;
        }
      }
      break;
    case AFHDS2A_PACKET_SETTINGS:
      packet[0] = 0xaa;
      packet[9] = 0xfd;
      packet[10] = 0xff;
      packet[11] = g_model.moduleData[INTERNAL_MODULE].afhds2a.servoFreq;
      packet[12] = g_model.moduleData[INTERNAL_MODULE].afhds2a.servoFreq >> 8;
      if (g_model.moduleData[INTERNAL_MODULE].subType & (AFHDS2A_SUBTYPE_PPM_IBUS & AFHDS2A_SUBTYPE_PPM_SBUS)) {
        packet[13] = 0x01;  // PPM output enabled
      } else {
        packet[13] = 0x00;
      }
      packet[14] = 0x00;
      for (uint32_t i = 15; i < 37; i++) {
        packet[i] = 0xff;
      }
      packet[18] = 0x05;  // ?
      packet[19] = 0xdc;  // ?
      packet[20] = 0x05;  // ?
      if (g_model.moduleData[INTERNAL_MODULE].subType & (AFHDS2A_SUBTYPE_PWM_SBUS & AFHDS2A_SUBTYPE_PPM_SBUS)) {
        packet[21] = 0xdd;  // SBUS output enabled
      } else {
        packet[21] = 0xde;  // IBUS
      }
      break;
  }
  if (hopping_frequency_no >= AFHDS2A_NUMFREQ)
    packet[37] = 0x00;
  else
    packet[37] = 0;  //hopping_frequency_no+2;
}

void ActionAFHDS2A(void) {
  uint8_t Channel;
  static uint8_t packet_type;

  uint8_t txPacket[AFHDS2A_TXPACKET_SIZE];
  uint8_t *rxPacket = (uint8_t *)&telemetryRxBuffer;

  static uint16_t packet_counter = 0;
#if defined(AFHDS2A_FREQ_TUNING)
  A7105_AdjustLOBaseFreq();
#endif

  if (moduleState[INTERNAL_MODULE].mode == MODULE_MODE_BIND) {
    if (IS_BIND_DONE && IS_BIND_STOP) {
      TRACE("Binding in progress...");
      // __disable_irq(); crashes
      RadioState = ((TIM_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_BIND1));
      BIND_IN_PROGRESS;
      BIND_START;
    } else {
      if (IS_BIND_DONE) {
        TRACE("Bind done!");
        moduleState[INTERNAL_MODULE].mode = MODULE_MODE_NORMAL;
        s_editMode = EDIT_SELECT_MENU;
        storageDirty(EE_GENERAL);  // Save receiverId
        BIND_STOP;
      }
    }
  } else {
    if (IS_BIND_IN_PROGRESS || IS_BIND_START) {
      TRACE("Bind cancelled.");
      BIND_DONE;
      BIND_STOP;
      // __enable_irq();
    } else if (IS_BIND_DONE) {
      BIND_STOP;
      // __enable_irq();
    }
  }
  if (moduleState[INTERNAL_MODULE].mode == MODULE_MODE_RANGECHECK && !IS_RANGE_FLAG_on) {
    RANGE_FLAG_on;
  }
  if (moduleState[INTERNAL_MODULE].mode != MODULE_MODE_RANGECHECK && IS_RANGE_FLAG_on) {
    RANGE_FLAG_off;
  }
  //----------------------------------------------------------------------------
  if (IS_BIND_DONE) {
    RadioState = (RadioState & 0xF0) | AFHDS2A_DATA;
  }
  switch (RadioState) {
    case ((TIM_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_BIND1)):
    case ((TIM_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_BIND2)):
    case ((TIM_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_BIND3)):
    case ((TIM_CALL << CALLER) | (RES << SEND_RES) | (AFHDS2A_BIND1)):
    case ((TIM_CALL << CALLER) | (RES << SEND_RES) | (AFHDS2A_BIND2)):
    case ((TIM_CALL << CALLER) | (RES << SEND_RES) | (AFHDS2A_BIND3)):
      goto SendBIND_;

    case ((TIM_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_BIND4)):
      goto SendBIND4_;

    case ((GPIO_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_BIND1)):
    case ((GPIO_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_BIND2)):
    case ((GPIO_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_BIND3)):
      goto EndSendBIND123_;

    case ((GPIO_CALL << CALLER) | (RES << SEND_RES) | (AFHDS2A_BIND1)):
    case ((GPIO_CALL << CALLER) | (RES << SEND_RES) | (AFHDS2A_BIND2)):
    case ((GPIO_CALL << CALLER) | (RES << SEND_RES) | (AFHDS2A_BIND3)):
      goto ResBIND123_;

    case ((TIM_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_DATA)):
    case ((TIM_CALL << CALLER) | (RES << SEND_RES) | (AFHDS2A_DATA)):
      goto SendData_;

    case ((GPIO_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_DATA)):
      goto EndSendData_;

    case ((GPIO_CALL << CALLER) | (RES << SEND_RES) | (AFHDS2A_DATA)):
      goto ResData_;

    default:
      return;
  }
//--------------------------------------------------------------------------
SendBIND4_:  //--------------------------------------------------------------
  bind_phase++;
  if (bind_phase >= 4) {
    hopping_frequency_no = 1;
    RadioState = (RadioState & 0xF0) | AFHDS2A_DATA;
    SETBIT(RadioState, SEND_RES, SEND);
    BIND_DONE;
    return;
  }
SendBIND_:  //--------------------------------------------------------------
  AFHDS2A_build_bind_packet((uint8_t *)&txPacket);
  Channel = (packet_count % 2 ? 0x0d : 0x8c);
  SETBIT(RadioState, SEND_RES, SEND);
  goto Send_;
EndSendBIND123_:  //-----------------------------------------------------------
  A7105_SetPower();
  A7105_SetTxRxMode(TXRX_OFF);  // Turn LNA off since we are in near range and we want to prevent swamping
  A7105_Strobe(A7105_RX);
  EnableGIO();
  RadioState++;
  if ((RadioState & 0x0F) > AFHDS2A_BIND3)
    RadioState = (RadioState & 0xF0) | AFHDS2A_BIND1;
  SETBIT(RadioState, SEND_RES, RES);
  return;
ResBIND123_:  //-----------------------------------------------------------
  if (!(A7105_ReadReg(A7105_00_MODE) & (1<<5))) { // CRCF Ok
    A7105_ReadData(rxPacket, AFHDS2A_RXPACKET_SIZE);
    if ((rxPacket[0] == 0xbc) & (rxPacket[9] == 0x01)) {
        memcpy(&g_eeGeneral.receiverId[g_model.header.modelId[INTERNAL_MODULE]], &rxPacket[5], 4);
        RadioState = (RadioState & 0xF0) | AFHDS2A_BIND4;
        bind_phase = 0;
        SETBIT(RadioState, SEND_RES, SEND);
    }
  }
  return;
SendData_:  //--------------------------------------------------------------
  Channel = hopping_frequency[hopping_frequency_no++];
  AFHDS2A_build_packet((uint8_t *)&txPacket, packet_type);
  SETBIT(RadioState, SEND_RES, SEND);
  if (hopping_frequency_no >= AFHDS2A_NUMFREQ) {
    hopping_frequency_no = 0;
    goto SendNoAntSwitch_;
  }
  goto Send_;
EndSendData_:  //-----------------------------------------------------------
  A7105_SetPower();
  A7105_SetTxRxMode(RX_EN);
  A7105_Strobe(A7105_RX);
  if (!(packet_counter % 1569))
    packet_type = AFHDS2A_PACKET_FAILSAFE;
  else
    packet_type = AFHDS2A_PACKET_STICKS;
  SETBIT(RadioState, SEND_RES, RES);
  EnableGIO();
  SETBIT(RadioState, SEND_RES, RES);
  return;
ResData_:  //-----------------------------------------------------------
  if (!(A7105_ReadReg(A7105_00_MODE) & (1<<5))) { // CRCF Ok
    A7105_ReadData(rxPacket, AFHDS2A_RXPACKET_SIZE);
    if (rxPacket[0] == 0xAA && rxPacket[9] == 0xFC)  // RX is asking for settings
      packet_type = AFHDS2A_PACKET_SETTINGS;
    else if (rxPacket[0] == 0xAA && rxPacket[9] == 0xFD)  // RX is asking for FailSafe
      packet_type = AFHDS2A_PACKET_FAILSAFE;
    else if (rxPacket[0] == 0xAA || rxPacket[0] == 0xAC) {
      if (!memcmp(&rxPacket[1], ID.rx_tx_addr, 4) && // Validate TX address
        !memcmp(&rxPacket[5], &g_eeGeneral.receiverId[g_model.header.modelId[INTERNAL_MODULE]], 4)) {  // Validate RX address
        if (rxPacket[0] == 0xAA) {
          int16_t tx_rssi = 256 - (A7105_ReadReg(A7105_1D_RSSI_THOLD) * 8) / 5;  // value from A7105 is between 8 for maximum signal strength to 160 or less
          tx_rssi = limit<int16_t>(0, tx_rssi, 255);
          rxPacket[8] = tx_rssi;
        }
        pendingTelemetryPollFrame = true;
      }
    }
    SETBIT(RadioState, SEND_RES, SEND);
  }
  return;
Send_:  //---------------------------------------------------------------
  A7105_AntSwitch();
SendNoAntSwitch_:
  A7105_WriteData((uint8_t *)&txPacket, AFHDS2A_TXPACKET_SIZE, Channel);
  EnableGIO();
  packet_count++;
  packet_counter++;
  return;
}

void initAFHDS2A() {
  RadioState = ((TIM_CALL << CALLER) | (SEND << SEND_RES) | (AFHDS2A_DATA));
  ID.MProtocol_id = GetChipID();
  AFHDS2A_calc_channels();
  A7105_Init();
  packet_count = 0;
  hopping_frequency_no = 0;
  if (g_model.moduleData[INTERNAL_MODULE].subType & 0x04) {
    num_ch = 17;
  } else {
    num_ch = 14;
  }
  BIND_STOP;
  BIND_DONE;
}
