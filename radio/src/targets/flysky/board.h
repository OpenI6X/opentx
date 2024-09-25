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

#ifndef _BOARD_H_
#define _BOARD_H_

#include "stddef.h"
#include "stdbool.h"
#include "flysky_gimbal_driver.h"

#if defined(__cplusplus) && !defined(SIMU)
extern "C" {
#endif

#if __clang__
// clang is very picky about the use of "register"
// tell it to ignore for the STM32 includes instead of modyfing the orginal files
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif

#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/CMSIS/Device/ST/STM32F0xx/Include/stm32f0xx.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_crs.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_rcc.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_gpio.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_tim.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_adc.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_spi.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_i2c.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_rtc.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_pwr.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_dma.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_usart.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_flash.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_dbgmcu.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_misc.h"
#include "STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries/STM32F0xx_StdPeriph_Driver/inc/stm32f0xx_crc.h"

#if __clang__
// Restore warnings about registers
#pragma clang diagnostic pop
#endif

#include "usb_driver.h"
#if !defined(SIMU)
  #include "usbd_cdc_core.h"
  #include "usbd_msc_core.h"
  #include "usbd_hid_core.h"
  #include "usbd_usr.h"
  #include "usbd_desc.h"
  #include "usb_conf.h"
  #include "usbd_conf.h"
#endif

#include "hal.h"

#if defined(__cplusplus) && !defined(SIMU)
}
#endif

#if defined(STM32F0)
#define VECTOR_TABLE_SIZE (48)                    // 31 positive vectors, 0 vector, 7 negative vectors and 9 extra
#define SYSCFG_CFGR1_MEM_MODE__MAIN_FLASH      0  // x0: Main Flash memory mapped at 0x0000 0000
#define SYSCFG_CFGR1_MEM_MODE__SYSTEM_FLASH    1  // 01: System Flash memory mapped at 0x0000 0000
#define SYSCFG_CFGR1_MEM_MODE__SRAM            3  // 11: Embedded SRAM mapped at 0x0000 0000
#endif

#define FLASHSIZE                       0x20000  // 128 kb
#define BOOTLOADER_SIZE                 0x4000   //  16 kb
#define FIRMWARE_ADDRESS                0x08000000

#define LUA_MEM_MAX                     (0)    // max allowed memory usage for complete Lua  (in bytes), 0 means unlimited


#define PERI1_FREQUENCY               48000000
#define PERI2_FREQUENCY               48000000

#define TIMER_MULT_APB1                 1 //2
#define TIMER_MULT_APB2                 1 //2

#define strcpy_P strcpy
#define strcat_P strcat

#define BATTERY_WARN                  45 // 4.5V
#define BATTERY_MIN                   43 // 4.3V
#define BATTERY_MAX                   60 // 6.0V

//maybe bind?
#define IS_SHIFT_KEY(index)             (false)
#define IS_SHIFT_PRESSED()              (false)

extern uint16_t sessionTimer;

// Board driver
void boardInit(void);
void boardOff(void);

// Timers driver
void init2MhzTimer();
void init5msTimer();

// Delays driver
#ifdef __cplusplus
extern "C" {
#endif
// void delaysInit(void);
void delay_01us(uint16_t nb);
void delay_us(uint16_t nb);
void delay_ms(uint32_t ms);
#ifdef __cplusplus
}
#endif

// #if !defined(BOOT)
// #define usbPlugged() (false)
// #define usbStarted() (false)
// #define getSelectedUsbMode() (USB_UNSELECTED_MODE)
// #endif


// CPU Unique ID
#define LEN_CPU_UID                     (3*8+2)
void getCPUUniqueID(char * s);

// SD driver
#define BLOCK_SIZE                      512 /* Block Size in Bytes */
#if !defined(SIMU) || defined(SIMU_DISKIO)
uint32_t sdIsHC(void);
uint32_t sdGetSpeed(void);
#define SD_IS_HC()                      (sdIsHC())
#define SD_GET_SPEED()                  (sdGetSpeed())
#define SD_GET_FREE_BLOCKNR()           (sdGetFreeSectors())
#else
#define SD_IS_HC()                      (0)
#define SD_GET_SPEED()                  (0)
#endif
#define __disk_read                     disk_read
#define __disk_write                    disk_write
#if defined(SIMU) || !defined(SDCARD)
  #if !defined(SIMU_DISKIO)
    #define sdInit()
    #define sdDone()
  #endif
  #define sdMount()
  #define SD_CARD_PRESENT()               true
#else
void sdInit(void);
void sdMount(void);
void sdDone(void);
void sdPoll10ms(void);
uint32_t sdMounted(void);
#define SD_CARD_PRESENT()               ((SD_GPIO_PRESENT_GPIO->IDR & SD_GPIO_PRESENT_GPIO_PIN) == 0)
#endif

