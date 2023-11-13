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

#define KEYS_MATRIX_R1_PIN              GPIO_Pin_6
#define KEYS_MATRIX_R2_PIN              GPIO_Pin_7
#define KEYS_MATRIX_R3_PIN              GPIO_Pin_8

#define KEYS_MATRIX_L1_PIN              GPIO_Pin_12
#define KEYS_MATRIX_L2_PIN              GPIO_Pin_13
#define KEYS_MATRIX_L3_PIN              GPIO_Pin_14
#define KEYS_MATRIX_L4_PIN              GPIO_Pin_15

#define KEYS_BIND_GPIO                  GPIOF
#define KEYS_BIND_PIN                   GPIO_Pin_2

#define KEYS_RCC_AHB1Periph             (RCC_AHBPeriph_GPIOC | RCC_AHBPeriph_GPIOD | RCC_AHBPeriph_GPIOF)

#define KEYS_COLUMNS_PINS               (KEYS_MATRIX_R1_PIN | KEYS_MATRIX_R2_PIN | KEYS_MATRIX_R3_PIN)
#define KEYS_LINES_PINS                 (KEYS_MATRIX_L1_PIN | KEYS_MATRIX_L2_PIN | KEYS_MATRIX_L3_PIN | KEYS_MATRIX_L4_PIN)

//buggy implementation

#define KEYS_GPIO_PIN_RIGHT 1024
#define KEYS_GPIO_PIN_LEFT 1024
#define KEYS_GPIO_PIN_UP 1024
#define KEYS_GPIO_PIN_DOWN 1024


// LCD driver
#define LCD_RCC_AHB1Periph            (RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOD | RCC_AHBPeriph_GPIOE)
#define LCD_RCC_APB1Periph            0
#define LCD_RCC_APB2Periph            0

#define LCD_DATA_GPIO                 GPIOE
#define LCD_RW_RST_RS_GPIO            GPIOB
#define LCD_RD_CS_GPIO                GPIOD

#define LCD_DATA_PIN                  (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7)
#define LCD_RW_PIN                    GPIO_Pin_5
#define LCD_RST_PIN                   GPIO_Pin_4
#define LCD_RS_PIN                    GPIO_Pin_3
#define LCD_RD_PIN                    GPIO_Pin_7
#define LCD_CS_PIN                    GPIO_Pin_2


// CRC
#define CRC_RCC_AHB1Periph             RCC_AHBPeriph_CRC

// I2C Bus: EEPROM
#define I2C_RCC_APB1Periph            RCC_APB1Periph_I2C2
#define I2C                           I2C2
#define I2C_GPIO_AF                   GPIO_AF_1
#define I2C_RCC_AHB1Periph            RCC_AHBPeriph_GPIOB
#define I2C_GPIO                      GPIOB
#define I2C_SCL_GPIO_PIN              GPIO_Pin_10
#define I2C_SDA_GPIO_PIN              GPIO_Pin_11
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
//#define EEPROM_VERIFY_WRITES

// ADC
#define ADC_MAIN                        ADC1
#define ADC_DMA_Channel                 DMA1_Channel1
// #define ADC_SET_DMA_FLAGS()             ADC_DMA->HIFCR = (DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4)
// #define ADC_TRANSFER_COMPLETE()         (ADC_DMA->HISR & DMA_HISR_TCIF4)
#define ADC_DMA_TC_FLAG               DMA1_FLAG_TC1
#define ADC_SAMPTIME                    ADC_SampleTime_41_5Cycles

