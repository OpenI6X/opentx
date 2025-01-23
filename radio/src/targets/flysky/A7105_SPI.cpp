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
/********************/
/** A7105 routines **/
/********************/
#include "opentx.h"

#include "stdint.h"
#include "iface_a7105.h"
#include "hal.h"
volatile uint8_t RadioState;

uint8_t protocol_flags=0;
uint8_t prev_power=0xFD; // unused power value

//Protocol variables
ID_t ID;
uint8_t  packet_count = 0;
uint8_t  bind_phase;
uint8_t  hopping_frequency[AFHDS2A_NUMFREQ];
uint8_t  hopping_frequency_no;


/******************************************************************************/
/*                           Radio control                                    */
/******************************************************************************/
const uint8_t AFHDS2A_A7105_regs[] = {
      0xFF, 0xC2 | (1<<5), 0x00, 0x25, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,0x19/*GIO1 SDO*/, 0x01/*GIO2 WTR*/, 0x05, 0x00, 0x50,// 00 - 0f
      0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x4f, 0x62, 0x80, 0xFF, 0xFF, 0x2a, 0x32, 0xc3, 0x1E/*0x1f*/,  // 10 - 1f
      0x1e, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,	       // 20 - 2f
      0x01, 0x0f // 30 - 31
};

void SPI_Write(uint8_t command)
{
	while((SPI1->SR & SPI_SR_BSY));
	*(__IO uint8_t *)&SPI1->DR = command;					// Write the first data item to be transmitted into the SPI_DR register (this clears the TXE flag).
	#ifdef DEBUG_SPI
		debug("%02X ",command);
	#endif
	while (!(SPI1->SR & SPI_SR_RXNE));
	command = (uint8_t)SPI1->DR;					// ... and read the last received data.
}

uint8_t SPI_SDI_Read() 
{
	uint8_t rx=0;
	// uint8_t dummy;
    // while ((SPI1->SR & SPI_SR_FRLVL) != 0) { // not present in multi
    //   dummy = (uint8_t)(READ_REG(SPI1->DR));
    // }
    // (void)dummy;
	while(!(SPI1->SR & SPI_SR_TXE));
	while((SPI1->SR & SPI_SR_BSY));

    *(__IO uint8_t *)&SPI1->DR = 0x00; // not present in multi
    while(!(SPI1->SR & SPI_SR_RXNE));
    rx=(uint8_t)SPI1->DR;
	return rx;
}
/*---------------------------------------------------------------------------*/
inline void a7105_csn_on(void) {RF_SCN_GPIO_PORT->BSRR = RF_SCN_SET_PIN;}
inline void a7105_csn_off(void) {RF_SCN_GPIO_PORT->BSRR = RF_SCN_RESET_PIN;}
inline void RF0_SetVal(void) {RF_RF0_GPIO_PORT->BSRR = RF_RF0_SET_PIN;}
inline void RF0_ClrVal(void) {RF_RF0_GPIO_PORT->BSRR = RF_RF0_RESET_PIN;}
inline void RF1_SetVal(void) {RF_RF1_GPIO_PORT->BSRR = RF_RF1_SET_PIN;}
inline void RF1_ClrVal(void) {RF_RF1_GPIO_PORT->BSRR = RF_RF1_RESET_PIN;}
inline void TX_RX_PutVal(uint32_t Val) {
  uint8_t tmp = 0;
  tmp  = (Val << 1) & 0x02;
  tmp |= (Val >> 1) & 0x01;
  RF_RxTx_GPIO_PORT->BSRR = (uint32_t)((RF_RxTx_PIN_MASK << 16) | (tmp << 8));
}

// Channel value -125%<->125% is scaled to 16bit value with no limit
//int16_t convert_channel_16b_nolimit(uint8_t num, int16_t min, int16_t max)
//{
//	int32_t val=Channel_data[num];				// 0<->2047
//	val=(val-CHANNEL_MIN_100)*(max-min)/(CHANNEL_MAX_100-CHANNEL_MIN_100)+min;
//	return (uint16_t)val;
//}

void A7105_AntSwitch(void) {
	static uint8_t sw = 0;
	switch (sw) {

	case 0:
		RF1_ClrVal();
		RF0_SetVal();
		sw = 1;
		return;
	case 1:
		RF0_ClrVal();
		RF1_SetVal();
		sw = 0;
		return;
	}

}

void A7105_SetTxRxMode(uint8_t mode) {
	TX_RX_PutVal(mode);
}

void A7105_Strobe(uint8_t address) {
  A7105_CSN_off;
  SPI_Write(address);
  A7105_CSN_on;
}

void A7105_WriteData(uint8_t * data, uint8_t len, uint8_t channel) {
	uint8_t i;
	A7105_CSN_off;
	SPI_Write(A7105_RST_WRPTR);
	SPI_Write(A7105_05_FIFO_DATA);
	for (i = 0; i < len; i++)
		SPI_Write(data[i]);
	A7105_CSN_on;
	// if(protocol!=PROTO_WFLY2)
	// {
		// if(!(protocol==PROTO_FLYSKY || (protocol==PROTO_KYOSHO && sub_protocol==KYOSHO_HYPE)))
		// {
			A7105_Strobe(A7105_STANDBY);	//Force standby mode, ie cancel any TX or RX...
			A7105_SetTxRxMode(TX_EN);		//Switch to PA
		// }
		A7105_WriteReg(A7105_0F_PLL_I, channel);
		A7105_Strobe(A7105_TX);
	// }
}