//buzzer
#if !defined(BOOT)
#include "buzzer_driver.h"
#define BUZZER_HEARTBEAT buzzerHeartbeat
#endif

#if defined(DFPLAYER)
#include "dfplayer_driver.h"
#endif

// Flash Write driver
#define FLASH_PAGESIZE 256
void unlockFlash(void);
void lockFlash(void);
void flashWrite(uint32_t * address, uint32_t * buffer);
uint32_t isFirmwareStart(const uint8_t * buffer);
uint32_t isBootloaderStart(const uint8_t * buffer);

// Pulses driver
#define INTERNAL_MODULE_ON()            {}
#define INTERNAL_MODULE_OFF()           {}
#ifdef EXTPWR_INVERT
#define EXTERNAL_MODULE_OFF()           EXTMODULE_PWR_GPIO->BSRR = EXTMODULE_PWR_GPIO_PIN // GPIO_SetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN)
#define EXTERNAL_MODULE_ON()            EXTMODULE_PWR_GPIO->BRR = EXTMODULE_PWR_GPIO_PIN // GPIO_ResetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN)
#else
#define EXTERNAL_MODULE_ON()            EXTMODULE_PWR_GPIO->BSRR = EXTMODULE_PWR_GPIO_PIN // GPIO_SetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN)
#define EXTERNAL_MODULE_OFF()           EXTMODULE_PWR_GPIO->BRR = EXTMODULE_PWR_GPIO_PIN // GPIO_ResetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN)
#endif
#define IS_INTERNAL_MODULE_ON()         (false)
#define IS_EXTERNAL_MODULE_ON()         (GPIO_ReadInputDataBit(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN) == Bit_SET)
#if defined(INTMODULE_USART)
  #define IS_UART_MODULE(port)          (port == INTERNAL_MODULE)
#else
  #define IS_UART_MODULE(port)          false
#endif

void init_no_pulses(uint32_t port);
void disable_no_pulses(uint32_t port);
void init_ppm( uint32_t module_index );
void disable_ppm( uint32_t module_index );
void init_pxx( uint32_t module_index );
void disable_pxx( uint32_t module_index );
void init_serial( uint32_t module_index, uint32_t baudrate, uint32_t period);
void disable_serial( uint32_t module_index);
void init_module_timer( uint32_t module_index, uint32_t period, uint8_t state);
void disable_module_timer( uint32_t module_index);

//jsut to allow compilation
void setupPulsesSbus(uint8_t port);
void extmoduleSendNextFrame();
//void intmoduleSendNextFrame();

// Trainer driver
#define SLAVE_MODE()                    (false) // (g_model.trainerMode == TRAINER_MODE_SLAVE)
#define TRAINER_CONNECTED()           (true)

#if defined(TRAINER_GPIO)
  void init_trainer_capture(void);
  void stop_trainer_capture(void);
#else
  #define init_trainer_capture()
#endif
#if defined(TRAINER_MODULE_HEARTBEAT)
  void init_cppm_on_heartbeat_capture(void);
  void stop_cppm_on_heartbeat_capture(void);
  void init_sbus_on_heartbeat_capture(void);
  void stop_sbus_on_heartbeat_capture(void);
#else
  #define init_cppm_on_heartbeat_capture()
  #define stop_cppm_on_heartbeat_capture()
  #define init_sbus_on_heartbeat_capture()
  #define stop_sbus_on_heartbeat_capture()
#endif

// SBUS
int sbusGetByte(uint8_t * byte);

// Keys driver
enum EnumKeys
{
  KEY_MENU,
  KEY_ENTER = KEY_MENU,
  KEY_EXIT,
  KEY_DOWN,
  KEY_MINUS = KEY_DOWN,
  KEY_UP,
  KEY_PLUS = KEY_UP,
  KEY_RIGHT,
  KEY_LEFT,
  KEY_BIND,
  TRM_BASE,
  TRM_LH_DWN = TRM_BASE,
  TRM_LH_UP,
  TRM_LV_DWN,
  TRM_LV_UP,
  TRM_RV_DWN,
  TRM_RV_UP,
  TRM_RH_DWN,
  TRM_RH_UP,
  TRM_LAST = TRM_RH_UP,
  NUM_KEYS
};

enum EnumSwitches
{
  SW_SA,
  SW_SB,
  SW_SC,
  SW_SD,
  NUM_SWITCHES
};

