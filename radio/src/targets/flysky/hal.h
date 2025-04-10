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

#ifndef _HAL_H_
#define _HAL_H_
    
#define KEYS_MATRIX_LINES_GPIO          GPIOD
#define KEYS_MATRIX_COLUMNS_GPIO        GPIOC

#define KEYS_MATRIX_R1_PIN              LL_GPIO_PIN_6
#define KEYS_MATRIX_R2_PIN              LL_GPIO_PIN_7
#define KEYS_MATRIX_R3_PIN              LL_GPIO_PIN_8

#define KEYS_MATRIX_L1_PIN              LL_GPIO_PIN_12
#define KEYS_MATRIX_L2_PIN              LL_GPIO_PIN_13
#define KEYS_MATRIX_L3_PIN              LL_GPIO_PIN_14
#define KEYS_MATRIX_L4_PIN              LL_GPIO_PIN_15

#define KEYS_BIND_GPIO                  GPIOF
#define KEYS_BIND_PIN                   LL_GPIO_PIN_2

#define KEYS_RCC_AHB1Periph             (LL_AHB1_GRP1_PERIPH_GPIOC | LL_AHB1_GRP1_PERIPH_GPIOD | LL_AHB1_GRP1_PERIPH_GPIOF)

#define KEYS_COLUMNS_PINS               (KEYS_MATRIX_R1_PIN | KEYS_MATRIX_R2_PIN | KEYS_MATRIX_R3_PIN)
#define KEYS_LINES_PINS                 (KEYS_MATRIX_L1_PIN | KEYS_MATRIX_L2_PIN | KEYS_MATRIX_L3_PIN | KEYS_MATRIX_L4_PIN)

//buggy implementation

#define KEYS_GPIO_PIN_RIGHT 1024
#define KEYS_GPIO_PIN_LEFT 1024
#define KEYS_GPIO_PIN_UP 1024
#define KEYS_GPIO_PIN_DOWN 1024


// LCD driver
#define LCD_RCC_AHB1Periph            (LL_AHB1_GRP1_PERIPH_GPIOB | LL_AHB1_GRP1_PERIPH_GPIOD | LL_AHB1_GRP1_PERIPH_GPIOE)
#define LCD_RCC_APB1Periph            0
#define LCD_RCC_APB2Periph            0

#define LCD_DATA_GPIO                 GPIOE
#define LCD_RW_RST_RS_GPIO            GPIOB
#define LCD_RD_CS_GPIO                GPIOD

#define LCD_DATA_PIN                  (LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7)
#define LCD_RW_PIN                    LL_GPIO_PIN_5
#define LCD_RST_PIN                   LL_GPIO_PIN_4
#define LCD_RS_PIN                    LL_GPIO_PIN_3
#define LCD_RD_PIN                    LL_GPIO_PIN_7
#define LCD_CS_PIN                    LL_GPIO_PIN_2


// CRC
#define CRC_RCC_AHB1Periph            LL_AHB1_GRP1_PERIPH_CRC

// I2C Bus: EEPROM
#define I2C_RCC_APB1Periph            LL_APB1_GRP1_PERIPH_I2C2
#define I2C                           I2C2
#define I2C_GPIO_AF                   LL_GPIO_AF_1
#define I2C_RCC_AHB1Periph            LL_AHB1_GRP1_PERIPH_GPIOB
#define I2C_GPIO                      GPIOB
#define I2C_SCL_GPIO_PIN              LL_GPIO_PIN_10
#define I2C_SDA_GPIO_PIN              LL_GPIO_PIN_11
#define I2C_SCL_GPIO_PinSource        GPIO_PinSource10
#define I2C_SDA_GPIO_PinSource        GPIO_PinSource11
// 0x40B22536; //100kHz 0x10950C27; //400kHz
// 0x00E51842 - CubeMX 48MHz, 400k, 200ns/200ns, analog on
// 0x10D55F7C - CubeMX 48MHz, 100k, 300ns/300ns, analog on
// 0x00401B5A - Erfly6 48MHz, 375k,   ?ns/  ?ns, analog off
#define I2C_TIMING                    0x00401B5A;
#define I2C_ADDRESS_EEPROM            0xA0 // 0x50 << 1 (convert to upper 7 bits)
#define I2C_FLASH_PAGESIZE            64
#define EEPROM_BLOCK_SIZE     (64)