void A7105_ReadData(uint8_t * data, uint8_t len) {
	uint8_t i;
	A7105_Strobe(A7105_RST_RDPTR);
	A7105_CSN_off;
	SPI_Write(0x40 | A7105_05_FIFO_DATA);	//bit 6 =1 for reading
	for (i=0;i<len;i++)
		data[i]=SPI_SDI_Read();
	A7105_CSN_on;
}

void A7105_WriteReg(uint8_t address, uint8_t data) {
	A7105_CSN_off;
	SPI_Write(address); 
//	NOP();
	SPI_Write(data);  
	A7105_CSN_on;
}

uint8_t A7105_ReadReg(uint8_t address) {
	uint8_t result;
	A7105_CSN_off;
	SPI_Write(address |=0x40);				//bit 6 =1 for reading
	result = SPI_SDI_Read();  
	A7105_CSN_on;
	return(result); 
}

uint8_t A7105_Reset() {
	uint8_t result;
	
	A7105_WriteReg(A7105_00_MODE, 0x00);
	// delay_ms(1);
	A7105_SetTxRxMode(TXRX_OFF);			             //Set both GPIO as output and low
	result=A7105_ReadReg(A7105_10_PLL_II) == 0x9E; //check if is reset.
	A7105_Strobe(A7105_STANDBY);
	return result;
}

void A7105_WriteID(uint32_t ida) {
	A7105_CSN_off;
	SPI_Write(A7105_06_ID_DATA);			//ex id=0x5475c52a ;txid3txid2txid1txid0
	SPI_Write((ida>>24)&0xff);				//54 
	SPI_Write((ida>>16)&0xff);				//75
	SPI_Write((ida>>8)&0xff);				//c5
	SPI_Write((ida>>0)&0xff);				//2a
	A7105_CSN_on;
}

void A7105_SetPower()
{
	uint8_t power=A7105_BIND_POWER;
	if(IS_BIND_DONE)
		#ifdef A7105_ENABLE_LOW_POWER
			power=IS_POWER_FLAG_on?A7105_HIGH_POWER:A7105_LOW_POWER;
		#else
			power=A7105_HIGH_POWER;
		#endif
	if(IS_RANGE_FLAG_on)
		power=A7105_RANGE_POWER;
	if(prev_power != power)
	{
		A7105_WriteReg(A7105_28_TX_TEST, power);
		prev_power=power;
	}
}


// Fine tune A7105 LO base frequency
// this is required for some A7105 modules and/or RXs with inaccurate crystal oscillator
void A7105_AdjustLOBaseFreq(void)
{
	static int16_t old_offset=2048;
	int16_t offset = 0; // g_model.FreqOffset; -300 <-> 300
	if(old_offset==offset)	// offset is the same as before...
			return;
	old_offset=offset;

	// LO base frequency = 32e6*(bip+(bfp/(2^16)))
	uint8_t bip;	// LO base frequency integer part
	uint16_t bfp;	// LO base frequency fractional part
	offset++;		// as per datasheet, not sure why recommended, but that's a +1kHz drift only ...
	offset<<=1;
	if(offset < 0)
	{
		bip = 0x4a;	// 2368 MHz
		bfp = 0xffff + offset;
	}
	else
	{
		bip = 0x4b;	// 2400 MHz (default)
		bfp = offset;
	}
	A7105_WriteReg( A7105_11_PLL_III, bip);
	A7105_WriteReg( A7105_12_PLL_IV, (bfp >> 8) & 0xff);
	A7105_WriteReg( A7105_13_PLL_V, bfp & 0xff);
}

static void __attribute__((unused)) A7105_SetVCOBand(uint8_t vb1, uint8_t vb2)
{	// Set calibration band value to best match
	uint8_t diff1, diff2;

	if (vb1 >= 4)
		diff1 = vb1 - 4;
	else
		diff1 = 4 - vb1;

	if (vb2 >= 4)
		diff2 = vb2 - 4;
	else
		diff2 = 4 - vb2;

	if (diff1 == diff2 || diff1 > diff2)
		A7105_WriteReg(A7105_25_VCO_SBCAL_I, vb1 | 0x08);
	else
		A7105_WriteReg(A7105_25_VCO_SBCAL_I, vb2 | 0x08);
}