#define ADC_RCC_AHB1Periph            (RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC | RCC_AHBPeriph_DMA1)
#define ADC_RCC_APB1Periph            0
#define ADC_RCC_APB2Periph            RCC_APB2Periph_ADC1
#if !defined(FLYSKY_GIMBAL)
#define ADC_GPIO_PIN_STICK_RV         GPIO_Pin_0  // PA.00
#define ADC_GPIO_PIN_STICK_RH         GPIO_Pin_1  // PA.01
#define ADC_GPIO_PIN_STICK_LV         GPIO_Pin_2  // PA.02
#define ADC_GPIO_PIN_STICK_LH         GPIO_Pin_3  // PA.03
#define ADC_CHANNEL_STICK_RV          ADC_Channel_0  // ADC1_IN0
#define ADC_CHANNEL_STICK_RH          ADC_Channel_1  // ADC1_IN1
#define ADC_CHANNEL_STICK_LV          ADC_Channel_2  // ADC1_IN2
#define ADC_CHANNEL_STICK_LH          ADC_Channel_3  // ADC1_IN3
#endif // FLYSKY_GIMBAL
#define ADC_GPIO_PIN_POT1             GPIO_Pin_6  // PA.06
#define ADC_GPIO_PIN_POT2             GPIO_Pin_0  // PB.00
#define ADC_GPIO_PIN_BATT             GPIO_Pin_0  // PC.00
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
#define PWR_RCC_AHB1Periph              (RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOE | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC | RCC_AHBPeriph_GPIOD | RCC_AHBPeriph_GPIOE)
/*
#if defined(PCBX9E) || defined(PCBX7) || defined(PCBXLITE)
#define PWR_PRESS_BUTTON
#endif

  #define PWR_SWITCH_GPIO               GPIOD
  #define PWR_SWITCH_GPIO_PIN           GPIO_Pin_1  // PD.01
  #define PWR_ON_GPIO                   GPIOD
  #define PWR_ON_GPIO_PIN               GPIO_Pin_0  // PD.00

#define PWR_ON_GPIO_MODER               GPIO_MODER_MODER0
#define PWR_ON_GPIO_MODER_OUT           GPIO_MODER_MODER0_0
*/

// Internal Module
/*--------------interrupt handlers-------------------------------------------*/ 

// PPM
void TIM15_IRQHandler(void);

// Internal RF
void EXTI2_3_IRQHandler(void);
void TIM16_IRQHandler(void);

#define READBIT(A, B) ((A >> (B & 7)) & 1)
#define SETBIT(T, B, V) (T = V ? T | (1<<B) : T & ~(1<<B))
#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)    ((REG) & (BIT))
#define CLEAR_REG(REG)        ((REG) = (0x0))
#define WRITE_REG(REG, VAL)   ((REG) = (VAL))
#define READ_REG(REG)         ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

#define RF_SCK_GPIO_PORT GPIOE
#define RF_SCK_PIN_MASK GPIO_IDR_13

#define RF_SDIO_GPIO_PORT GPIOE
#define RF_SDIO_PIN_MASK GPIO_IDR_15

#define RF_SCN_GPIO_PORT GPIOE
#define RF_SCN_SET_PIN GPIO_BSRR_BS_12
#define RF_SCN_RESET_PIN GPIO_BSRR_BR_12

#define RF_GIO2_GPIO_PORT GPIOB
#define RF_GIO2_PIN EXTI_IMR_IM2

#define RF_RxTx_GPIO_PORT GPIOE
#define RF_RxTx_PIN_MASK 0x00000300U

#define RF_Rx_SET_PIN GPIO_BSRR_BS_8
#define RF_Rx_RESET_PIN GPIO_BSRR_BR_8

#define RF_Tx_SET_PIN GPIO_PIN_BSRR_BS_9
#define RF_Tx_RESET_PIN GPIO_PIN_BSRR_BR_9

#define RF_RF0_GPIO_PORT GPIOE
#define RF_RF0_SET_PIN GPIO_BSRR_BS_10
#define RF_RF0_RESET_PIN GPIO_BSRR_BR_10

#define RF_RF1_GPIO_PORT GPIOE
#define RF_RF1_SET_PIN GPIO_BSRR_BS_11
#define RF_RF1_RESET_PIN GPIO_BSRR_BR_11

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
void init_afhds2a(uint32_t port);
void disable_afhds2a(uint32_t port);
#define A7105_CSN_on a7105_csn_on()
#define A7105_CSN_off a7105_csn_off()

/******************************************************************************/
/*                                                                            */
/*                 External Interrupt/Event Controller (EXTI)                 */
/*                                                                            */
/******************************************************************************/

