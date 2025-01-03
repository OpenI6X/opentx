set(CPU_FAMILY STM32)
set(MCU cortex-m0)
set(STM32LIB_DIR ${THIRDPARTY_DIR}/STM32F0xx_StdPeriph_Lib_V1.5.0/Libraries)
include_directories(${STM32LIB_DIR}/STM32F0xx_StdPeriph_Driver/inc)
include_directories(${STM32LIB_DIR}/CMSIS/Device/ST/STM32F0xx/Include)
include_directories(${STM32LIB_DIR}/CMSIS/include)
add_definitions(-DSTM32F0)
set(STM32LIB_SRC
  ${STM32LIB_SRC}
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_crs.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_misc.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_gpio.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_dbgmcu.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_rcc.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_spi.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_i2c.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_dma.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_rtc.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_pwr.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_usart.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_adc.c
  STM32F0xx_StdPeriph_Driver/src/stm32f0xx_crc.c
  )
set(FIRMWARE_TARGET_SRC ${FIRMWARE_TARGET_SRC} ../common/arm/stm32/f0/system_stm32f0xx.c)

