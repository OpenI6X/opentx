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
#include <stdint.h>

#ifndef _IFACE_A7105_H_
#define _IFACE_A7105_H_
#define _BV(bit) (1 << (bit))

#define TX_EN            0b00000010
#define RX_EN            0b00000001
#define TXRX_OFF         0b00000011
#define AFHDS2A_NUMFREQ	 16
#define RADIO_PPM_CENTER 1500

#define AFHDS2A_TXPACKET_SIZE 38
#define AFHDS2A_RXPACKET_SIZE 37

//#define FORCE_AFHDS2A_TUNING 0
enum A7105_POWER
{
	A7105_POWER_0 = 0x00<<3 | 0x00,	// TXPOWER_100uW  = -23dBm == PAC=0 TBG=0
	A7105_POWER_1 = 0x00<<3 | 0x01,	// TXPOWER_300uW  = -20dBm == PAC=0 TBG=1
	A7105_POWER_2 = 0x00<<3 | 0x02,	// TXPOWER_1mW    = -16dBm == PAC=0 TBG=2
	A7105_POWER_3 = 0x00<<3 | 0x04,	// TXPOWER_3mW    = -11dBm == PAC=0 TBG=4
	A7105_POWER_4 = 0x01<<3 | 0x05,	// TXPOWER_10mW   =  -6dBm == PAC=1 TBG=5
	A7105_POWER_5 = 0x02<<3 | 0x07,	// TXPOWER_30mW   =   0dBm == PAC=2 TBG=7
	A7105_POWER_6 = 0x03<<3 | 0x07,	// TXPOWER_100mW  =   1dBm == PAC=3 TBG=7
	A7105_POWER_7 = 0x03<<3 | 0x07	// TXPOWER_150mW  =   1dBm == PAC=3 TBG=7
};

#define A7105_HIGH_POWER	A7105_POWER_7
#define	A7105_LOW_POWER		A7105_POWER_3
#define	A7105_RANGE_POWER	A7105_POWER_0
#define	A7105_BIND_POWER	A7105_POWER_0

//Channel MIN MAX values
#define CHANNEL_MAX_100	1844	//	100%
#define CHANNEL_MIN_100	204		//	100%
#define CHANNEL_MAX_125	2047	//	125%
#define CHANNEL_MIN_125	0		//	125%

//Channel definitions
//#define	CH1	0
//#define	CH2	1
//#define	CH3	2
//#define	CH4	3
//#define	CH5	4
//#define	CH6	5
//#define	CH7	6
//#define	CH8	7
//#define	CH9	8
//#define	CH10	9
//#define	CH11	10
//#define	CH12	11
//#define	CH13	12
//#define	CH14	13
//#define	CH15	14
//#define	CH16	15

extern uint8_t protocol_flags;
extern uint8_t protocol;
extern uint8_t prev_power; // unused power value

// #define RX_RSSI 0
// #define RX_Err  1
// #define RX_IntV 2
// #define RX_ExtV 3
// #define RX_Temp 4
// #define RX_RPM  5

extern uint8_t *packet;
extern uint8_t *packet_in;

extern volatile bool pendingTelemetryPollFrame;

//#define NUM_CHN 16
//// Servo data
//extern uint16_t Channel_data[NUM_CHN];

// Protocol variables

typedef union {
	uint32_t MProtocol_id; //tx id,
	uint8_t rx_tx_addr[4];
}ID_t;

extern ID_t ID;


//extern uint32_t MProtocol_id;//tx id,
extern uint8_t  packet_count;
extern uint8_t  phase;
extern uint8_t  bind_phase;
extern uint8_t  hopping_frequency[AFHDS2A_NUMFREQ];
extern uint8_t  hopping_frequency_no;
extern uint8_t  option;   // option value should be between 0 and 70 which gives a value between 50 and 400Hz
extern volatile uint8_t RadioState;

#define BIND_IN_PROGRESS	protocol_flags &= ~_BV(7)
#define BIND_DONE			protocol_flags |= _BV(7)
#define BIND_START			protocol_flags |= _BV(6)
#define BIND_STOP       	protocol_flags &= ~_BV(6)

#define IS_BIND_DONE		( ( protocol_flags & _BV(7) ) !=0 )
#define IS_BIND_IN_PROGRESS	( ( protocol_flags & _BV(7) ) ==0 )
#define IS_BIND_START		( ( protocol_flags & _BV(6) ) !=0 )
#define IS_BIND_STOP       	( ( protocol_flags & _BV(6) ) ==0 )

#define RANGE_FLAG_on		protocol_flags |= _BV(3)
#define RANGE_FLAG_off		protocol_flags &= ~_BV(3)
#define IS_RANGE_FLAG_on	( ( protocol_flags & _BV(3) ) !=0 )