/* References Defines */
#define  EXTI_IMR_IM0 EXTI_IMR_MR0
#define  EXTI_IMR_IM1 EXTI_IMR_MR1
#define  EXTI_IMR_IM2 EXTI_IMR_MR2
#define  EXTI_IMR_IM3 EXTI_IMR_MR3
#define  EXTI_IMR_IM4 EXTI_IMR_MR4
#define  EXTI_IMR_IM5 EXTI_IMR_MR5
#define  EXTI_IMR_IM6 EXTI_IMR_MR6
#define  EXTI_IMR_IM7 EXTI_IMR_MR7
#define  EXTI_IMR_IM8 EXTI_IMR_MR8
#define  EXTI_IMR_IM9 EXTI_IMR_MR9
#define  EXTI_IMR_IM10 EXTI_IMR_MR10
#define  EXTI_IMR_IM11 EXTI_IMR_MR11
#define  EXTI_IMR_IM12 EXTI_IMR_MR12
#define  EXTI_IMR_IM13 EXTI_IMR_MR13
#define  EXTI_IMR_IM14 EXTI_IMR_MR14
#define  EXTI_IMR_IM15 EXTI_IMR_MR15
#define  EXTI_IMR_IM16 EXTI_IMR_MR16
#define  EXTI_IMR_IM17 EXTI_IMR_MR17
#define  EXTI_IMR_IM18 EXTI_IMR_MR18
#define  EXTI_IMR_IM19 EXTI_IMR_MR19
#define  EXTI_IMR_IM20 EXTI_IMR_MR20
#define  EXTI_IMR_IM21 EXTI_IMR_MR21
#define  EXTI_IMR_IM22 EXTI_IMR_MR22
#define  EXTI_IMR_IM23 EXTI_IMR_MR23
#define  EXTI_IMR_IM25 EXTI_IMR_MR25
#define  EXTI_IMR_IM26 EXTI_IMR_MR26
#define  EXTI_IMR_IM27 EXTI_IMR_MR27
#define  EXTI_IMR_IM31 EXTI_IMR_MR31

/* References Defines */
#define  EXTI_EMR_EM0 EXTI_EMR_MR0
#define  EXTI_EMR_EM1 EXTI_EMR_MR1
#define  EXTI_EMR_EM2 EXTI_EMR_MR2
#define  EXTI_EMR_EM3 EXTI_EMR_MR3
#define  EXTI_EMR_EM4 EXTI_EMR_MR4
#define  EXTI_EMR_EM5 EXTI_EMR_MR5
#define  EXTI_EMR_EM6 EXTI_EMR_MR6
#define  EXTI_EMR_EM7 EXTI_EMR_MR7
#define  EXTI_EMR_EM8 EXTI_EMR_MR8
#define  EXTI_EMR_EM9 EXTI_EMR_MR9
#define  EXTI_EMR_EM10 EXTI_EMR_MR10
#define  EXTI_EMR_EM11 EXTI_EMR_MR11
#define  EXTI_EMR_EM12 EXTI_EMR_MR12
#define  EXTI_EMR_EM13 EXTI_EMR_MR13
#define  EXTI_EMR_EM14 EXTI_EMR_MR14
#define  EXTI_EMR_EM15 EXTI_EMR_MR15
#define  EXTI_EMR_EM16 EXTI_EMR_MR16
#define  EXTI_EMR_EM17 EXTI_EMR_MR17
#define  EXTI_EMR_EM18 EXTI_EMR_MR18
#define  EXTI_EMR_EM19 EXTI_EMR_MR19
#define  EXTI_EMR_EM20 EXTI_EMR_MR0
#define  EXTI_EMR_EM21 EXTI_EMR_MR21
#define  EXTI_EMR_EM22 EXTI_EMR_MR22
#define  EXTI_EMR_EM23 EXTI_EMR_MR23
#define  EXTI_EMR_EM25 EXTI_EMR_MR25
#define  EXTI_EMR_EM26 EXTI_EMR_MR26
#define  EXTI_EMR_EM27 EXTI_EMR_MR27
#define  EXTI_EMR_EM31 EXTI_EMR_MR31

