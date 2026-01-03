
#ifndef _CRC_DRIVER_H_
#define _CRC_DRIVER_H_

#define CRC8_POL_D5   0xD5D5D5D5
#define CRC8_POL_BA   0xBABABABA
#define CRC8_INIT_VAL 0x00

#define CRC16_POL_1021 0x1021
#define CRC16_INIT_VAL 0xFFFF

uint8_t crc8(const uint8_t * ptr, uint32_t len);
uint8_t crc8_BA(const uint8_t * ptr, uint32_t len);
uint16_t crc16(const uint8_t * ptr, uint32_t len);

#endif // _CRC_DRIVER_H_