enum A7105_State {
    A7105_SLEEP     = 0x80,
    A7105_IDLE      = 0x90,
    A7105_STANDBY   = 0xA0,
    A7105_PLL       = 0xB0,
    A7105_RX        = 0xC0,
    A7105_TX        = 0xD0,
    A7105_RST_WRPTR = 0xE0,
    A7105_RST_RDPTR = 0xF0,
};

enum {
    A7105_00_MODE         = 0x00,
    A7105_01_MODE_CONTROL = 0x01,
    A7105_02_CALC         = 0x02,
    A7105_03_FIFOI        = 0x03,
    A7105_04_FIFOII       = 0x04,
    A7105_05_FIFO_DATA    = 0x05,
    A7105_06_ID_DATA      = 0x06,
    A7105_07_RC_OSC_I     = 0x07,
    A7105_08_RC_OSC_II    = 0x08,
    A7105_09_RC_OSC_III   = 0x09,
    A7105_0A_CK0_PIN      = 0x0A,
    A7105_0B_GPIO1_PIN1   = 0x0B,
    A7105_0C_GPIO2_PIN_II = 0x0C,
    A7105_0D_CLOCK        = 0x0D,
    A7105_0E_DATA_RATE    = 0x0E,
    A7105_0F_PLL_I        = 0x0F,
    A7105_10_PLL_II       = 0x10,
    A7105_11_PLL_III      = 0x11,
    A7105_12_PLL_IV       = 0x12,
    A7105_13_PLL_V        = 0x13,
    A7105_14_TX_I         = 0x14,
    A7105_15_TX_II        = 0x15,
    A7105_16_DELAY_I      = 0x16,
    A7105_17_DELAY_II     = 0x17,
    A7105_18_RX           = 0x18,
    A7105_19_RX_GAIN_I    = 0x19,
    A7105_1A_RX_GAIN_II   = 0x1A,
    A7105_1B_RX_GAIN_III  = 0x1B,
    A7105_1C_RX_GAIN_IV   = 0x1C,
    A7105_1D_RSSI_THOLD   = 0x1D,
    A7105_1E_ADC          = 0x1E,
    A7105_1F_CODE_I       = 0x1F,
    A7105_20_CODE_II      = 0x20,
    A7105_21_CODE_III     = 0x21,
    A7105_22_IF_CALIB_I   = 0x22,
    A7105_23_IF_CALIB_II  = 0x23,
    A7105_24_VCO_CURCAL   = 0x24,
    A7105_25_VCO_SBCAL_I  = 0x25,
    A7105_26_VCO_SBCAL_II = 0x26,
    A7105_27_BATTERY_DET  = 0x27,
    A7105_28_TX_TEST      = 0x28,
    A7105_29_RX_DEM_TEST_I  = 0x29,
    A7105_2A_RX_DEM_TEST_II = 0x2A,
    A7105_2B_CPC          = 0x2B,
    A7105_2C_XTAL_TEST    = 0x2C,
    A7105_2D_PLL_TEST     = 0x2D,
    A7105_2E_VCO_TEST_I   = 0x2E,
    A7105_2F_VCO_TEST_II  = 0x2F,
    A7105_30_IFAT         = 0x30,
    A7105_31_RSCALE       = 0x31,
    A7105_32_FILTER_TEST  = 0x32,
};
#define A7105_0F_CHANNEL A7105_0F_PLL_I

enum A7105_MASK {
    A7105_MASK_FBCF = 1 << 4,
    A7105_MASK_VBCF = 1 << 3,
};

enum ePaketType{
	AFHDS2A_PACKET_STICKS,
	AFHDS2A_PACKET_SETTINGS,
	AFHDS2A_PACKET_FAILSAFE,
};

enum ePhase {
	AFHDS2A_BIND1 = 0,
	AFHDS2A_BIND2 = 1,
	AFHDS2A_BIND3 = 2,
	AFHDS2A_BIND4 = 3,
	AFHDS2A_DATA  = 4,
};

#define CALLER    4
#define TIM_CALL  0
#define GPIO_CALL 1

#define SEND_RES  5
#define SEND      0
#define RES       1

#define PASS_INERRUPT  6
#define NOT_PASS       0
#define PASS           1

void A7105_Sleep(void);
void A7105_Init(void);
void A7105_AdjustLOBaseFreq(void);
void A7105_ReadData(uint8_t len);
uint8_t A7105_ReadReg(uint8_t address);
void A7105_WriteReg(uint8_t address, uint8_t data);
void A7105_SetPower();
void A7105_SetTxRxMode(uint8_t mode);
void A7105_Strobe(uint8_t address);
void A7105_WriteData(uint8_t len, uint8_t channel);
void A7105_AntSwitch(void);

#endif
