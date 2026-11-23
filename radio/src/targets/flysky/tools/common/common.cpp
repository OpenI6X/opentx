
#include "opentx.h"

void crsfNotEnabledPopup(event_t event)
{
  if (!isModuleCrossfire(EXTERNAL_MODULE)) {
    // dialogs.lua drawNoModule()
    lcdDrawText(2, 0, "No ExpressLRS", MIDSIZE);
    lcdDrawText(2, 14, "Enable a CRSF External\036module in Model settings");
    if (event == EVT_KEY_BREAK(KEY_EXIT)) popMenu();
    return;
  }
}