/* References Defines */
#define EXTI_RTSR_RT0 EXTI_RTSR_TR0
#define EXTI_RTSR_RT1 EXTI_RTSR_TR1
#define EXTI_RTSR_RT2 EXTI_RTSR_TR2
#define EXTI_RTSR_RT3 EXTI_RTSR_TR3
#define EXTI_RTSR_RT4 EXTI_RTSR_TR4
#define EXTI_RTSR_RT5 EXTI_RTSR_TR5
#define EXTI_RTSR_RT6 EXTI_RTSR_TR6
#define EXTI_RTSR_RT7 EXTI_RTSR_TR7
#define EXTI_RTSR_RT8 EXTI_RTSR_TR8
#define EXTI_RTSR_RT9 EXTI_RTSR_TR9
#define EXTI_RTSR_RT10 EXTI_RTSR_TR10
#define EXTI_RTSR_RT11 EXTI_RTSR_TR11
#define EXTI_RTSR_RT12 EXTI_RTSR_TR12
#define EXTI_RTSR_RT13 EXTI_RTSR_TR13
#define EXTI_RTSR_RT14 EXTI_RTSR_TR14
#define EXTI_RTSR_RT15 EXTI_RTSR_TR15
#define EXTI_RTSR_RT16 EXTI_RTSR_TR16
#define EXTI_RTSR_RT17 EXTI_RTSR_TR17
#define EXTI_RTSR_RT19 EXTI_RTSR_TR19
#define EXTI_RTSR_RT20 EXTI_RTSR_TR20
#define EXTI_RTSR_RT21 EXTI_RTSR_TR21
#define EXTI_RTSR_RT22 EXTI_RTSR_TR22

/* References Defines */
#define EXTI_SWIER_SWI0 EXTI_SWIER_SWIER0
#define EXTI_SWIER_SWI1 EXTI_SWIER_SWIER1
#define EXTI_SWIER_SWI2 EXTI_SWIER_SWIER2
#define EXTI_SWIER_SWI3 EXTI_SWIER_SWIER3
#define EXTI_SWIER_SWI4 EXTI_SWIER_SWIER4
#define EXTI_SWIER_SWI5 EXTI_SWIER_SWIER5
#define EXTI_SWIER_SWI6 EXTI_SWIER_SWIER6
#define EXTI_SWIER_SWI7 EXTI_SWIER_SWIER7
#define EXTI_SWIER_SWI8 EXTI_SWIER_SWIER8
#define EXTI_SWIER_SWI9 EXTI_SWIER_SWIER9
#define EXTI_SWIER_SWI10 EXTI_SWIER_SWIER10
#define EXTI_SWIER_SWI11 EXTI_SWIER_SWIER11
#define EXTI_SWIER_SWI12 EXTI_SWIER_SWIER12
#define EXTI_SWIER_SWI13 EXTI_SWIER_SWIER13
#define EXTI_SWIER_SWI14 EXTI_SWIER_SWIER14
#define EXTI_SWIER_SWI15 EXTI_SWIER_SWIER15
#define EXTI_SWIER_SWI16 EXTI_SWIER_SWIER16
#define EXTI_SWIER_SWI17 EXTI_SWIER_SWIER17
#define EXTI_SWIER_SWI19 EXTI_SWIER_SWIER19
#define EXTI_SWIER_SWI20 EXTI_SWIER_SWIER20
#define EXTI_SWIER_SWI21 EXTI_SWIER_SWIER21
#define EXTI_SWIER_SWI22 EXTI_SWIER_SWIER22

/* References Defines */
#define EXTI_PR_PIF0 EXTI_PR_PR0
#define EXTI_PR_PIF1 EXTI_PR_PR1
#define EXTI_PR_PIF2 EXTI_PR_PR2
#define EXTI_PR_PIF3 EXTI_PR_PR3
#define EXTI_PR_PIF4 EXTI_PR_PR4
#define EXTI_PR_PIF5 EXTI_PR_PR5
#define EXTI_PR_PIF6 EXTI_PR_PR6
#define EXTI_PR_PIF7 EXTI_PR_PR7
#define EXTI_PR_PIF8 EXTI_PR_PR8
#define EXTI_PR_PIF9 EXTI_PR_PR9
#define EXTI_PR_PIF10 EXTI_PR_PR10
#define EXTI_PR_PIF11 EXTI_PR_PR11
#define EXTI_PR_PIF12 EXTI_PR_PR12
#define EXTI_PR_PIF13 EXTI_PR_PR13
#define EXTI_PR_PIF14 EXTI_PR_PR14
#define EXTI_PR_PIF15 EXTI_PR_PR15
#define EXTI_PR_PIF16 EXTI_PR_PR16
#define EXTI_PR_PIF17 EXTI_PR_PR17
#define EXTI_PR_PIF19 EXTI_PR_PR19
#define EXTI_PR_PIF20 EXTI_PR_PR20
#define EXTI_PR_PIF21 EXTI_PR_PR21
#define EXTI_PR_PIF22 EXTI_PR_PR22