// ADC
#define ADC_MAIN                      ADC1
#define ADC_DMA_Channel               DMA1_Channel1
#define ADC_DMA_Channel_CH            LL_DMA_CHANNEL_1
// #define ADC_SET_DMA_FLAGS()             ADC_DMA->HIFCR = (DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4)
// #define ADC_TRANSFER_COMPLETE()         (ADC_DMA->HISR & DMA_HISR_TCIF4)
#define ADC_DMA_TC_FLAG               DMA1_FLAG_TC1
#define ADC_SAMPTIME                  LL_ADC_SAMPLINGTIME_239CYCLES_5

#define ADC_RCC_AHB1Periph            (LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_GPIOB | LL_AHB1_GRP1_PERIPH_GPIOC | LL_AHB1_GRP1_PERIPH_DMA1)
#define ADC_RCC_APB1Periph            0
#define ADC_RCC_APB2Periph            LL_APB1_GRP2_PERIPH_ADC1
#if !defined(FLYSKY_GIMBAL)
#define ADC_GPIO_PIN_STICK_RV         LL_GPIO_PIN_0  // PA.00
#define ADC_GPIO_PIN_STICK_RH         LL_GPIO_PIN_1  // PA.01
#define ADC_GPIO_PIN_STICK_LV         LL_GPIO_PIN_2  // PA.02
#define ADC_GPIO_PIN_STICK_LH         LL_GPIO_PIN_3  // PA.03
#define ADC_CHANNEL_STICK_RV          ADC_Channel_0  // ADC1_IN0
#define ADC_CHANNEL_STICK_RH          ADC_Channel_1  // ADC1_IN1
#define ADC_CHANNEL_STICK_LV          ADC_Channel_2  // ADC1_IN2
#define ADC_CHANNEL_STICK_LH          ADC_Channel_3  // ADC1_IN3
#endif // FLYSKY_GIMBAL
#define ADC_GPIO_PIN_POT1             LL_GPIO_PIN_6  // PA.06
#define ADC_GPIO_PIN_POT2             LL_GPIO_PIN_0  // PB.00
#define ADC_GPIO_PIN_BATT             LL_GPIO_PIN_0  // PC.00
#if !defined(FLYSKY_GIMBAL)
#define ADC_GPIOA_PINS                (ADC_GPIO_PIN_STICK_RV | ADC_GPIO_PIN_STICK_RH | ADC_GPIO_PIN_STICK_LH | ADC_GPIO_PIN_STICK_LV | ADC_GPIO_PIN_POT1)
#else
#define ADC_GPIOA_PINS                ADC_GPIO_PIN_POT1
#endif // FLYSKY_GIMBAL
#define ADC_GPIOB_PINS                ADC_GPIO_PIN_POT2
#define ADC_GPIOC_PINS                ADC_GPIO_PIN_BATT
#define ADC_CHANNEL_POT1              ADC_Channel_6
#define ADC_CHANNEL_POT2              ADC_Channel_8
#define ADC_CHANNEL_BATT              ADC_Channel_10

// PWR and LED driver
#define PWR_RCC_AHB1Periph            (LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_GPIOE | LL_AHB1_GRP1_PERIPH_GPIOB | LL_AHB1_GRP1_PERIPH_GPIOC | LL_AHB1_GRP1_PERIPH_GPIOD | LL_AHB1_GRP1_PERIPH_GPIOE)

