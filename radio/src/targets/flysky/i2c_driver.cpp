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
#include "opentx.h"
#include "board.h"

void eepromPageWrite(uint8_t* pBuffer, uint16_t WriteAddr, uint8_t NumByteToWrite);
void eepromWaitEepromStandbyState(void);

void i2cInit()
{
  I2C_DeInit(I2C);

 GPIO_InitTypeDef GPIO_InitStructure;
//  GPIO_InitStructure.GPIO_Pin = I2C_WP_GPIO_PIN;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
//  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//  GPIO_Init(I2C_WP_GPIO, &GPIO_InitStructure);
//  GPIO_ResetBits(I2C_WP_GPIO, I2C_WP_GPIO_PIN);

  I2C_InitTypeDef I2C_InitStructure;
  I2C_InitStructure.I2C_Timing = I2C_TIMING_400K;
  // I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;
  // I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x00;
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  // I2C_InitStructure.I2C_AnalogFilter = I2C_AnalogFilter_Enable;
  // I2C_InitStructure.I2C_DigitalFilter = 0x00;
  I2C_Init(I2C, &I2C_InitStructure);
  I2C_Cmd(I2C, ENABLE);

  GPIO_PinAFConfig(I2C_GPIO, I2C_SCL_GPIO_PinSource, I2C_GPIO_AF);
  GPIO_PinAFConfig(I2C_GPIO, I2C_SDA_GPIO_PinSource, I2C_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = I2C_SCL_GPIO_PIN | I2C_SDA_GPIO_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(I2C_GPIO, &GPIO_InitStructure);
}

/**
 
  *** Polling Mode ***
  ====================
    [..] In Polling Mode, the I2C communication can be managed by 15 flags:
        (#) I2C_FLAG_TXE: to indicate the status of Transmit data register empty flag.
        (#) I2C_FLAG_TXIS: to indicate the status of Transmit interrupt status flag .
        (#) I2C_FLAG_RXNE: to indicate the status of Receive data register not empty flag.
        (#) I2C_FLAG_ADDR: to indicate the status of Address matched flag (slave mode).
        (#) I2C_FLAG_NACKF: to indicate the status of NACK received flag.
        (#) I2C_FLAG_STOPF: to indicate the status of STOP detection flag.
        (#) I2C_FLAG_TC: to indicate the status of Transfer complete flag(master mode).
        (#) I2C_FLAG_TCR: to indicate the status of Transfer complete reload flag.
        (#) I2C_FLAG_BERR: to indicate the status of Bus error flag.
        (#) I2C_FLAG_ARLO: to indicate the status of Arbitration lost flag.
        (#) I2C_FLAG_OVR: to indicate the status of Overrun/Underrun flag.
        (#) I2C_FLAG_PECERR: to indicate the status of PEC error in reception flag.
        (#) I2C_FLAG_TIMEOUT: to indicate the status of Timeout or Tlow detection flag.
        (#) I2C_FLAG_ALERT: to indicate the status of SMBus Alert flag.
        (#) I2C_FLAG_BUSY: to indicate the status of Bus busy flag.

    [..] In this Mode it is advised to use the following functions:
        (+) FlagStatus I2C_GetFlagStatus(I2C_TypeDef* I2Cx, uint32_t I2C_FLAG);
        (+) void I2C_ClearFlag(I2C_TypeDef* I2Cx, uint32_t I2C_FLAG);

    [..]
        (@)Do not use the BUSY flag to handle each data transmission or reception.It is 
           better to use the TXIS and RXNE flags instead.
*/
#define I2C_TIMEOUT_MAX 1000
bool I2C_WaitEvent(uint32_t event)
{
  uint32_t timeout = I2C_TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(I2C, event)) {
    if ((timeout--) == 0) return false;
  }
  return true;
}

bool I2C_WaitEventCleared(uint32_t event)
{
  uint32_t timeout = I2C_TIMEOUT_MAX;
  while (I2C_GetFlagStatus(I2C, event)) {
    if ((timeout--) == 0) return false;
  }
  return true;
}

void eepromPageRead(uint8_t *buffer, size_t address, size_t size) {
  uint8_t wb[2];
  wb[0] = (uint8_t)(address >> 8);
  wb[1] = (uint8_t)(address & 0xFF);
  // uint8_t start_page = address / 64;
  // uint8_t end_page = (address + size -1) / 64;
  // TRACE("eepromPageRead addr %d size %d [from %d to %d]", address, size, start_page, end_page);
  i2c_transfer7(I2C2, I2C_ADDRESS_EEPROM, wb, 2, buffer, size);
#ifdef RTOS_WAIT_MS
  RTOS_WAIT_MS(1);
#else
  delay_ms(1);
#endif

  //DUMP(buffer, size);
}

void eepromPageWrite(uint8_t *buffer, uint16_t address, uint8_t size) {
  static uint8_t temp[2 + I2C_FLASH_PAGESIZE];
  temp[0] = (uint8_t)(address >> 8);
  temp[1] = (uint8_t)(address & 0xFF);
  // uint8_t start_page = address / 64;
  // uint8_t end_page = (address + size - 1) / 64;
  // TRACE("eepromPageWrite addr %d size %d [start page %d end page %d]", address, size, start_page, end_page);
  memcpy(temp + 2, buffer, size);
  //DUMP(temp, size + 2);
  i2c_transfer7(I2C2, I2C_ADDRESS_EEPROM, temp, size + 2, NULL, 0);

#ifdef RTOS_WAIT_MS
  RTOS_WAIT_MS(5);
#else
  delay_ms(5);
#endif

#if defined(EEPROM_VERIFY_WRITES)
  eepromPageRead(temp, address, size);
  for (int i = 0; i < size; i++) {
    if (temp[i] != buffer[i]) {
      TRACE("--------- eeprom verify failed  ----------");
      while (1)
        ;
    }
  }

#endif
}

void eepromReadBlock(uint8_t * buffer, size_t address, size_t size)
{
  while (!I2C_EE_ReadBlock(buffer, address, size)) {
    i2cInit();
  }
}

/**
  * @brief  Writes buffer of data to the I2C EEPROM.
  * @param  buffer : pointer to the buffer containing the data to be
  *   written to the EEPROM.
  * @param  address : EEPROM's internal address to write to.
  * @param  size : number of bytes to write to the EEPROM.
  * @retval None
  */
void eepromWriteBlock(uint8_t * buffer, size_t address, size_t size)
{
  uint8_t offset = address % I2C_FLASH_PAGESIZE;
  uint8_t count = I2C_FLASH_PAGESIZE - offset;
  if (size < count) {
    count = size;
  }
  while (count > 0) {
    eepromPageWrite(buffer, address, count);
    eepromWaitEepromStandbyState(); // TODO
    address += count;
    buffer += count;
    size -= count;
    count = I2C_FLASH_PAGESIZE;
    if (size < I2C_FLASH_PAGESIZE) {
      count = size;
    }
  }
}

uint8_t eepromIsTransferComplete() {
  return 1;
}

void i2c_test() {
  static uint8_t temp[128];
  uint8_t i;
  for (i = 0; i < 128; i++) {
    temp[i] = i;
  }
  DUMP(temp, 128);
  eepromWriteBlock(temp, 10, 128);
  memset(temp, 0, 128);
  eepromReadBlock(temp, 10, 128);
}