enum EnumSwitchesPositions
{
  SW_SA0,
  SW_SA1,
  SW_SA2,
  SW_SB0,
  SW_SB1,
  SW_SB2,
  SW_SC0,
  SW_SC1,
  SW_SC2,
  SW_SD0,
  SW_SD1,
  SW_SD2,
};
#define IS_3POS(x)            ((x) == SW_SC)
#define IS_TOGGLE(x)					false
#define NUM_SWITCHES          4

void keysInit(void);
uint8_t keyState(uint8_t index);
uint32_t switchState(uint8_t index);
uint32_t readKeys(void);
uint32_t readTrims(void);

#define TRIMS_PRESSED()                 (readTrims())
#define KEYS_PRESSED()                  (readKeys())

#define BOOTLOADER_KEYS                0x2100

#define NUM_TRIMS                      4
#define NUM_TRIMS_KEYS                 (NUM_TRIMS * 2)

#define NUM_MOUSE_ANALOGS              0
#define NUM_DUMMY_ANAS                 0

// WDT driver
#define WDTO_500MS                      500
#if defined(WATCHDOG_DISABLED) || defined(SIMU)
  #define wdt_enable(x)
  #define wdt_reset()
#else
  #define wdt_enable(x)                 watchdogInit(x)
  #define wdt_reset()                   IWDG->KR = 0xAAAA
#endif
#define wdt_disable()
void watchdogInit(unsigned int duration);
#define WAS_RESET_BY_SOFTWARE()             (RCC_GetFlagStatus(RCC_FLAG_SFTRST))
#define WAS_RESET_BY_WATCHDOG()             (RCC_GetFlagStatus(RCC_FLAG_WWDGRST) || RCC_GetFlagStatus(RCC_FLAG_IWDGRST) )
#define WAS_RESET_BY_WATCHDOG_OR_SOFTWARE() (RCC_GetFlagStatus(RCC_FLAG_SFTRST) || RCC_GetFlagStatus(RCC_FLAG_WWDGRST) || RCC_GetFlagStatus(RCC_FLAG_IWDGRST))

// ADC driver
enum Analogs {
  STICK1,
  STICK2,
  STICK3,
  STICK4,
  SW_A,   
  SW_B,
  SW_C,
  SW_D,
  POT_FIRST,
  POT1 = POT_FIRST,
  POT2,
  POT_LAST = POT2,
  TX_VOLTAGE,
  NUM_ANALOGS
};

#define NUM_POTS                        (POT_LAST-POT_FIRST+1)
#define NUM_XPOTS                       0						//disable xpot for now
#define NUM_SLIDERS                     0

enum CalibratedAnalogs {
  CALIBRATED_STICK1,
  CALIBRATED_STICK2,
  CALIBRATED_STICK3,
  CALIBRATED_STICK4,
  CALIBRATED_POT_FIRST,
  CALIBRATED_POT1=CALIBRATED_POT_FIRST,
  CALIBRATED_POT2,
  CALIBRATED_POT_LAST = CALIBRATED_POT2,
  NUM_CALIBRATED_ANALOGS
};

#define IS_POT(x)                     ((x)>=POT_FIRST && (x)<=POT_LAST)
#define IS_SLIDER(x)                  ((x)>POT_LAST && (x)<TX_VOLTAGE)
void adcInit(void);
void adcRead(void);
extern uint16_t adcValues[NUM_ANALOGS];
uint16_t getAnalogValue(uint8_t index);
uint16_t* getAnalogValues();
uint16_t getBatteryVoltage();   // returns current battery voltage in 10mV steps

#define BATT_SCALE                    150

#if defined(__cplusplus) && !defined(SIMU)
extern "C" {
#endif

// Power driver
#define SOFT_PWR_CTRL
void pwrInit(void);
uint32_t pwrCheck(void);
void pwrOn(void);
void pwrOff(void);
uint32_t pwrPressed(void);
#if defined(PWR_PRESS_BUTTON)
uint32_t pwrPressedDuration(void);
#endif

#if defined(SIMU)
#define UNEXPECTED_SHUTDOWN()           false
#else
#define UNEXPECTED_SHUTDOWN()           (WAS_RESET_BY_WATCHDOG())
#endif

// Backlight driver
void backlightInit(void);
void backlightDisable(void);
#define BACKLIGHT_DISABLE()             backlightDisable()
#define BACKLIGHT_FORCED_ON             101
uint8_t isBacklightEnabled(void);
void backlightEnable(uint8_t level);
#define BACKLIGHT_ENABLE()            backlightEnable(currentBacklightBright)

#if !defined(SIMU)
  void usbJoystickUpdate();
#endif
#define USB_NAME                        "FS-i6X"
#define USB_MANUFACTURER                'F', 'l', 'y', 'S', 'k', 'y', ' ', ' '  /* 8 bytes */
#define USB_PRODUCT                     'F', 'S', '-', 'i', '6', 'X', ' ', ' '  /* 8 Bytes */

#if defined(__cplusplus) && !defined(SIMU)
}
#endif