#define UID_BASE              ((uint32_t)0x1FFFF7ACU)       /*!< Unique device ID register base address */

 /* #define INTMODULE_PULSES
 PXX and DSM!!!!!

  #define INTMODULE_RCC_AHB1Periph      (RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOC | RCC_AHBPeriph_DMA2)
  #define INTMODULE_RCC_APB1Periph      0
  #define INTMODULE_RCC_APB2Periph      RCC_APB2Periph_TIM1
  #define INTMODULE_PWR_GPIO            GPIOC
  #define INTMODULE_PWR_GPIO_PIN        GPIO_Pin_6  // PC.06
  #define INTMODULE_TX_GPIO             GPIOA
  #define INTMODULE_TX_GPIO_PIN         GPIO_Pin_10 // PA.10
  #define INTMODULE_TX_GPIO_PinSource   GPIO_PinSource10
  #define INTMODULE_TIMER               TIM1
  #define INTMODULE_TIMER_CC_IRQn       TIM1_CC_IRQn
  #define INTMODULE_TIMER_CC_IRQHandler TIM1_CC_IRQHandler
  #define INTMODULE_TX_GPIO_AF          GPIO_AF_TIM1
  #define INTMODULE_DMA_CHANNEL         DMA_Channel_6
  #define INTMODULE_DMA_STREAM          DMA2_Stream5
  #define INTMODULE_DMA_STREAM_IRQn     DMA2_Stream5_IRQn
  #define INTMODULE_DMA_STREAM_IRQHandler DMA2_Stream5_IRQHandler
  #define INTMODULE_DMA_FLAG_TC         DMA_IT_TCIF5
  #define INTMODULE_TIMER_FREQ          (PERI2_FREQUENCY * TIMER_MULT_APB2)
*/
#define INTMODULE_RCC_APB2Periph      (RCC_APB2Periph_TIM16 | RCC_APB2Periph_SPI1)

// External Module
#define EXTMODULE_PWR_GPIO            GPIOC
#define EXTMODULE_PWR_GPIO_PIN        GPIO_Pin_13  // PC.13
#define EXTMODULE_RCC_AHBPeriph       (RCC_AHBPeriph_GPIOC | RCC_AHBPeriph_GPIOF)
#define EXTMODULE_RCC_APB2Periph      RCC_APB2Periph_TIM15 // TIM15_CH2
#define EXTMODULE_TX_GPIO             GPIOF
#define EXTMODULE_TX_GPIO_PIN         GPIO_Pin_10 // PF.10
#define EXTMODULE_TX_GPIO_PinSource   GPIO_PinSource10
#define EXTMODULE_TX_GPIO_AF          GPIO_AF_0
#define EXTMODULE_TIMER               TIM15
#define EXTMODULE_TIMER_IRQn          TIM15_IRQn
#define EXTMODULE_TIMER_IRQHandler    TIM15_IRQHandler
#define EXTMODULE_TIMER_FREQ          (PERI2_FREQUENCY * TIMER_MULT_APB2)

extern void ISR_TIMER0_COMP_vect(void);
extern void ISR_TIMER2_OVF_vect(void);
extern void ISR_TIMER1_COMPA_vect(void);
extern void ISR_TIMER3_CAPT_vect(void);

// Trainer Port
#define TRAINER_GPIO                  GPIOF
#define TRAINER_IN_GPIO_PIN           GPIO_Pin_9  // PC.08
#define TRAINER_IN_GPIO_PinSource     GPIO_PinSource9
#define TRAINER_GPIO_AF               GPIO_AF_0
// #define TRAINER_OUT_GPIO_PIN          GPIO_Pin_9  // PC.09
// #define TRAINER_OUT_GPIO_PinSource    GPIO_PinSource9
// #define TRAINER_DETECT_GPIO           GPIOA
// #define TRAINER_DETECT_GPIO_PIN       GPIO_Pin_8  // PA.08
// #define TRAINER_TIMER                 EXTMODULE_TIMER
// #define TRAINER_TIMER_IRQn            EXTMODULE_TIMER_IRQn
// #define TRAINER_DMA                   DMA1
// #define TRAINER_DMA_CHANNEL           DMA_Channel_5
// #define TRAINER_DMA_STREAM            DMA1_Stream2
// #define TRAINER_DMA_IRQn              DMA1_Stream2_IRQn
// #define TRAINER_DMA_IRQHandler        DMA1_Stream2_IRQHandler
// #define TRAINER_DMA_FLAG_TC           DMA_IT_TCIF2
// #define TRAINER_TIMER_IRQn            TIM3_IRQn
// #define TRAINER_TIMER_IRQHandler      TIM3_IRQHandler
// #define TRAINER_TIMER_FREQ            (PERI1_FREQUENCY * TIMER_MULT_APB1)


