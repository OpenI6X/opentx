option(SHUTDOWN_CONFIRMATION "Shutdown confirmation" OFF)
option(PCBI6X_ELRS "Enable ExpressLRS configuration tool" NO) # ~4.0kB
option(CRSF_EXTENDED_TYPES "Enable Support for CRSF STRING and FLOAT types" NO) # ~0.2kB
option(PCBI6X_HELLO "Enable Hello World example tool" NO)
option(PCBI6X_INAV "Enable INAV Lite telemetry screen" NO) # ~2kB
option(USB_VBUS "Removes manual USB connection menu option, requires wiring USB VBUS to PA15 pad" NO)
option(USB_MSD "Enable USB mass storage mode" YES) # 3.2kB
option(AFHDS2A_LQI_CH "Send RSSI at channel 1-17" OFF)
option(DFPLAYER "Enables DFPlayer on AUX3 TX" NO) # ~1.7kB
option(FAKE_RSSI "Enables converting TRSS to RSSI data for receivers not sending RSSI" NO)
option(SBUS_TRAINER "Enables SBUS Trainer mode and trainer lost/back sounds" NO) # ~0.6kB

if(PCB STREQUAL I6X)
  set(PWR_BUTTON "SWITCH" CACHE STRING "Pwr button type (PRESS/SWITCH)")
  set(SPLASH "OFF")
  set(CPU_TYPE STM32F0)
  set(CPU_TYPE_FULL STM32F072)  # for size report
  set(LINKER_SCRIPT targets/flysky/STM32F072VB_FLASH.ld)
  set(HAPTIC NO)
  set(FLAVOUR i6x)
  set(NANO YES)
  set(LUA NO)
  set(GUI_DIR 128x64)
  set(NAVIGATION_TYPE 9x)
  set(BITMAPS_TARGET 9x_bitmaps)
  set(FONTS_TARGET 9x_fonts_1bit)
  set(LCD_DRIVER lcd_driver.cpp)
  set(DEFAULT_MODE 2)
  set(GVAR_SCREEN model_gvars.cpp)
  set(AUX_SERIAL_DRIVER ../common/arm/stm32/aux_serial_driver.cpp)
  set(DEFAULT_TEMPLATE_SETUP "0" CACHE STRING "Default channel order")

  add_definitions(-DCROSSFIRE)
  # add_definitions(-DNAVIGATION_POT1) # ~324B
  # add_definitions(-DNAVIGATION_POT2)
  add_definitions(-DSTM32F072)
  add_definitions(-DPWR_BUTTON_${PWR_BUTTON})
  add_definitions(-DEEPROM_VARIANT=0)
  add_definitions(-DSOFTWARE_VOLUME)
  add_definitions(-DAFHDS2A)
endif()

if(NOT AFHDS2A_LQI_CH STREQUAL OFF)
  add_definitions(-DAFHDS2A_LQI_CH=${AFHDS2A_LQI_CH})
endif()

if(USB_VBUS STREQUAL YES)
  add_definitions(-DUSB_VBUS)
endif()

if(FLYSKY_GIMBAL STREQUAL YES)
  add_definitions(-DFLYSKY_GIMBAL)
  set(TARGET_SRC
    ${TARGET_SRC}
    ../common/arm/stm32/flysky_gimbal_driver.cpp
    )
endif()

if(DFPLAYER STREQUAL YES)
  add_definitions(-DDFPLAYER)
  set(TARGET_SRC
  ${TARGET_SRC}
  dfplayer_driver.cpp
  )
endif()

if(FAKE_RSSI STREQUAL YES)
  add_definitions(-DFAKE_RSSI)
endif()

if(CRSF_EXTENDED_TYPES STREQUAL YES)
  add_definitions(-DCRSF_EXTENDED_TYPES)
endif()

if(PCBI6X_ELRS STREQUAL YES)
  add_definitions(-DPCBI6X_ELRS)
  set(TARGET_SRC
    ${TARGET_SRC}
    tools/elrs.cpp
    )
endif()

if(PCBI6X_HELLO STREQUAL YES)
  add_definitions(-DPCBI6X_HELLO)
set(TARGET_SRC
  ${TARGET_SRC}
  tools/hello.cpp
  )
endif()

if((PCBI6X_INAV STREQUAL YES)) # AND ((TRANSLATIONS STREQUAL EN) OR (NOT PCBI6X_ELRS STREQUAL YES)))
  add_definitions(-DPCBI6X_INAV)
set(TARGET_SRC
  ${TARGET_SRC}
  tools/inav.cpp
  )
endif()

if(USB_MSD STREQUAL YES)
  add_definitions(-DUSB_MSD)
endif()

set(TARGET_SRC
  ${TARGET_SRC}
  A7105_SPI.cpp
  AFHDS2A_a7105.cpp
  intmodule_pulses_driver.cpp
  trainer_driver.cpp
  ../common/arm/stm32/timers_driver.cpp
  )

if(SBUS_TRAINER STREQUAL YES)
  add_definitions(-DSBUS)
  add_definitions(-DSBUS_TRAINER)
  set(PULSES_SRC
    ${PULSES_SRC}
    ../sbus.cpp
    )
endif()

set(HSE_VALUE 8000000)
set(SDCARD NO) # ~24kB
set(EEPROM EEPROM_RLC)
set(TARGET_DIR flysky)
set(PPM_LIMITS_SYMETRICAL YES)
set(DEFAULT_TEMPLATE_SETUP 21) # AETR
add_definitions(-DPCBI6X -DBUZZER)
# add_definitions(-DMENU_DIAG_ANAS_KEYS) # ~600B

if(NOT DEFAULT_TEMPLATE_SETUP STREQUAL "")
  add_definitions(-DDEFAULT_TEMPLATE_SETUP=${DEFAULT_TEMPLATE_SETUP})
endif()

set(GUI_SRC ${GUI_SRC}
  model_input_edit.cpp
  model_mix_edit.cpp
  model_display.cpp
  radio_diagkeys.cpp
  radio_diaganas.cpp
  radio_hardware.cpp
  view_channels.cpp
  view_telemetry.cpp
  view_about.cpp
  view_channels.cpp
  )

set(SRC ${SRC} buzzer.cpp)

set(TARGET_SRC
  ${TARGET_SRC}
  board.cpp
  buzzer_driver.cpp
  backlight_driver.cpp
  extmodule_driver.cpp
  adc_driver.cpp
  i2c_driver.cpp
  crc_driver.cpp
  ../common/arm/stm32/mixer_scheduler_driver.cpp
  )

set(FIRMWARE_TARGET_SRC
  ${FIRMWARE_TARGET_SRC}
  lcd_driver.cpp
  pwr_driver.cpp
  startup_stm32f072.s
  )
set(FIRMWARE_DEPENDENCIES ${FIRMWARE_DEPENDENCIES} ${BITMAPS_TARGET})