// #ifdef AFHDS2A_A7105_INO
// //const uint8_t AFHDS2A_A7105_regs[] = {
// //	0xFF, 0x42 | (1<<5), 0x00, 0x25, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,0x01, 0x3c, 0x05, 0x00, 0x50, // 00 - 0f
// //	0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x4f, 0x62, 0x80, 0xFF, 0xFF, 0x2a, 0x32, 0xc3, 0x1E/*0x1f*/,	 // 10 - 1f
// //	0x1e, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,			 // 20 - 2f
// //	0x01, 0x0f // 30 - 31
// //};
// #endif
// #ifdef BUGS_A7105_INO
// const uint8_t PROGMEM BUGS_A7105_regs[] = {
// 	0xFF, 0x42, 0x00, 0x15, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0x01, 0x50,	// 00 - 0f
// 	0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x40, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x0f,	// 10 - 1f
// 	0x16, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x3b, 0x00, 0x0b, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,	// 20 - 2f
// 	0x01, 0x0f // 30 - 31
// };
// #endif
// #ifdef FLYSKY_A7105_INO
// const uint8_t PROGMEM FLYSKY_A7105_regs[] = {
// 	0xff, 0x42, 0x00, 0x14, 0x00, 0xff, 0xff ,0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 0x05, 0x00, 0x50,	// 00 - 0f
// 	0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x00, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x0f,	// 10 - 1f
// 	0x13, 0xc3, 0x00, 0xff, 0x00, 0x00, 0x3b, 0x00, 0x17, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,	// 20 - 2f
// 	0x01, 0x0f // 30 - 31
// };
// #endif
// #ifdef FLYZONE_A7105_INO
// const uint8_t PROGMEM FLYZONE_A7105_regs[] = {
// 	0xff, 0x42, 0x00, 0x07, 0x00, 0xff, 0xff ,0x00, 0x00, 0x00, 0x00, 0x01, 0x21, 0x05, 0x01, 0x50,	// 00 - 0f
// 	0x9e, 0x4b, 0x00, 0x02, 0x16, 0x2b, 0x12, 0x00, 0x62, 0x80, 0x80, 0x00, 0x0a, 0x32, 0xc3, 0x1f,	// 10 - 1f
// 	0x12, 0x00, 0x00, 0xff, 0x00, 0x00, 0x3a, 0x00, 0x3f, 0x47, 0x80, 0x03, 0x01, 0x45, 0x18, 0x00,	// 20 - 2f
// 	0x01, 0x0f // 30 - 31
// };
// #endif
// #ifdef HUBSAN_A7105_INO
// const uint8_t PROGMEM HUBSAN_A7105_regs[] = {
// 	0xFF, 0x63, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF ,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x04, 0xFF,	// 00 - 0f
// 	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x2B, 0xFF, 0xFF, 0x62, 0x80, 0xFF, 0xFF, 0x0A, 0xFF, 0xFF, 0x07,	// 10 - 1f
// 	0x17, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x47, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 20 - 2f
// 	0xFF, 0xFF // 30 - 31
// };
// #endif

void A7105_Sleep(void) {
	A7105_WriteReg(A7105_0B_GPIO1_PIN1, 0x33);
	A7105_WriteReg(A7105_0C_GPIO2_PIN_II, 0x33);
}

#define ID_NORMAL  0x55201041 // used by Multiprotocol Module
#define ID_NORMAL2 0x5475C52A // used by DeviationTX & erFly6
void A7105_Init(void)
{
	uint8_t *A7105_Regs = 0;
//	uint8_t vco_calibration0, vco_calibration1;

  A7105_Reset();
  delay_ms(1);

	A7105_WriteID(ID_NORMAL2);

	A7105_Regs = (uint8_t*) AFHDS2A_A7105_regs;

	for (uint32_t i = 0; i < 0x32; i++) {
		uint8_t val = A7105_Regs[i];
		if (val != 0xFF)
			A7105_WriteReg(i, val);
	}

	while (A7105_ReadReg(A7105_10_PLL_II) != 0x9E);

  // Calibration
	A7105_Strobe(A7105_PLL);

	//IF Filter Bank Calibration
	A7105_WriteReg(A7105_02_CALC, 1);
	while (A7105_ReadReg(A7105_02_CALC));       // Wait for calibration to end
	//VCO Current Calibration
	A7105_WriteReg(A7105_24_VCO_CURCAL, 0x13);  //Recommended calibration from A7105 Datasheet
	//VCO Bank Calibration
	A7105_WriteReg(A7105_26_VCO_SBCAL_II, 0x3b);//Recommended calibration from A7105 Datasheet

	//VCO Bank Calibrate channel 0
	A7105_WriteReg(A7105_0F_CHANNEL, 0);
	A7105_WriteReg(A7105_02_CALC, 2);
	while (A7105_ReadReg(A7105_02_CALC));       // Wait for calibration to end

    //VCO Bank Calibrate channel A0
	A7105_WriteReg(A7105_0F_CHANNEL, 0xa0);
	A7105_WriteReg(A7105_02_CALC, 2);
	while (A7105_ReadReg(A7105_02_CALC));       // Wait for calibration to end

	A7105_WriteReg(A7105_25_VCO_SBCAL_I, 0x0A);	//Reset VCO Band calibration

	A7105_SetTxRxMode(TX_EN);
	A7105_SetPower();

	A7105_Strobe(A7105_STANDBY);
}
