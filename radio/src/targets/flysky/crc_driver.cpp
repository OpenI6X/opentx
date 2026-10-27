/**
 * According to: https://jeelabs.org/2018/hardware-crc
 * HW CRC can be 16x faster than lookup table implementation
 *
 */
#include "opentx.h"
#include "crc_driver.h"

#if defined(SIMU)
// No CRC peripheral on the host: compute what the hardware unit is configured to compute
// below (MSB first, no input/output reversal, polynomial in the low `bits` of POL).
static uint32_t crcSoftware(const uint8_t * ptr, uint32_t len, uint32_t init, uint32_t poly, uint8_t bits)
{
  const uint32_t mask = (1u << bits) - 1;
  const uint32_t top = 1u << (bits - 1);
  uint32_t crc = init & mask;
  poly &= mask;
  while (len--) {
    crc ^= (uint32_t)(*ptr++) << (bits - 8);
    for (int i = 0; i < 8; i++)
      crc = ((crc & top) ? (crc << 1) ^ poly : crc << 1) & mask;
  }
  return crc;
}

uint8_t crc8(const uint8_t * ptr, uint32_t len)
{
  return crcSoftware(ptr, len, CRC8_INIT_VAL, CRC8_POL_D5, 8);
}

uint8_t crc8_BA(const uint8_t * ptr, uint32_t len)
{
  return crcSoftware(ptr, len, CRC8_INIT_VAL, CRC8_POL_BA, 8);
}

uint16_t crc16(const uint8_t * ptr, uint32_t len)
{
  return crcSoftware(ptr, len, CRC16_INIT_VAL, CRC16_POL_1021, 16);
}
#else
uint8_t crc8(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = CRC8_INIT_VAL;
  CRC->POL = CRC8_POL_D5;
  CRC->CR = LL_CRC_POLYLENGTH_8B;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = (*ptr++);
  }
  return (uint8_t)(CRC->DR);
}

uint8_t crc8_BA(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = CRC8_INIT_VAL;
  CRC->POL = CRC8_POL_BA;
  CRC->CR = LL_CRC_POLYLENGTH_8B;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = (*ptr++);
  }
  return (uint8_t)(CRC->DR);
}

// Used by FLYSKY_GIMBAL
uint16_t crc16(const uint8_t * ptr, uint32_t len) {
  CRC->INIT = CRC16_INIT_VAL;
  CRC->POL = CRC16_POL_1021;
  CRC->CR = LL_CRC_POLYLENGTH_16B;
  CRC->CR |= CRC_CR_RESET;
  for (uint32_t i = 0; i < len; i++) {
    *(__IO uint8_t*)(CRC_BASE) = (*ptr++);
  }
  return (uint16_t)(CRC->DR);
}
#endif // defined(SIMU)
