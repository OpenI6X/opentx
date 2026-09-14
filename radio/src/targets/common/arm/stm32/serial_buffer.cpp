#include "opentx.h"

#include "serial_buffer_union.h"

extern "C" {
extern volatile uint32_t APP_Tx_ptr_in;
extern volatile uint32_t APP_Rx_ptr_out;
extern uint32_t APP_Rx_length;
extern uint8_t USB_Tx_State;
}

SerialBufferUnion serialBuffer;

void serialTxBufferClear()
{
  serialBuffer.auxSerialTxFifo.clear();
}

bool usbTelemMirrorActive()
{
  return getSelectedUsbMode() == USB_SERIAL_MODE && g_eeGeneral.usbSerialMode == USB_SERIAL_MODE_TELEMETRY_MIRROR;
}