#define SD_RCC_AHB1Periph 0
#define HAPTIC_RCC_AHB1Periph 0
#define EXTMODULE_RCC_AHB1Periph 0
#define SPORT_UPDATE_RCC_AHB1Periph 0
#define BT_RCC_AHB1Periph 0
#define TRAINER_RCC_AHB1Periph 0
#define HAPTIC_RCC_APB1Periph 0
#define SD_RCC_APB1Periph 0
#define BT_RCC_APB1Periph 0
#define HAPTIC_RCC_APB2Periph 0
#define BT_RCC_APB2Periph 0
#define SD_GPIO_PRESENT_GPIO 0

// USB
#define USB_RCC_AHBPeriph_GPIO          RCC_AHBPeriph_GPIOA
#define USB_GPIO                        GPIOA
#define USB_GPIO_PIN_VBUS               GPIO_Pin_15  // PA.15

// Flash (taken from f2)
#define FLASH_CR_SER               ((uint32_t)0x00000002)
#define FLASH_PSIZE_BYTE           ((uint32_t)0x00000000)
#define FLASH_PSIZE_HALF_WORD      ((uint32_t)0x00000100)
#define FLASH_PSIZE_WORD           ((uint32_t)0x00000200)
#define FLASH_PSIZE_DOUBLE_WORD    ((uint32_t)0x00000300)
#define CR_PSIZE_MASK              ((uint32_t)0xFFFFFCFF)

// Serial Port

#define TRAINER_BATTERY_COMPARTMENT

// AUX Serial
#define AUX_SERIAL_RCC_AHB1Periph         (RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_DMA1)
#define AUX_SERIAL_RCC_APB2Periph         RCC_APB2Periph_USART1
#define AUX_SERIAL_GPIO                   GPIOA
#define AUX_SERIAL_GPIO_PIN_TX            GPIO_Pin_9 // PA9
#define AUX_SERIAL_GPIO_PIN_RX            GPIO_Pin_10 // PA10
#define AUX_SERIAL_GPIO_PinSource_TX      GPIO_PinSource9
#define AUX_SERIAL_GPIO_PinSource_RX      GPIO_PinSource10
#define AUX_SERIAL_GPIO_AF                GPIO_AF_1
#define AUX_SERIAL_USART                  USART1
#define AUX_SERIAL_USART_IRQHandler       USART1_IRQHandler
#define AUX_SERIAL_USART_IRQn             USART1_IRQn
#define AUX_SERIAL_DMA_Channel_RX         DMA1_Channel3

// AUX2 Serial
#define AUX2_SERIAL_RCC_AHB1Periph         (RCC_AHBPeriph_GPIOC | RCC_AHBPeriph_DMA1)
#define AUX2_SERIAL_RCC_APB1Periph         RCC_APB1Periph_USART4
#define AUX2_SERIAL_GPIO                   GPIOC
#define AUX2_SERIAL_GPIO_PIN_TX            GPIO_Pin_10 // PC10
#define AUX2_SERIAL_GPIO_PIN_RX            GPIO_Pin_11 // PC11
#define AUX2_SERIAL_GPIO_PinSource_TX      GPIO_PinSource10
#define AUX2_SERIAL_GPIO_PinSource_RX      GPIO_PinSource11
#define AUX2_SERIAL_GPIO_AF                GPIO_AF_0
#define AUX2_SERIAL_USART                  USART4
#define AUX2_SERIAL_USART_IRQHandler       USART3_4_IRQHandler
#define AUX2_SERIAL_USART_IRQn             USART3_4_IRQn
#define AUX2_SERIAL_DMA_Channel_RX         DMA1_Channel6


#define SPORT_MAX_BAUDRATE            400000

