set(LUA "NO" CACHE STRING "Lua scripts (YES/NO/NO_MODEL_SCRIPTS)")
set_property(CACHE LUA PROPERTY STRINGS YES NO NO_MODEL_SCRIPTS)
set(LUA_SCRIPT_LOAD_MODE "" CACHE STRING "Script loading mode and compilation flags [btTxcd] (see loadScript() API docs). Blank for default ('bt' on radio, 'T' on SIMU/DEBUG builds)")
option(LUA_COMPILER "Pre-compile and save Lua scripts" OFF)
option(LUA_ALLOCATOR_TRACER "Trace Lua memory (de)allocations to debug port (also needs DEBUG=YES NANO=NO)" OFF)

option(USB_SERIAL "Enable USB serial (CDC)" OFF) # 1.1kB
set(ARCH ARM)

if(NOT CPU_TYPE STREQUAL STM32F0)
  set(STM32USB_DIR ${THIRDPARTY_DIR}/STM32_USB-Host-Device_Lib_V2.2.0/Libraries)
  add_definitions(-DSTM32 -DVARIO)
  include_directories(${RADIO_SRC_DIRECTORY}/targets/common/arm/stm32)
  include_directories(${STM32USB_DIR}/STM32_USB_OTG_Driver/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Core/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Class/msc/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Class/hid/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Class/cdc/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Core/inc/)
  include_directories(${STM32USB_DIR}/STM32_USB_OTG_Driver/inc)

  set(STM32USB_SRC
    STM32_USB_OTG_Driver/src/usb_core.c
    STM32_USB_OTG_Driver/src/usb_dcd.c
    STM32_USB_OTG_Driver/src/usb_dcd_int.c
    STM32_USB_Device_Library/Core/src/usbd_core.c
    STM32_USB_Device_Library/Core/src/usbd_ioreq.c
    STM32_USB_Device_Library/Core/src/usbd_req.c
    )
else() # STM32F0
  set(STM32USB_DIR ${THIRDPARTY_DIR}/STM32F0x2_USB-FS-Device_Lib_V1.0.0/Libraries)
  add_definitions(-DSTM32 -DVARIO)
  include_directories(${RADIO_SRC_DIRECTORY}/targets/common/arm/stm32)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Driver/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Core/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Class/msc/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Class/hid/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Class/cdc/inc)
  include_directories(${STM32USB_DIR}/STM32_USB_Device_Library/Core/inc/)

  set(STM32USB_SRC
    STM32_USB_Device_Driver/src/usb_core.c
    STM32_USB_Device_Driver/src/usb_dcd.c
    STM32_USB_Device_Driver/src/usb_dcd_int.c
    STM32_USB_Device_Library/Core/src/usbd_core.c
    STM32_USB_Device_Library/Core/src/usbd_ioreq.c
    STM32_USB_Device_Library/Core/src/usbd_req.c
    )
endif()

set(TARGET_SRC
  ${TARGET_SRC}
  ../common/arm/stm32/rtc_driver.cpp
  ../common/arm/stm32/cpu_id.cpp
  ${AUX_SERIAL_DRIVER}
  )
set(FIRMWARE_TARGET_SRC
  ${FIRMWARE_TARGET_SRC}
  ../common/arm/stm32/delays.cpp
  )

if(NOT CPU_TYPE STREQUAL STM32F0)
 add_definitions(-DLUA_INPUTS)
endif()

set(FIRMWARE_TARGET_SRC
 ${FIRMWARE_TARGET_SRC}
 ../common/arm/stm32/usb_bsp.c
 ../common/arm/stm32/usbd_pwr.c
 ../common/arm/stm32/usbd_desc.c
 ../common/arm/stm32/usbd_usr.cpp
 ../common/arm/stm32/usb_driver.cpp
 ../common/arm/stm32/flash_driver.cpp
 ../common/arm/stm32/usbd_storage_msd.cpp
 )

if(DEBUG OR CLI OR USB_SERIAL)
  set(STM32USB_SRC
    ${STM32USB_SRC}
    STM32_USB_Device_Library/Class/cdc/src/usbd_cdc_core.c
  )
  set(FIRMWARE_TARGET_SRC
    ${FIRMWARE_TARGET_SRC}
    ../common/arm/stm32/usbd_cdc.cpp
  )
  add_definitions(-DUSB_SERIAL)

# disable USB_SERIAL if it will exceed flash size
# if ((NOT TRANSLATIONS STREQUAL EN) AND DFPLAYER STREQUAL YES AND USB_MSD STREQUAL YES AND PCBI6X_ELRS STREQUAL YES AND PCBI6X_INAV STREQUAL YES AND USB_SERIAL STREQUAL YES)
#  message(WARNING "USB_SERIAL will be disabled")
#  remove_definitions(-DUSB_SERIAL)
# endif()
endif()
set(STM32USB_SRC
  ${STM32USB_SRC}
  STM32_USB_Device_Library/Class/msc/src/usbd_msc_data.c
  STM32_USB_Device_Library/Class/msc/src/usbd_msc_scsi.c
  STM32_USB_Device_Library/Class/msc/src/usbd_msc_bot.c
  STM32_USB_Device_Library/Class/msc/src/usbd_msc_core.c
  )
set(FIRMWARE_TARGET_SRC
  ${FIRMWARE_TARGET_SRC}
  ../common/arm/stm32/usbd_hid_joystick.c
  )
if(GVARS)
  set(GUI_SRC
    ${GUI_SRC}
    ${GVAR_SCREEN}
    )
endif()
set(FIRMWARE_SRC
  ${FIRMWARE_SRC}
  serial.cpp
  syscalls.c
  )
foreach(FILE ${STM32LIB_SRC})
  set(FIRMWARE_SRC
    ${FIRMWARE_SRC}
    ${STM32LIB_DIR}/${FILE}
    )
endforeach()

foreach(FILE ${STM32USB_SRC})
  set(FIRMWARE_SRC
    ${FIRMWARE_SRC}
    ${STM32USB_DIR}/${FILE}
    )
endforeach()
if(NOT CPU_TYPE STREQUAL STM32F0)
set(SRC
  ${SRC}
  io/bootloader_flash.cpp
  )
endif()
