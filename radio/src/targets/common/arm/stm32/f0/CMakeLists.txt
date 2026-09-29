set(CPU_FAMILY STM32)
set(MCU cortex-m0)

# STM32 HAL/LL library
set(STM32CUBE_DIR ${THIRDPARTY_DIR}/STM32F0xx_HAL_Driver)
include_directories(${STM32CUBE_DIR}/Inc)

add_library(stm32cube_ll OBJECT EXCLUDE_FROM_ALL
  # LL drivers
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_gpio.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_tim.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_dma.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_usart.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_rcc.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_i2c.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_exti.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_adc.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_spi.c
  ${STM32CUBE_DIR}/Src/stm32f0xx_ll_usb.c
  # HAL drivers
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_pwr.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_pwr_ex.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_rcc.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_rtc.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_i2c.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_pcd.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_pcd_ex.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_flash.c
#  ${STM32CUBE_DIR}/Src/stm32f0xx_hal_flash_ex.c
  )

# stm32f0xx_ll_usb.c only needs a few endpoint type constants that the HAL
# headers leave commented out (stm32f0xx_ll_usb.h).
target_compile_definitions(stm32cube_ll PRIVATE
  BTABLE_ADDRESS=0x000
  EP_TYPE_CTRL=0U
  USB_EP_CONTROL=0x0200
  USB_EP_BULK=0x0000
  USE_USB_DOUBLE_BUFFER=0
  )

target_compile_options(stm32cube_ll PRIVATE -DUSE_FULL_LL_DRIVER)
target_include_directories(stm32cube_ll PRIVATE
  ${THIRDPARTY_DIR}/CMSIS/Device/ST/STM32F0xx/Include
  ${THIRDPARTY_DIR}/CMSIS/Include)

set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:stm32cube_ll>)
set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:stm32cube_ll>)

# StdPeriph library
set(STM32LIB_DIR ${THIRDPARTY_DIR})
include_directories(${STM32LIB_DIR}/CMSIS/Device/ST/STM32F0xx/Include)
include_directories(${STM32LIB_DIR}/CMSIS/Include)

add_definitions(-DSTM32F0)

add_library(stm32_stdperiph OBJECT EXCLUDE_FROM_ALL
  targets/common/arm/stm32/f0/system_stm32f0xx.c
# targets/flysky/startup_stm32f072.s
)

set(FIRMWARE_SRC ${FIRMWARE_SRC} $<TARGET_OBJECTS:stm32_stdperiph>)
set(BOOTLOADER_SRC ${BOOTLOADER_SRC} $<TARGET_OBJECTS:stm32_stdperiph>)