// I2C driver: EEPROM
#define EEPROM_SIZE                   (16*1024)

void i2cInit(void);
void eepromReadBlock(uint8_t * buffer, size_t address, size_t size);
void eepromWriteBlock(uint8_t * buffer, size_t address, size_t size);
uint8_t eepromIsTransferComplete();

// Debug driver
void debugPutc(const char c);

// Telemetry driver
void telemetryPortInit(uint32_t baudrate, uint8_t mode);
void telemetryPortSetDirectionOutput(void);
//void sportSendBuffer(uint8_t * buffer, uint32_t count);
void sportSendBuffer(const uint8_t* buffer, unsigned long count);
uint8_t telemetryGetByte(uint8_t * byte);
// extern uint32_t telemetryErrors;

extern volatile bool pendingTelemetryPollFrame;

// Audio driver
void initBuzzerTimer(void);
void audioInit(void);
void audioEnd(void);
void dacStart(void);
void dacStop(void);

#define VOLUME_LEVEL_MAX  23
#define VOLUME_LEVEL_DEF  12
#if !defined(SOFTWARE_VOLUME)
void setScaledVolume(uint8_t volume);
void setVolume(uint8_t volume);
int32_t getVolume(void);
#endif
void audioConsumeCurrentBuffer();
void setSampleRate(uint32_t frequency);
void referenceSystemAudioFiles();
#define audioDisableIrq()               __disable_irq()
#define audioEnableIrq()                __enable_irq()

// Haptic driver
void hapticInit(void);
void hapticOff(void);

// Second serial port driver
#if defined(AUX_SERIAL_GPIO)
#define DEBUG_BAUDRATE                  115200
#define AUX_SERIAL
extern uint8_t auxSerialMode;
void auxSerialInit(unsigned int mode, unsigned int protocol);
void auxSerialPutc(char c);
#define auxSerialTelemetryInit(protocol) //auxSerialInit(UART_MODE_TELEMETRY, protocol)
void auxSerialSbusInit(void);
void auxSerialStop(void);
#endif

// Aux2 serial port driver
#if defined(FLYSKY_GIMBAL)
#define AUX4_SERIAL
#define AUX4_SERIAL_BAUDRATE FLYSKY_HALL_BAUDRATE // 921600
#define AUX4_SERIAL_RXFIFO_SIZE HALLSTICK_BUFF_SIZE // 128
void flysky_gimbal_init();
#endif
#if defined(DFPLAYER)
#define AUX3_SERIAL
#define AUX3_SERIAL_BAUDRATE DFPLAYER_BAUDRATE
#endif

#if defined(AUX3_SERIAL)
// extern uint8_t aux2SerialMode;
// #if defined __cplusplus
// void aux2SerialSetup(unsigned int baudrate, bool dma, uint16_t length = USART_WordLength_8b, uint16_t parity = USART_Parity_No, uint16_t stop = USART_StopBits_1);
// #endif
void aux3SerialInit(void);
void aux3SerialPutc(char c);
#endif

#if defined(AUX4_SERIAL)
void aux4SerialInit(void);
void aux4SerialStop(void);
void aux4SerialSetIdleCb(void (*cb)());
#endif

#define USART_FLAG_ERRORS (USART_FLAG_ORE | USART_FLAG_PE) // | USART_FLAG_FE, USART_FLAG_NE

// LCD driver
#define LCD_W                           128
#define LCD_H                           64
#define LCD_DEPTH                       1
#define LCD_CONTRAST_MIN                30
#define LCD_CONTRAST_MAX                63
#define LCD_CONTRAST_DEFAULT            38

#define lcdRefreshWait()

void lcdInit(void);
void lcdOff(void);
void lcdSetRefVolt(unsigned char val);
void lcdSetContrast(void);
void lcdRefresh();

extern uint8_t currentTrainerMode;
void checkTrainerSettings(void);

#if defined(__cplusplus)
//#include "fifo.h"
#include "dmafifo.h"

#if defined(CROSSFIRE)
#define TELEMETRY_FIFO_SIZE             128
#else
#define TELEMETRY_FIFO_SIZE             64
#endif

// extern Fifo<uint8_t, TELEMETRY_FIFO_SIZE> telemetryFifo;
extern DMAFifo<32> auxSerialRxFifo;
#if defined(AUX4_SERIAL)
extern DMAFifo<AUX4_SERIAL_RXFIFO_SIZE> aux4SerialRxFifo;
#endif
#endif

#endif // _BOARD_H_
