#include "iface_a7105.h"
#include "opentx.h"

/*---------------------------------------------------------------------------*/
void ActionAFHDS(void) {
  A7105_AdjustLOBaseFreq();
  packet[0] = IS_BIND_IN_PROGRESS ? 0xaa : 0x55;
  packet[1] = ID.rx_tx_addr[3];
  packet[2] = ID.rx_tx_addr[2];
  packet[3] = ID.rx_tx_addr[1];
  packet[4] = ID.rx_tx_addr[0];
  for (uint32_t ch = 0; ch < 8; ch++) {
    uint16_t channelMicros = channelOutputs[ch] / 2 + RADIO_PPM_CENTER;
    packet[5 + ch * 2] = channelMicros & 0xFF;         // low byte of servo timing(1000-2000us)
    packet[6 + ch * 2] = (channelMicros >> 8) & 0xFF;  // high byte of servo timing(1000-2000us)
  }
  A7105_SetPower();
  A7105_WriteData(21, IS_BIND_IN_PROGRESS ? 0x01 : hopping_frequency[hopping_frequency_no & 0x0F]);
  hopping_frequency_no++;
  if (hopping_frequency_no % 16)
    A7105_AntSwitch();
}
/*---------------------------------------------------------------------------*/
const uint8_t AFHDS_tx_channels[8][4] = {
    {0x12, 0x34, 0x56, 0x78},
    {0x18, 0x27, 0x36, 0x45},
    {0x41, 0x82, 0x36, 0x57},
    {0x84, 0x13, 0x65, 0x72},
    {0x87, 0x64, 0x15, 0x32},
    {0x76, 0x84, 0x13, 0x52},
    {0x71, 0x62, 0x84, 0x35},
    {0x71, 0x86, 0x43, 0x52}};
/*---------------------------------------------------------------------------*/
void initAFHDS(void) {
  uint8_t chanrow;
  uint8_t chanoffset;
  uint8_t temp;

  A7105_Init();
  ID.MProtocol_id = GetChipID();
  // limit offset to 9 as higher values don't work with some RX (ie V912)
  // limit offset to 9 as CX20 repeats the same channels after that
  if ((ID.rx_tx_addr[3] & 0xF0) > 0x90)
    ID.rx_tx_addr[3] = ID.rx_tx_addr[3] - 0x70;

  // Build frequency hop table
  chanrow = ID.rx_tx_addr[3] & 0x0F;
  chanoffset = ID.rx_tx_addr[3] / 16;
  for (uint8_t i = 0; i < 16; i++) {
    temp = AFHDS_tx_channels[chanrow >> 1][i >> 2];
    if (i & 0x02)
      temp &= 0x0F;
    else
      temp >>= 4;
    temp *= 0x0A;
    if (i & 0x01)
      temp += 0x50;
    hopping_frequency[((chanrow & 1) ? 15 - i : i)] = temp - chanoffset;
  }
  SetPRTTimPeriod(PROTO_AFHDS_SPI);
  hopping_frequency_no = 0;
  packet_count = 0;
}