// Internal Module
// #define RF_SCK_GPIO_PORT GPIOE
// #define RF_SCK_PIN_MASK GPIO_IDR_13
// #define RF_SDIO_GPIO_PORT GPIOE
// #define RF_SDIO_PIN_MASK GPIO_IDR_15
#define RF_SCN_GPIO_PORT GPIOE
#define RF_SCN_SET_PIN GPIO_BSRR_BS_12
#define RF_SCN_RESET_PIN GPIO_BSRR_BR_12
// #define RF_GIO2_GPIO_PORT GPIOB
#define RF_GIO2_PIN EXTI_IMR_MR2
#define RF_RxTx_GPIO_PORT GPIOE
#define RF_RxTx_PIN_MASK 0x00000300U
// #define RF_Rx_SET_PIN GPIO_BSRR_BS_8
#define RF_Rx_RESET_PIN GPIO_BSRR_BR_8
// #define RF_Tx_SET_PIN GPIO_PIN_BSRR_BS_9
// #define RF_Tx_RESET_PIN GPIO_PIN_BSRR_BR_9
#define RF_RF0_GPIO_PORT GPIOE
#define RF_RF0_SET_PIN GPIO_BSRR_BS_10
#define RF_RF0_RESET_PIN GPIO_BSRR_BR_10

#define RF_RF1_GPIO_PORT GPIOE
#define RF_RF1_SET_PIN GPIO_BSRR_BS_11
#define RF_RF1_RESET_PIN GPIO_BSRR_BR_11

#define SETBIT(T, B, V) (T = V ? T | (1<<B) : T & ~(1<<B))

void SPI_Write(uint8_t command);
uint8_t SPI_SDI_Read(void);
void a7105_csn_on(void); 
void a7105_csn_off(void);
void RF0_SetVal(void);
void RF0_ClrVal(void);
void RF1_SetVal(void);
void RF1_ClrVal(void);
void TX_RX_PutVal(uint32_t Val);
void EnableGIO(void);
void DisableGIO(void);
void initAFHDS2A();
void ActionAFHDS2A();

#define A7105_CSN_on a7105_csn_on()
#define A7105_CSN_off a7105_csn_off()

#define INTMODULE_RCC_APB2Periph      (LL_APB1_GRP2_PERIPH_TIM16 | LL_APB1_GRP2_PERIPH_SPI1 | LL_APB1_GRP2_PERIPH_SYSCFG)
#define INTMODULE_TIMER               TIM16
#define INTMODULE_TIMER_IRQn          TIM16_IRQn
#define INTMODULE_TIMER_IRQHandler    TIM16_IRQHandler
// #define INTMODULE_TX_GPIO             GPIOA
// #define INTMODULE_TX_GPIO_PIN         LL_GPIO_PIN_10 // PA.10
// #define INTMODULE_TX_GPIO_PinSource   GPIO_PinSource10
// #define INTMODULE_TX_GPIO_AF          GPIO_AF_TIM1
// #define INTMODULE_DMA_CHANNEL         DMA_Channel_6
// #define INTMODULE_DMA_STREAM          DMA2_Stream5
// #define INTMODULE_DMA_STREAM_IRQn     DMA2_Stream5_IRQn
// #define INTMODULE_DMA_STREAM_IRQHandler DMA2_Stream5_IRQHandler
// #define INTMODULE_DMA_FLAG_TC         DMA_IT_TCIF5
// #define INTMODULE_TIMER_FREQ          (PERI1_FREQUENCY * TIMER_MULT_APB1)

// External Module
#define EXTMODULE_PWR_GPIO            GPIOC
#define EXTMODULE_PWR_GPIO_PIN        LL_GPIO_PIN_13  // PC.13
#define EXTMODULE_RCC_AHBPeriph       (LL_AHB1_GRP1_PERIPH_GPIOC | LL_AHB1_GRP1_PERIPH_GPIOF)
#define EXTMODULE_RCC_APB2Periph      LL_APB1_GRP2_PERIPH_TIM15 // TIM15_CH2
#define EXTMODULE_TX_GPIO             GPIOF
#define EXTMODULE_TX_GPIO_PIN         LL_GPIO_PIN_10 // PF.10
#define EXTMODULE_TX_GPIO_PinSource   GPIO_PinSource10
#define EXTMODULE_TX_GPIO_AF          LL_GPIO_AF_0
#define EXTMODULE_TIMER               TIM15
#define EXTMODULE_TIMER_IRQn          TIM15_IRQn
#define EXTMODULE_TIMER_IRQHandler    TIM15_IRQHandler
#define EXTMODULE_TIMER_FREQ          (PERI2_FREQUENCY * TIMER_MULT_APB2)

