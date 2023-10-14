/**
 * According to: https://jeelabs.org/2018/hardware-crc
 * HW CRC can be 16x faster than lookup table implementation
 * 
 * Only crc8, (crc16 is used in frsky fw update)
 */
#include "stm32f0xx_crc.h"
#include "crc_driver.h"

uint8_t crc8_hw(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = CRC8_INIT_VAL;
  CRC->POL = CRC8_POL_D5;
  CRC->CR = CRC_PolSize_8;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = (*ptr++);
  }
  return (uint8_t)(CRC->DR);
}

uint8_t crc8_BA_hw(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = CRC8_INIT_VAL;
  CRC->POL = CRC8_POL_BA;
  CRC->CR = CRC_PolSize_8;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = *ptr++;
  }
  return (uint8_t)(CRC->DR);
}

uint16_t crc16_hw(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = 0xFFFF;
  CRC->POL = 0x1021;
  CRC->CR = CRC_PolSize_16;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = (*ptr++);
  }
  return (uint16_t)(CRC->DR);
}
