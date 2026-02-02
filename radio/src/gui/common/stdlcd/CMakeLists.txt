set(GUI_SRC
  ${GUI_SRC}
  ../common/stdlcd/widgets.cpp
  ../common/stdlcd/popups.cpp
  )

#define (PCBI6X)
#if(PXX2 OR LUA OR MULTIMODULE OR PCBI6X)
  set(GUI_SRC ${GUI_SRC} ../common/stdlcd/radio_tools.cpp)
#endif()

if(HELI)
  set(GUI_SRC ${GUI_SRC} ../common/stdlcd/model_heli.cpp)
endif()

add_definitions(-DGRAPHICS)
set(GUI_SRC
 ${GUI_SRC}
  ../common/stdlcd/model_inputs_mixes_common.cpp
  ../common/stdlcd/model_inputs.cpp
  ../common/stdlcd/model_mixes.cpp
  ../common/stdlcd/view_text.cpp
  ../common/stdlcd/radio_trainer.cpp
)
set(CURVE_EDIT_SRC )


set(GUI_SRC
  ${GUI_SRC}
  ../common/stdlcd/model_curves.cpp
  model_curve_edit.cpp
  )

if(SDCARD)
  set(GUI_SRC
    ${GUI_SRC}
    ../common/stdlcd/radio_sdmanager.cpp
    )
endif()