// Trainer Port
#define TRAINER_GPIO                  GPIOF
#define TRAINER_IN_GPIO_PIN           LL_GPIO_PIN_9  // PF.09
#define TRAINER_IN_GPIO_PinSource     GPIO_PinSource9
#define TRAINER_GPIO_AF               LL_GPIO_AF_0
// #define TRAINER_TIMER                 EXTMODULE_TIMER
// #define TRAINER_TIMER_IRQn            EXTMODULE_TIMER_IRQn
// #define TRAINER_TIMER_IRQHandler      EXTMODULE_TIMER_IRQHandler
// #define TRAINER_DMA                   DMA1
// #define TRAINER_DMA_CHANNEL           DMA1_Channel5
// #define TRAINER_DMA_IRQn              DMA1_Channel4_5_IRQn
// #define TRAINER_DMA_IRQHandler        DMA1_Channel4_5_IRQHandler
// #define TRAINER_DMA_FLAG_TC           DMA_IT_TCIF5
// #define TRAINER_TIMER_FREQ            (PERI1_FREQUENCY * TIMER_MULT_APB1)

// USB
#define USB_RCC_APB1Periph_CRS          LL_APB1_GRP1_PERIPH_CRS
#define USB_RCC_AHBPeriph_GPIO          LL_AHB1_GRP1_PERIPH_GPIOA
#define USB_GPIO                        GPIOA
#define USB_GPIO_PIN_DM                 LL_GPIO_PIN_11 // PA.11
#define USB_GPIO_PIN_DP                 LL_GPIO_PIN_12 // PA.12

// Flash (taken from f2)
#define FLASH_CR_SER               ((uint32_t)0x00000002)
#define FLASH_PSIZE_BYTE           ((uint32_t)0x00000000)
#define FLASH_PSIZE_HALF_WORD      ((uint32_t)0x00000100)
#define FLASH_PSIZE_WORD           ((uint32_t)0x00000200)
#define FLASH_PSIZE_DOUBLE_WORD    ((uint32_t)0x00000300)
#define CR_PSIZE_MASK              ((uint32_t)0xFFFFFCFF)

// Serial Port
#if defined(SBUS_TRAINER)
#define TRAINER_BATTERY_COMPARTMENT
#endif

// AUX Serial
#define AUX_SERIAL_RCC_AHB1Periph         (LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_DMA1)
#define AUX_SERIAL_RCC_APB2Periph         LL_APB1_GRP2_PERIPH_USART1
#define AUX_SERIAL_GPIO                   GPIOA
#define AUX_SERIAL_GPIO_PIN_TX            LL_GPIO_PIN_9 // PA9
#define AUX_SERIAL_GPIO_PIN_RX            LL_GPIO_PIN_10 // PA10
#define AUX_SERIAL_GPIO_PinSource_TX      GPIO_PinSource9
#define AUX_SERIAL_GPIO_PinSource_RX      GPIO_PinSource10
#define AUX_SERIAL_GPIO_AF                LL_GPIO_AF_1
#define AUX_SERIAL_USART                  USART1
#define AUX_SERIAL_USART_IRQHandler       USART1_IRQHandler
#define AUX_SERIAL_USART_IRQn             USART1_IRQn
#define AUX_SERIAL_DMA_Channel_RX         DMA1_Channel3
#define AUX_SERIAL_DMA_Channel_RX_CH      LL_DMA_CHANNEL_3

// AUX3 Serial, only TX for DFPLAYER
#define AUX3_SERIAL_RCC_AHB1Periph         LL_AHB1_GRP1_PERIPH_GPIOC
#define AUX3_SERIAL_RCC_APB1Periph         LL_APB1_GRP1_PERIPH_USART3
#define AUX3_SERIAL_GPIO                   GPIOC
#define AUX3_SERIAL_GPIO_PIN_TX            LL_GPIO_PIN_10 // PC10
#define AUX3_SERIAL_GPIO_PinSource_TX      GPIO_PinSource10
#define AUX3_SERIAL_GPIO_AF                LL_GPIO_AF_1
#define AUX3_SERIAL_USART                  USART3