// Telemetry
#define TELEMETRY_RCC_AHB1Periph        (RCC_AHBPeriph_GPIOD | RCC_AHBPeriph_DMA1)
#define TELEMETRY_RCC_APB1Periph        RCC_APB1Periph_USART2
#define TELEMETRY_GPIO                  GPIOD
#define TELEMETRY_TX_GPIO_PIN           GPIO_Pin_5  // PD.05
#define TELEMETRY_RX_GPIO_PIN           
#define TELEMETRY_GPIO_PinSource_TX     GPIO_PinSource5
#define TELEMETRY_GPIO_PinSource_RX     
#define TELEMETRY_GPIO_AF               GPIO_AF_0
#define TELEMETRY_USART                 USART2
#define TELEMETRY_DMA_Channel_TX        DMA1_Channel4
#define TELEMETRY_DMA_TX_IRQn           DMA1_Channel4_5_IRQn
#define TELEMETRY_DMA_TX_IRQHandler     DMA1_Channel4_5_IRQHandler
#define TELEMETRY_DMA_TX_FLAG_TC        DMA1_IT_TC4
#define TELEMETRY_USART_IRQHandler      USART2_IRQHandler
#define TELEMETRY_USART_IRQn            USART2_IRQn
#define TELEMETRY_DIR_OUTPUT()          
#define TELEMETRY_DIR_INPUT()           
/*
F072 IRQs
#define DMA1_Channel1_IRQHandler          DMA1_Ch1_IRQHandler
#define DMA1_Channel2_3_IRQHandler        DMA1_Ch2_3_DMA2_Ch1_2_IRQHandler
#define DMA1_Channel4_5_IRQHandler        DMA1_Ch4_7_DMA2_Ch3_5_IRQHandler
#define DMA1_Channel4_5_6_7_IRQHandler    DMA1_Ch4_7_DMA2_Ch3_5_IRQHandler

#define DMA1_Ch1_IRQn                     DMA1_Channel1_IRQn
#define DMA1_Ch2_3_DMA2_Ch1_2_IRQn        DMA1_Channel2_3_IRQn           
#define DMA1_Channel4_5_IRQn              DMA1_Channel4_5_6_7_IRQn
#define DMA1_Ch4_7_DMA2_Ch3_5_IRQn        DMA1_Channel4_5_6_7_IRQn 

*/

// Heartbeat

  // #define TRAINER_MODULE_HEARTBEAT
  // #define HEARTBEAT_RCC_AHB1Periph      RCC_AHBPeriph_GPIOC
  // #define HEARTBEAT_RCC_APB2Periph      RCC_APB2Periph_USART6
  // #define HEARTBEAT_GPIO                GPIOC
  // #define HEARTBEAT_GPIO_PIN            GPIO_Pin_7  // PC.07
  // #define HEARTBEAT_GPIO_PinSource      GPIO_PinSource7
  // #define HEARTBEAT_GPIO_AF_SBUS        GPIO_AF_USART6
  // #define HEARTBEAT_GPIO_AF_CAPTURE     GPIO_AF_TIM3
  // #define HEARTBEAT_USART               USART6
  // #define HEARTBEAT_USART_IRQHandler    USART6_IRQHandler
  // #define HEARTBEAT_USART_IRQn          USART6_IRQn
  // #define HEARTBEAT_DMA_Stream          DMA2_Stream1
  // #define HEARTBEAT_DMA_Channel         DMA_Channel_5

// pwm, requires wiring BL pad to PC9 pad
  #define BACKLIGHT_RCC_APB1Periph      RCC_APB1Periph_TIM3
  #define BACKLIGHT_RCC_AHB1Periph      RCC_AHBPeriph_GPIOC
  #define BACKLIGHT_GPIO                GPIOC
  #define BACKLIGHT_GPIO_PIN            GPIO_Pin_9
  #define BACKLIGHT_TIMER_FREQ          (PERI1_FREQUENCY * TIMER_MULT_APB1)
  #define BACKLIGHT_TIMER               TIM3
  #define BACKLIGHT_GPIO_PinSource      GPIO_PinSource9
  #define BACKLIGHT_GPIO_AF             GPIO_AF_0
  #define BACKLIGHT_CCMR2               TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2 // Channel4, PWM
  #define BACKLIGHT_CCER                TIM_CCER_CC4P | TIM_CCER_CC4E
  #define BACKLIGHT_COUNTER_REGISTER    BACKLIGHT_TIMER->CCR4
  // std, fixed brightness
  #define BACKLIGHT_STD_RCC_APB1Periph      0
  #define BACKLIGHT_STD_RCC_AHB1Periph      RCC_AHBPeriph_GPIOF
  #define BACKLIGHT_STD_GPIO                GPIOF
  #define BACKLIGHT_STD_GPIO_PIN            GPIO_Pin_3

