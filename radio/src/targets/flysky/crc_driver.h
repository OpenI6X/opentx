
#ifndef _CRC_DRIVER_H_
#define _CRC_DRIVER_H_

#define CRC8_POL_D5   0xD5D5D5D5
#define CRC8_POL_BA   0xBABABABA
#define CRC8_INIT_VAL 0x00

//void crcInit(void);
uint8_t crc8_hw(const uint8_t * ptr, uint32_t len);
uint8_t crc8_BA_hw(const uint8_t * ptr, uint32_t len);
uint16_t crc16_hw(const uint8_t * ptr, uint32_t len);

#endif // _CRC_DRIVER_H_