// AUX4 Serial, only RX for FLYSKY_GIMBAL
#define AUX4_SERIAL_RCC_AHB1Periph         (LL_AHB1_GRP1_PERIPH_GPIOC | LL_AHB1_GRP1_PERIPH_DMA1)
#define AUX4_SERIAL_RCC_APB1Periph         LL_APB1_GRP1_PERIPH_USART3
#define AUX4_SERIAL_GPIO                   GPIOC
#define AUX4_SERIAL_GPIO_PIN_RX            LL_GPIO_PIN_11 // PC11
#define AUX4_SERIAL_GPIO_PinSource_RX      GPIO_PinSource11
#define AUX4_SERIAL_GPIO_AF                LL_GPIO_AF_0
#define AUX4_SERIAL_USART                  USART4
#define AUX4_SERIAL_DMA_Channel_RX         DMA1_Channel6

#define AUX34_SERIAL_USART_IRQHandler       USART3_4_IRQHandler
#define AUX34_SERIAL_USART_IRQn             USART3_4_IRQn

#define SPORT_MAX_BAUDRATE            400000

// Telemetry
#define TELEMETRY_RCC_AHB1Periph        (LL_AHB1_GRP1_PERIPH_GPIOD | LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_DMA1)
#define TELEMETRY_RCC_APB1Periph        LL_APB1_GRP1_PERIPH_USART2
#define TELEMETRY_GPIO                  GPIOD
#define TELEMETRY_TX_GPIO_PIN           LL_GPIO_PIN_5  // PD.05
#define TELEMETRY_RX_GPIO               GPIOA
#define TELEMETRY_RX_GPIO_PIN           LL_GPIO_PIN_15 // PA.15
#define TELEMETRY_GPIO_PinSource_TX     GPIO_PinSource5
#define TELEMETRY_GPIO_PinSource_RX     GPIO_PinSource15
#define TELEMETRY_TX_GPIO_AF            LL_GPIO_AF_0
#define TELEMETRY_RX_GPIO_AF            LL_GPIO_AF_1
#define TELEMETRY_USART                 USART2
#define TELEMETRY_DMA_Channel_TX        DMA1_Channel4
#define TELEMETRY_DMA_Channel_TX_CH     LL_DMA_CHANNEL_4
#define TELEMETRY_DMA_TX_IRQn           DMA1_Channel4_5_IRQn
#define TELEMETRY_DMA_TX_IRQHandler     DMA1_Channel4_5_IRQHandler
#define TELEMETRY_DMA_TX_FLAG_TC        DMA1_IT_TC4
#define TELEMETRY_DMA_Channel_RX        DMA1_Channel5
#define TELEMETRY_DMA_Channel_RX_CH     LL_DMA_CHANNEL_5
#define TELEMETRY_USART_IRQHandler      USART2_IRQHandler
#define TELEMETRY_USART_IRQn            USART2_IRQn


// Backlight
// pwm, requires wiring BL pad to PC9 pad
  #define BACKLIGHT_RCC_APB1Periph      LL_APB1_GRP1_PERIPH_TIM3
  #define BACKLIGHT_RCC_AHB1Periph      LL_AHB1_GRP1_PERIPH_GPIOC
  #define BACKLIGHT_GPIO                GPIOC
  #define BACKLIGHT_GPIO_PIN            LL_GPIO_PIN_9
  #define BACKLIGHT_TIMER_FREQ          (PERI1_FREQUENCY * TIMER_MULT_APB1)
  #define BACKLIGHT_TIMER               TIM3
  #define BACKLIGHT_GPIO_PinSource      GPIO_PinSource9
  #define BACKLIGHT_GPIO_AF             LL_GPIO_AF_0
  #define BACKLIGHT_CCMR2               TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2 // Channel4, PWM
  #define BACKLIGHT_CCER                TIM_CCER_CC4P | TIM_CCER_CC4E
  #define BACKLIGHT_COUNTER_REGISTER    BACKLIGHT_TIMER->CCR4
  // std, fixed brightness
  #define BACKLIGHT_STD_RCC_APB1Periph      0
  #define BACKLIGHT_STD_RCC_AHB1Periph      RCC_AHBPeriph_GPIOF
  #define BACKLIGHT_STD_GPIO                GPIOF
  #define BACKLIGHT_STD_GPIO_PIN            LL_GPIO_PIN_3