// Audio
//#define AUDIO_RCC_AHB1Periph            (RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_DMA1)
//#define AUDIO_RCC_APB1Periph            (RCC_APB1Periph_TIM6)
//#define AUDIO_OUTPUT_GPIO               GPIOA
//#define AUDIO_OUTPUT_GPIO_PIN           GPIO_Pin_8  // PA.08
//#define AUDIO_DMA_Stream                DMA1_Stream5
//#define AUDIO_DMA_Stream_IRQn           DMA1_Stream5_IRQn
//#define AUDIO_TIM_IRQn                  TIM6_DAC_IRQn
//#define AUDIO_TIM_IRQHandler            TIM6_DAC_IRQHandler
//#define AUDIO_DMA_Stream_IRQHandler     DMA1_Stream5_IRQHandler
//#define AUDIO_TIMER                     TIM6
//#define AUDIO_DMA                       DMA1

// Buzzer on TIMER 1
#define BUZZER_GPIO_PORT GPIOA
#define BUZZER_GPIO_PIN GPIO_Pin_8
#define BUZZER_GPIO_PinSource GPIO_PinSource8
#define BUZZER_RCC_AHBPeriph RCC_AHBPeriph_GPIOA
#define PWM_RCC_APB2Periph              RCC_APB2Periph_TIM1
#define PWM_TIMER                       TIM1

// Xms Interrupt TIMER 14
#define INTERRUPT_xMS_RCC_APB1Periph    RCC_APB1Periph_TIM14
#define INTERRUPT_xMS_TIMER             TIM14
#define INTERRUPT_xMS_IRQn              TIM14_IRQn
#define INTERRUPT_xMS_IRQHandler        TIM14_IRQHandler

// 2MHz Timer
#define TIMER_2MHz_RCC_APB1Periph       RCC_APB1Periph_TIM7
#define TIMER_2MHz_TIMER                TIM7

// Mixer scheduler timer
#define MIXER_SCHEDULER_TIMER_RCC_APB2Periph RCC_APB2Periph_TIM17
#define MIXER_SCHEDULER_TIMER                TIM17
#define MIXER_SCHEDULER_TIMER_FREQ           (PERI1_FREQUENCY * TIMER_MULT_APB1)
#define MIXER_SCHEDULER_TIMER_IRQn           TIM17_IRQn
#define MIXER_SCHEDULER_TIMER_IRQHandler     TIM17_IRQHandler

//all used RCC goes here
#define RCC_AHB1_LIST                   (I2C_RCC_AHB1Periph | BACKLIGHT_STD_RCC_APB1Periph | BACKLIGHT_RCC_AHB1Periph | LCD_RCC_AHB1Periph | KEYS_RCC_AHB1Periph | BUZZER_RCC_AHBPeriph | EXTMODULE_RCC_AHBPeriph | CRC_RCC_AHB1Periph | TELEMETRY_RCC_AHB1Periph | AUX_SERIAL_RCC_AHB1Periph | AUX2_SERIAL_RCC_AHB1Periph | ADC_RCC_AHB1Periph)
#define RCC_APB1_LIST                   (I2C_RCC_APB1Periph | INTERRUPT_xMS_RCC_APB1Periph | TIMER_2MHz_RCC_APB1Periph | TELEMETRY_RCC_APB1Periph | BACKLIGHT_STD_RCC_APB1Periph | BACKLIGHT_RCC_APB1Periph | RCC_APB1Periph_USB | AUX2_SERIAL_RCC_APB1Periph)
#define RCC_APB2_LIST                   (MIXER_SCHEDULER_TIMER_RCC_APB2Periph | PWM_RCC_APB2Periph | INTMODULE_RCC_APB2Periph | EXTMODULE_RCC_APB2Periph | AUX_SERIAL_RCC_APB2Periph | ADC_RCC_APB2Periph)

#endif // _HAL_H_
