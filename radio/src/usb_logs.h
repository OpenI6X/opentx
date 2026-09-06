#pragma once
#if defined(USB_LOGS)
extern uint8_t logDelay;
void usbLogsInit();
void usbLogsWrite();
#endif
