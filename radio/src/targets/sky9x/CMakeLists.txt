set(ARCH ARM)
set(LUA NO)
if(PCB STREQUAL 9XRPRO)
  add_definitions(-Dat91sam3s8 -DREVX)
  set(FLAVOUR 9xrpro)
  set(LINKER_SCRIPT targets/sky9x/sam3s8c_flash.ld)
  set(TARGET_SRC ${TARGET_SRC} rtc_driver.cpp)
  set(BOOTLOADER bootflash8.lbm)
elseif(PCB STREQUAL AR9X)
  add_definitions(-Dat91sam3s8 -DAR9X -DSOFTWARE_VOLUME)
  set(FLAVOUR ar9x)
  include_directories(${THIRDPARTY_DIR}/xdk-asf-3.31.0/sam/drivers/twi)
  set(SRC ${SRC} ${THIRDPARTY_DIR}/xdk-asf-3.31.0/sam/drivers/twi/twi.c)
  set(TARGET_SRC ${TARGET_SRC} i2c_driver.cpp rtc_ds3231_driver.cpp)
  set(LINKER_SCRIPT targets/sky9x/sam3s8c_flash.ld)
  set(BOOTLOADER bootflash8.lbm)
else()
  set(FLAVOUR sky9x)
  option(PCBREV "PCB Revision" REVC)
  set(TARGET_SRC ${TARGET_SRC} coproc_driver.cpp rotenc_driver.cpp rtc_driver.cpp)
  add_definitions(-DROTARY_ENCODERS=1)
  set(BOOTLOADER bootflash4.lbm)
  if(PCBREV STREQUAL REVA)
    add_definitions(-DDREVA)
    set(LINKER_SCRIPT targets/sky9x/sam3s2c_flash.ld)
  elseif(PCBREV STREQUAL REVB)
    add_definitions(-DDREVB -DCOPROCESSOR)
    set(LINKER_SCRIPT targets/sky9x/sam3s4c_flash.ld)
  else()
    add_definitions(-DDREVC -DCOPROCESSOR)
    set(LINKER_SCRIPT targets/sky9x/sam3s4c_flash.ld)
  endif()
endif()
set(MCU cortex-m3)
set(SDCARD YES)
set(HAPTIC YES)
set(EEPROM EEPROM_RAW)
set(GUI_DIR 128x64)
set(TARGET_DIR sky9x)
set(FIRMWARE_DEPENDENCIES ${FIRMWARE_DEPENDENCIES} 9x_bitmaps)
add_definitions(-DPCBSKY9X -DPPM_PIN_SERIAL)
add_definitions(-DAUDIO -DVOICE -DRTCLOCK -DVARIO)
add_definitions(-DEEPROM_VARIANT=0)
set(GUI_SRC ${GUI_SRC} ${9X_GUI_SRC} radio_hardware.cpp view_telemetry.cpp view_text.cpp view_about.cpp)
set(FIRMWARE_TARGET_SRC ${FIRMWARE_TARGET_SRC} core_cm3.c board_lowlevel.c crt.c vectors_sam3s.c)
set(FIRMWARE_TARGET_SRC ${FIRMWARE_TARGET_SRC} lcd_driver.cpp pwr_driver.cpp
  usb/device/core/USBD_UDP.c usb/device/core/USBDDriver.c
  usb/device/massstorage/MSDDriver.c usb/device/massstorage/MSDDStateMachine.c usb/device/massstorage/MSDLun.c
  usb/device/massstorage/MSDDriverDescriptors.c usb/device/massstorage/SBCMethods.c usb/common/core/USBEndpointDescriptor.c
  usb/common/core/USBGenericRequest.c usb/common/core/USBFeatureRequest.c usb/common/core/USBInterfaceRequest.c
  usb/common/core/USBGetDescriptorRequest.c usb/common/core/USBSetAddressRequest.c usb/common/core/USBSetConfigurationRequest.c
  usb/common/core/USBConfigurationDescriptor.c usb/common/core/USBGenericDescriptor.c MEDSdcard.c
  )
set(TARGET_SRC
  ${TARGET_SRC}
  board.cpp
  telemetry_driver.cpp
  serial2_driver.cpp
  adc_driver.cpp
  eeprom_driver.cpp
  pulses_driver.cpp
  keys_driver.cpp
  audio_driver.cpp
  buzzer_driver.cpp
  haptic_driver.cpp
  sdcard_driver.cpp
  massstorage.cpp
)
set(SRC ${SRC} debug.cpp)
set(FIRMWARE_SRC ${FIRMWARE_SRC} loadboot.cpp)