// Audio
//...

// Buzzer
#define BUZZER_GPIO_PORT                GPIOA
#define BUZZER_GPIO_PIN                 LL_GPIO_PIN_8
#define BUZZER_GPIO_PinSource           GPIO_PinSource8
#define BUZZER_RCC_AHBPeriph            LL_AHB1_GRP1_PERIPH_GPIOA
#define PWM_RCC_APB2Periph              LL_APB1_GRP2_PERIPH_TIM1
#define PWM_TIMER                       TIM1

// DFPlayer
#if defined(DFPLAYER)
#define DFPLAYER_GPIO_PORT              GPIOC
#define DFPLAYER_GPIO_PIN_BUSY          LL_GPIO_PIN_14
#define DFPLAYER_BAUDRATE               9600
#endif

// Xms Interrupt
#define INTERRUPT_xMS_RCC_APB1Periph    LL_APB1_GRP1_PERIPH_TIM14
#define INTERRUPT_xMS_TIMER             TIM14
#define INTERRUPT_xMS_IRQn              TIM14_IRQn
#define INTERRUPT_xMS_IRQHandler        TIM14_IRQHandler

// 2MHz Timer
#define TIMER_2MHz_RCC_APB1Periph       LL_APB1_GRP1_PERIPH_TIM7
#define TIMER_2MHz_TIMER                TIM7

// Mixer scheduler timer
#define MIXER_SCHEDULER_TIMER_RCC_APB2Periph LL_APB1_GRP2_PERIPH_TIM17
#define MIXER_SCHEDULER_TIMER                TIM17
#define MIXER_SCHEDULER_TIMER_FREQ           (PERI1_FREQUENCY * TIMER_MULT_APB1)
#define MIXER_SCHEDULER_TIMER_IRQn           TIM17_IRQn
#define MIXER_SCHEDULER_TIMER_IRQHandler     TIM17_IRQHandler

//all used RCC goes here
#define RCC_AHB1_GRP1_LIST                   (I2C_RCC_AHB1Periph | BACKLIGHT_STD_RCC_APB1Periph | BACKLIGHT_RCC_AHB1Periph | LCD_RCC_AHB1Periph | KEYS_RCC_AHB1Periph | BUZZER_RCC_AHBPeriph \
                                         | EXTMODULE_RCC_AHBPeriph | CRC_RCC_AHB1Periph | TELEMETRY_RCC_AHB1Periph | AUX_SERIAL_RCC_AHB1Periph \
                                         | AUX3_SERIAL_RCC_AHB1Periph | AUX4_SERIAL_RCC_AHB1Periph | ADC_RCC_AHB1Periph | USB_RCC_AHBPeriph_GPIO)
#define RCC_APB1_GRP1_LIST                   (I2C_RCC_APB1Periph | LL_APB1_GRP1_PERIPH_TIM6 /*delays*/ | INTERRUPT_xMS_RCC_APB1Periph | TIMER_2MHz_RCC_APB1Periph \
                                         | TELEMETRY_RCC_APB1Periph | BACKLIGHT_STD_RCC_APB1Periph | BACKLIGHT_RCC_APB1Periph | LL_APB1_GRP1_PERIPH_USB \
                                         | AUX3_SERIAL_RCC_APB1Periph | AUX4_SERIAL_RCC_APB1Periph | USB_RCC_APB1Periph_CRS)
#define RCC_APB1_GRP2_LIST                   (MIXER_SCHEDULER_TIMER_RCC_APB2Periph | PWM_RCC_APB2Periph | INTMODULE_RCC_APB2Periph | EXTMODULE_RCC_APB2Periph \
                                         | AUX_SERIAL_RCC_APB2Periph | ADC_RCC_APB2Periph)

#endif // _HAL_H_
