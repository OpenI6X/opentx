/**
 * ExpressLRS V3 configurator for i6X based on elrsV2/3.lua
 * @author Jan Kozak (ajjjjjjjj)
 *
 */
#include "opentx.h"

enum cmd_status {
    STATUS_READY = 0,
    STATUS_START = 1,       // user has clicked the command to execute
    STATUS_PROGRESS = 2,   // command is executing
    STATUS_CONFIRMATION_NEEDED = 3,     // command pending user OK
    STATUS_CONFIRM = 4,   // user has confirmed
    STATUS_CANCEL = 5,      // user has requested cancel
    STATUS_POLL = 6,       // UI is requesting status update
};

enum data_type {
    TYPE_UINT8 = 0,
    TYPE_INT8 = 1,
    TYPE_UINT16 = 2,
    TYPE_INT16 = 3,
    TYPE_FLOAT = 8,
    TYPE_SELECT = 9,
    TYPE_STRING = 10,
    TYPE_FOLDER = 11,
    TYPE_INFO = 12,
    TYPE_COMMAND = 13,
    TYPE_BACK = 14,
    TYPE_DEVICE = 15,
    TYPE_DEVICES_FOLDER = 16
};

#define CRSF_FRAMETYPE_DEVICE_PING 0x28
#define CRSF_FRAMETYPE_DEVICE_INFO 0x29
#define CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY 0x2B
#define CRSF_FRAMETYPE_PARAMETER_READ 0x2C
#define CRSF_FRAMETYPE_PARAMETER_WRITE 0x2D
#define CRSF_FRAMETYPE_ELRS_STATUS 0x2E

/**
 * INT16 and FLOAT support:
 * Values (val,min,max[,step]) are keep in buffer after name.
 * prec - how many digits in fractional part.
 * FLOAT: { VALUE 4B | MIN 4B | MAX 4B | STEP 4B } = 16B
 * INT16: { VALUE 2B | MIN 2B | MAX 2B } =  6B
*/
struct Parameter {
  uint16_t offset;
  uint8_t type;
  uint8_t nameLength;
  uint8_t size;         // INT8/16/FLOAT/SELECT size
  uint8_t id;
  union {
#if defined(CRSF_EXTENDED_TYPES)
    int32_t value;
#else
    int16_t value;
#endif
    struct {
      uint8_t timeout;      // COMMAND
      uint8_t lastStatus;   // COMMAND
      uint8_t status;       // COMMAND
      uint8_t infoOffset;   // COMMAND, paramData info offset
    };
  };
};

struct ParamFunctions {
  void (*load)(Parameter*, uint8_t *, uint8_t);
  void (*save)(Parameter*);
  void (*display)(Parameter*, uint8_t, uint8_t);
};

static constexpr uint16_t BUFFER_SIZE = CTOOL_DATA_SIZE - 8/*DEVICES_MAX_COUNT*/;
static uint8_t *buffer = &reusableBuffer.cToolData[0];
static uint16_t bufferOffset = 0;

static uint8_t *paramData = &reusableBuffer.cToolData[0];
static uint32_t paramDataLen = 0;

static constexpr uint8_t MAX_PARAMS_COUNT = 18;
static Parameter *params = (Parameter *)&reusableBuffer.cToolData[BUFFER_SIZE - MAX_PARAMS_COUNT * sizeof(Parameter)];
static uint8_t allocatedParamsCount = 0;

static constexpr uint8_t DEVICES_MAX_COUNT = 8;
static uint8_t *deviceIds = &reusableBuffer.cToolData[BUFFER_SIZE];
//static uint8_t deviceIds[DEVICES_MAX_COUNT];
static uint8_t devicesLen = 0;

// Button and UI state IDs
static constexpr uint8_t backButtonId = 100;
static constexpr uint8_t otherDevicesId = 101;
enum { BTN_NONE, BTN_BACK, BTN_DEVICES };

// Device communication state
struct DeviceState {
  uint8_t id = 0xEE;
  uint8_t handsetId = 0xEF;
  uint8_t isELRS_TX = 0;
  uint8_t paramCount = 0;
  char name[20] = {};
};
static DeviceState device;

// UI navigation state
struct UIState {
  uint8_t btnState = BTN_NONE;
  uint8_t lineIndex = 1;
  uint8_t pageOffset = 0;
  Parameter * paramPopup = nullptr;
};
static UIState ui;

// Link statistics for ELRS TX modules
struct LinkStat {
  uint16_t good;
  uint8_t bad;
  uint8_t flags;
} static linkstat = {};

// Parameter loading state
struct ParamLoadState {
  uint8_t id = 1;
  uint8_t chunk = 0;
  tmr10ms_t timeout = 0;
  uint8_t expectedCount = 0;
  int8_t expectedChunks = -1;
  uint8_t allLoaded = 0;
  uint8_t currentFolderId = 0;
};
static ParamLoadState paramLoad;

// UI display state
struct DisplayState {
  char elrsInfo[20] = {};
  uint8_t titleShowWarn = 0;
  tmr10ms_t titleShowWarnTimeout = 100;
  tmr10ms_t devicesRefreshTimeout = 50;
  tmr10ms_t linkstatTimeout = 100;
};
static DisplayState display;

static constexpr uint8_t STRING_LEN_MAX = 15; // without trailing \0
static event_t currentEvent;

// UI Display Constants
static constexpr uint8_t COL1           =  0;
static constexpr uint8_t COL2           = 70;
static constexpr uint8_t MAX_LINE_INDEX =  6;
static constexpr uint8_t TEXT_YOFFSET   =  1;
static constexpr uint8_t TEXT_SIZE      =  8;
static constexpr uint8_t BAR_HEIGHT     =  8;

// Buffer Offset Constants (for parameter data layout)
static constexpr uint8_t PARAM_NAME_OFFSET     = 0;  // offset from param->offset
static constexpr uint8_t PARAM_VALUE_SIZE_MULT = 2;  // for min/max calculation
static constexpr uint8_t PARAM_DEFAULT_OFFSET  = 3;  // offset index for default value in integer params

#define getTime           get_tmr10ms
#define EVT_VIRTUAL_EXIT  EVT_KEY_BREAK(KEY_EXIT)
#define EVT_VIRTUAL_ENTER EVT_KEY_BREAK(KEY_ENTER)
#define EVT_VIRTUAL_NEXT  EVT_KEY_FIRST(KEY_DOWN)
#define EVT_VIRTUAL_PREV  EVT_KEY_FIRST(KEY_UP)

static constexpr uint8_t RESULT_NONE = 0;
static constexpr uint8_t RESULT_OK = 2;
static constexpr uint8_t RESULT_CANCEL = 1;

static void storeParam(Parameter * param);
static void clearData();
static void addBackButton();
static void reloadAllParam();
static Parameter * getParam(uint8_t line);
static void paramBackExec(Parameter * param);
static void parseDeviceInfoMessage(uint8_t* data);
static void parseParameterInfoMessage(uint8_t* data, uint8_t length);
static void parseElrsInfoMessage(uint8_t* data);
static void runPopupPage(event_t event);
static void runDevicePage(event_t event);
static void lcd_title();
static void lcd_warn();
static void handleDevicePageEvent(event_t event);

static void luaLcdDrawGauge(coord_t x, coord_t y, coord_t w, coord_t h, int32_t val, int32_t max) {
  uint8_t len = limit<uint8_t>(1, w*val/max, w);
  lcdDrawSolidFilledRect(x+len, y, w - len, h-2);
}

static void bufferPush(char * data, uint8_t len) {
  memcpy(&buffer[bufferOffset], data, len);
  bufferOffset += len;
}

static void resetParamData() {
  paramData = &reusableBuffer.cToolData[bufferOffset];
  paramDataLen = 0;
}

static void crossfireTelemetryCmd(const uint8_t cmd, const uint8_t index, const uint8_t * data, const uint8_t size) {
  // TRACE("crsf cmd %x %x %x", cmd, index, size);
  uint8_t crsfPushData[3 + size] = { device.id, device.handsetId, index };
  for (uint32_t i = 0; i < size; i++) {
    crsfPushData[3 + i] = data[i];
  }
  crossfireTelemetryPush(cmd, crsfPushData, sizeof(crsfPushData));
}

static void crossfireTelemetryCmd(const uint8_t cmd, const uint8_t index, const uint8_t value) {
  crossfireTelemetryCmd(cmd, index, &value, 1);
}

static void crossfireTelemetryPing() {
  const uint8_t crsfPushData[2] = { 0x00, 0xEA };
  crossfireTelemetryPush(CRSF_FRAMETYPE_DEVICE_PING, (uint8_t *) crsfPushData, 2);
}

// static void updateParamsOffset() {
//   uint16_t paramsSize = (expectedParamsCount + 1) * sizeof(Parameter); // + 1 for button (EXIT/DEVICES)
//   params = (Parameter *)&reusableBuffer.cToolData[BUFFER_SIZE - paramsSize];
// }

static void clearData() {
  // TRACE("clearData %d", allocatedParamsCount);
  memclear(reusableBuffer.cToolData, BUFFER_SIZE); // Skip deviceIds
  ui.btnState = BTN_NONE;
  allocatedParamsCount = 0;
}

// Both buttons must be added as last ones because i cannot overwrite existing Id
static void addBackButton() {
  Parameter backBtnParam;
  backBtnParam.id = backButtonId;
  backBtnParam.nameLength = 1; // mark as present
  backBtnParam.type = TYPE_BACK;
  storeParam(&backBtnParam);
  ui.btnState = BTN_BACK;
}

static void addOtherDevicesButton() {
  Parameter otherDevicesParam;
  otherDevicesParam.id = otherDevicesId;
  otherDevicesParam.nameLength = 1;
  otherDevicesParam.type = TYPE_DEVICES_FOLDER;
  storeParam(&otherDevicesParam);
  ui.btnState = BTN_DEVICES;
}

static void reloadAllParam() {
  paramLoad.allLoaded = 0;
  paramLoad.id = 1;
  paramLoad.chunk = 0;
  paramDataLen = 0;
  bufferOffset = 0;
}

static Parameter * getParamById(const uint8_t id) {
  for (uint32_t i = 0; i < allocatedParamsCount; i++) {
    if (params[i].id == id)
      return &params[i];
  }
  return nullptr;
}

/**
 * Store param at its location or add new one if not found.
 */
static void storeParam(Parameter * param) {
  Parameter * storedParam = getParamById(param->id);
  if (storedParam == nullptr) {
    storedParam = &params[allocatedParamsCount];
    allocatedParamsCount++;
  }
  memcpy(storedParam, param, sizeof(Parameter));
}

static int32_t paramGetValue(uint8_t * data, uint8_t size) {
  int32_t result = 0;
  for (uint32_t i = 0; i < size; i++) {
    result = (result << 8) + data[i];
  }
  return result;
}

static int32_t paramGetMin(Parameter * param) {
  return paramGetValue(&buffer[param->offset + param->nameLength + (0 * param->size)], param->size);
}

static int32_t paramGetMax(Parameter * param) {
  return paramGetValue(&buffer[param->offset + param->nameLength + (1 * param->size)], param->size);
}

/**
 * Get param from line index taking only loaded current folder params into account.
 */
static Parameter * getParam(const uint8_t line) {
  return &params[line - 1];
}

//static void incrParam(int32_t step) {
//  Parameter * param = getParam(lineIndex);
//  int32_t min = paramGetMin(param);
//  int32_t max = paramGetMax(param);
//  param->value = limit<int32_t>(min, param->value + step, max);
//}

static void selectParam(int8_t step) {
  int32_t newLineIndex = ui.lineIndex + step;

  if (newLineIndex <= 0) {
    newLineIndex = allocatedParamsCount;
  } else if (newLineIndex > allocatedParamsCount) {
    newLineIndex = 1;
    ui.pageOffset = 0;
  }

  Parameter * param;
  do {
    param = getParam(newLineIndex);
    if (param != 0 && param->nameLength != 0) break;
    newLineIndex = newLineIndex + step;
    if (newLineIndex <= 0) newLineIndex = allocatedParamsCount;
    if (newLineIndex > allocatedParamsCount) newLineIndex = 1;
  } while (newLineIndex != ui.lineIndex);

  ui.lineIndex = newLineIndex;

  if (ui.lineIndex > MAX_LINE_INDEX + ui.pageOffset) {
    ui.pageOffset = ui.lineIndex - MAX_LINE_INDEX;
  } else if (ui.lineIndex <= ui.pageOffset) {
    ui.pageOffset = ui.lineIndex - 1;
  }
}

static bool isExistingDevice(uint8_t devId) {
//   TRACE("isExistingDevice %x", devId);
  for (uint32_t i = 0; i < devicesLen; i++) {
    if (deviceIds[i] == devId) {
      return true;
    }
  }
  return false;
}

static void unitLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  uint8_t len = strlen((char*)&data[offset]) + 1;
  bufferPush((char*)&data[offset], len);
}

static void unitDisplay(Parameter * param, uint8_t y, uint16_t offset) {
  lcdDrawText(lcdLastRightPos, y, (char *)&buffer[offset]);
}

static void paramIntegerDisplay(Parameter *param, uint8_t y, uint8_t attr) {
    int32_t value = param->value;
    // Unit string is at: name + 2*size (min+max) + [1 byte prec for FLOAT]
    uint32_t unitOffset = param->offset + param->nameLength + (PARAM_VALUE_SIZE_MULT * param->size);
    
    if (param->type == TYPE_FLOAT) {
#if defined(CRSF_EXTENDED_TYPES)
      uint8_t prec = buffer[unitOffset];
      if (prec > 0) {
        attr |= (prec == 1 ? PREC1 : PREC2);
      }
      unitOffset += 1; // skip prec byte
#else
      return;
#endif
    }
    
    // Cast value based on parameter type
    switch (param->type) {
      case TYPE_UINT8:
        lcdDrawNumber(COL2, y, (uint8_t)value, attr);
        break;
      case TYPE_INT8:
        lcdDrawNumber(COL2, y, (int8_t)value, attr);
        break;
      case TYPE_UINT16:
        lcdDrawNumber(COL2, y, (uint16_t)value, attr);
        break;
      default: // TYPE_INT16, TYPE_FLOAT, or others
#if defined(CRSF_EXTENDED_TYPES)
        lcdDrawNumber(COL2, y, (param->type == TYPE_INT16) ? (int16_t)value : (int32_t)value, attr);
#else
        lcdDrawNumber(COL2, y, (int16_t)value, attr);
#endif
        break;
    }
    unitDisplay(param, y, unitOffset);
}

static uint8_t findSelectMinValue(const uint8_t * data) {
  uint8_t min = 0;
  while (data[min] == ';') min++;
  return min;
}

static uint8_t findSelectValuesCount(const uint8_t * data) {
  uint8_t count = 0;
  while (*data) {
    if (*data == ';') {
      data++;
      continue;
    }
    count++;
    while (*data && *data != ';') data++;
  }
  return count;
}

static void paramIntegerLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  // Determine value size based on type
  uint8_t valueSize;
  uint8_t minmaxSize;
  
  if (param->type == TYPE_UINT16 || param->type == TYPE_INT16) {
    valueSize = 2;
    minmaxSize = 4; // 2 bytes each for min+max
  } else if (param->type == TYPE_FLOAT) {
#if defined(CRSF_EXTENDED_TYPES)
    valueSize = 4;
    minmaxSize = 4 + 4 + 1 + 4; // min + max + prec + step
#else
    return;
#endif
  } else {
    // TYPE_UINT8, TYPE_INT8, TYPE_SELECT
    valueSize = 1;
    minmaxSize = 2;
  }
  
  param->size = valueSize;
  uint32_t optionsLen = 0;
  
  // SELECT types have option string before the value
  if (param->type == TYPE_SELECT) {
    optionsLen = strlen((char*)&data[offset]) + 1; // + \0
  }
  
  param->value = paramGetValue(&data[offset + optionsLen], valueSize);
  
  if (param->type == TYPE_SELECT) {
    uint8_t minVal = findSelectMinValue(&data[offset]);
    uint8_t maxVal = minVal + findSelectValuesCount(&data[offset]) - 1;
    bufferPush((char*)&minVal, 1);
    bufferPush((char*)&maxVal, 1);
    bufferPush((char*)&data[offset], optionsLen);
  } else {
    bufferPush((char *)&data[offset + valueSize], minmaxSize); // min + max
  }
  
  // Unit string follows: [options/] value min max [prec for FLOAT] unit
  uint8_t unitDataOffset = offset + optionsLen + valueSize + minmaxSize + valueSize; // [value] [min] [max] [default] [unit]
  unitLoad(param, data, unitDataOffset);
}

static void paramStringDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  char * str = (char *)&buffer[param->offset + param->nameLength];
  if (param->type == TYPE_INFO) 
    lcdDrawText(COL2, y, str, attr);
#if defined(CRSF_EXTENDED_TYPES)
  else
    editName(COL2, y, str, 10/* max len to fit screen */, currentEvent, attr);
#endif
}
static void paramStringLoad(Parameter * param, uint8_t * data, uint8_t offset) {
#if defined(CRSF_EXTENDED_TYPES)
  uint8_t len = strlen((char*)&data[offset]);
  // if (len) param->maxlen = data[offset + len + 1];
  char tmp[STRING_LEN_MAX] = {0};
  str2zchar(tmp, (char*)&data[offset], len);
  bufferPush(tmp, STRING_LEN_MAX);
#endif
}

static void paramStringSave(Parameter * param) {
#if defined(CRSF_EXTENDED_TYPES)
  char tmp[STRING_LEN_MAX + 1];
  zchar2str(tmp, (char*)&buffer[param->offset + param->nameLength], STRING_LEN_MAX);
  crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, param->id, (uint8_t *)&tmp, strlen(tmp) + 1);
#endif
}

static void paramMultibyteSave(Parameter * param) {
  uint8_t data[4];
  for (uint32_t i = 0; i < param->size; i++) {
      data[i] = (uint8_t)((param->value >> (8 * i)) & 0xFF);
    }
  crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, param->id, (uint8_t *)&data, param->size);
}

static void paramInfoLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  uint8_t len = strlen((char*)&data[offset]) + 1;
  bufferPush((char*)&data[offset], len); // info + \0
}

static bool getSelectedOption(char * option, char * options, const uint8_t value) {
  uint8_t index = 0;
  while (*options != '\0') {
    if (index == value) break;
    if (*options++ == ';') index++;
  }
  while (*options != ';' && *options != '\0') {
    *option++ = *options++;
  }
  *option = '\0';
  return (index == value);
}

static void paramTextSelectionDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  const uint32_t optionsOffset = param->offset + param->nameLength + 2 /* min, max */;
  const uint32_t optionsLen = strlen((char*)&buffer[optionsOffset]);
  char option[24];
  if (!getSelectedOption(option, (char*)&buffer[optionsOffset], param->value)) {
    option[0] = 'E';
    option[1] = 'R';
    option[2] = 'R';
    option[3] = '\0';
  }
  lcdDrawText(COL2, y, option, attr);
  unitDisplay(param, y, optionsOffset + optionsLen + 1);
}

static void paramFolderOpen(Parameter * param) {
  ui.lineIndex = 1;
  ui.pageOffset = 0;
  paramLoad.currentFolderId = param->id;
  reloadAllParam();
  if (param->type == TYPE_FOLDER) { // guard because it is reused for devices
    paramLoad.id = param->id + 1; // UX hack: start loading from first folder item to fetch it faster
  }
  clearData();
}

static void paramFolderDeviceOpen(Parameter * param) {
  paramLoad.expectedCount = devicesLen;
  devicesLen = 0;
  crossfireTelemetryPing(); // broadcast with standard handset ID to get all node respond correctly
  paramFolderOpen(param);
}

static void noopLoad(Parameter * param, uint8_t * data, uint8_t offset) {}
static void noopSave(Parameter * param) {}

static void paramCommandLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  param->status = data[offset];
  param->timeout = data[offset+1];
  param->infoOffset = offset+2; // do not copy info, access directly
  if (param->status == STATUS_READY) {
    ui.paramPopup = nullptr;
  }
}

static void paramCommandSave(Parameter * param) {
  if (param->status < STATUS_CONFIRM) {
    param->status = STATUS_START;
    crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, param->id, param->status);
    ui.paramPopup = param;
    ui.paramPopup->lastStatus = STATUS_READY;
    paramLoad.timeout = getTime() + param->timeout;
  }
}

static void paramUnifiedDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  uint8_t textIndent = COL1 + 9;
  char tmp[28];
  char * tmpString = tmp;
  if (param->type == TYPE_FOLDER) {
    *tmpString++ = '>';
    *tmpString++ = ' ';
    strAppend(tmpString, (char *)&buffer[param->offset], param->nameLength);
    textIndent = COL1;
  } else if (param->type == TYPE_DEVICES_FOLDER) {
    strAppend(tmpString, "> Other Devices");
    textIndent = COL1;
  } else { // CMD || DEVICE || BACK
    *tmpString++ = '[';
    if (param->type == TYPE_BACK) tmpString = strAppend(tmpString, "----BACK----");
    else tmpString = strAppend(tmpString, (char *)&buffer[param->offset], param->nameLength);
    *tmpString++ = ']';
    *tmpString = '\0';
  }
  lcdDrawText(textIndent, y, tmp, attr | BOLD);
}

static void paramBackExec(Parameter * param = 0) {
  paramLoad.currentFolderId = 0;
  reloadAllParam();
  devicesLen = 0;
  paramLoad.expectedCount = 0;
}

static void changeDeviceId(uint8_t devId) {
  //TRACE("changeDeviceId %x", devId);
  paramLoad.currentFolderId = 0;
  device.isELRS_TX = 0;
  //if the selected device ID (target) is a TX Module, we use our Lua ID, so TX Flag that user is using our LUA
  device.handsetId = (devId == 0xEE) ? 0xEF : 0xEA;
  device.id = devId;
  paramLoad.expectedCount = 0; //set this because next target wouldn't have the same count, and this trigger to request the new count
}

static void paramDeviceIdSelect(Parameter * param) {
//  TRACE("paramDeviceIdSelect %x", param->id);
  changeDeviceId(param->id);
  crossfireTelemetryPing();
}

static void parseDeviceInfoMessage(uint8_t* data) {
  uint8_t offset;
  uint8_t id = data[2];
// TRACE("parseDev:%x, exp:%d, devs:%d", id, expectedParamsCount, devicesLen);
  offset = strlen((char*)&data[3]) + 1 + 3;
  
  if (!isExistingDevice(id)) {
    deviceIds[devicesLen] = id;
    devicesLen++;
    
    if (paramLoad.currentFolderId == otherDevicesId) { // if "Other Devices" opened store devices to params
      Parameter deviceParam;
      deviceParam.id = id;
      deviceParam.type = TYPE_DEVICE;
      deviceParam.nameLength = offset - 4;
      deviceParam.offset = bufferOffset;
      bufferPush((char *)&data[3], deviceParam.nameLength);
      storeParam(&deviceParam);
      
      if (devicesLen == paramLoad.expectedCount) {
        paramLoad.allLoaded = 1;
      }
    }
  }

  if (device.id == id && paramLoad.currentFolderId != otherDevicesId) {
    memcpy(&device.name[0], (char *)&data[3], 20);
    device.isELRS_TX = ((paramGetValue(&data[offset], 4) == 0x454C5253) && (device.id == 0xEE)); // SerialNumber = 'E L R S' and ID is TX module
    uint8_t newParamCount = data[offset+12];
    
    reloadAllParam();
    if (newParamCount != paramLoad.expectedCount || newParamCount == 0) {
      paramLoad.expectedCount = newParamCount;
      clearData();
      if (newParamCount == 0) {
        paramLoad.allLoaded = 1;
      }
    }
  }
}

// Function table: types 0-8 (UINT8 through FLOAT) use index 0
static const ParamFunctions functions[] = {
  { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay },    // 0: UINT8/INT8/UINT16/INT16/FLOAT
  // { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay }, // INT8(1)
  // { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay }, // UINT16(2)
  // { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay }, // INT16(3)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay },
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay },
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay },
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay },
  // { .load=paramFloatLoad, .save=paramMultibyteSave, .display=paramIntegerDisplay }, // FLOAT(8)
  { .load=paramIntegerLoad, .save=paramMultibyteSave, .display=paramTextSelectionDisplay }, // 9: SELECT
  { .load=paramStringLoad, .save=paramStringSave, .display=paramStringDisplay },          // 10: STRING
  { .load=noopLoad, .save=paramFolderOpen, .display=paramUnifiedDisplay },                // 11: FOLDER
  { .load=paramInfoLoad, .save=noopSave, .display=paramStringDisplay },                  // 12: INFO
  { .load=paramCommandLoad, .save=paramCommandSave, .display=paramUnifiedDisplay },       // 13: COMMAND
  { .load=noopLoad, .save=paramBackExec, .display=paramUnifiedDisplay },                 // 14: BACK
  { .load=noopLoad, .save=paramDeviceIdSelect, .display=paramUnifiedDisplay },           // 15: DEVICE
  { .load=noopLoad, .save=paramFolderDeviceOpen, .display=paramUnifiedDisplay },         // 16: DEVICES_FOLDER
};

static ParamFunctions getFunctions(uint32_t type) {
  // Types 0-8 all use the integer function set
  if (type <= TYPE_FLOAT) return functions[0];
  // Types 9-16 map to functions[1-8]
  return functions[type - 8];
}

static void parseParameterInfoMessage(uint8_t* data, uint8_t length) {
  if (data[2] != device.id || data[3] != paramLoad.id) {
    paramDataLen = 0;
    paramLoad.chunk = 0;
    return;
  }
  
  if (paramDataLen == 0) {
    paramLoad.expectedChunks = -1;
  }

  Parameter tempParam = {0};
  Parameter* param = getParamById(paramLoad.id);
  if (param == nullptr) {
    param = &tempParam;
  }

  uint8_t chunksRemain = data[4];
  if (chunksRemain != paramLoad.expectedChunks && paramLoad.expectedChunks != -1) {
    return;
  }
  paramLoad.expectedChunks = chunksRemain - 1;

  if (paramDataLen == 0 && data[5] != paramLoad.currentFolderId) {
    if (paramLoad.id == paramLoad.expectedCount) {
      paramLoad.allLoaded = 1;
    }
    paramLoad.chunk = 0;
    paramLoad.id++;
    return;
  }

  memcpy(&paramData[paramDataLen], &data[5], length - 5);
  paramDataLen += length - 5;

  if (chunksRemain > 0) {
    paramLoad.chunk++;
  } else {
    paramLoad.chunk = 0;
    if (paramDataLen < 4) {
      paramDataLen = 0;
      return;
    }

    param->id = paramLoad.id;
    uint8_t parent = paramData[0];
    uint8_t type = paramData[1] & 0x7F;
    uint8_t hidden = paramData[1] & 0x80;

    if (param->nameLength != 0 && (paramLoad.currentFolderId != parent || param->type != type)) {
      paramDataLen = 0;
      return;
    }

    param->type = type;
    uint8_t nameLen = strlen((char*)&paramData[2]);

    if (parent != paramLoad.currentFolderId) {
      param->nameLength = 0;
    } else if (!hidden) {
      if (param->nameLength == 0) {
        param->nameLength = nameLen;
        param->offset = bufferOffset;
        bufferPush((char*)&paramData[2], param->nameLength);
      }
      getFunctions(param->type).load(param, paramData, 2 + nameLen + 1);
      storeParam(param);
    }

    if (ui.paramPopup == nullptr) {
      if (paramLoad.id == paramLoad.expectedCount) {
        paramLoad.allLoaded = 1;
      } else if (paramLoad.allLoaded == 0) {
        paramLoad.id++;
      }
      paramLoad.timeout = getTime() + 200;
    } else {
      paramLoad.timeout = getTime() + ui.paramPopup->timeout;
    }
    resetParamData();
  }
}

static void parseElrsInfoMessage(uint8_t* data) {
  if (data[2] != device.id) {
    paramDataLen = 0;
    paramLoad.chunk = 0;
    return;
  }

  linkstat.bad = data[3];
  linkstat.good = paramGetValue(&data[4], 2);
  uint8_t newFlags = data[6];
  // If flags are changing, reset the warning timeout to display/hide message immediately
  if (newFlags != linkstat.flags) {
    linkstat.flags = newFlags;
    display.titleShowWarnTimeout = 0;
  }
  strncpy(display.elrsInfo, (char*)&data[7], 20);
}

static void refreshNextCallback(uint8_t command, uint8_t* data, uint8_t length) {
  if (command == CRSF_FRAMETYPE_DEVICE_INFO) {
    parseDeviceInfoMessage(data);
  } else if (command == CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY && paramLoad.currentFolderId != otherDevicesId) {
    parseParameterInfoMessage(data, length);
    if (paramLoad.allLoaded < 1) {
      paramLoad.timeout = 0; // request next chunk immediately
    }
  } else if (command == CRSF_FRAMETYPE_ELRS_STATUS) {
    parseElrsInfoMessage(data);
  }

  if (ui.btnState == BTN_NONE && paramLoad.allLoaded) {
    if (paramLoad.currentFolderId == 0) {
      if (devicesLen > 1) addOtherDevicesButton();
    } else {
      addBackButton();
    }
  }
}

static void refreshNext() {
  tmr10ms_t time = getTime();
  
  if (ui.paramPopup != nullptr) {
    if (time > paramLoad.timeout && ui.paramPopup->status != STATUS_CONFIRMATION_NEEDED) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, ui.paramPopup->id, STATUS_POLL);
      paramLoad.timeout = time + ui.paramPopup->timeout;
    }
  } else if (time > display.devicesRefreshTimeout && paramLoad.expectedCount < 1) {
    display.devicesRefreshTimeout = time + 100; // 1s
    crossfireTelemetryPing();
  } else if (time > paramLoad.timeout && paramLoad.expectedCount != 0) {
    if (paramLoad.allLoaded < 1) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_READ, paramLoad.id, paramLoad.chunk);
      paramLoad.timeout = time + (device.isELRS_TX ? 50 : 500); // 0.5s for local / 5s for remote devices
    }
  }

  if (device.isELRS_TX && time > display.linkstatTimeout) {
    crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, 0x0, 0x0); // request linkstat
    display.linkstatTimeout = time + 100;
  }

  if (time > display.titleShowWarnTimeout) {
    display.titleShowWarn = (linkstat.flags > 3) ? !display.titleShowWarn : 0;
    display.titleShowWarnTimeout = time + 100;
  }
}

static void lcd_title() {
  lcdClear();

  if (device.isELRS_TX && !display.titleShowWarn) {
    char tmp[16];
    char * tmpString = tmp;
    tmpString = strAppendUnsigned(tmpString, linkstat.bad);
    strAppendStringWithIndex(tmpString, "/", linkstat.good);
    lcdDrawText(LCD_W - 11, 0, tmp, RIGHT);
    lcdDrawVerticalLine(LCD_W - 10, 0, BAR_HEIGHT, SOLID, INVERS);
    lcdDrawChar(LCD_W - 7, 0, (linkstat.flags & 1) ? 'C' : '-');
  }

  lcdInvertLine(0);
  if (paramLoad.allLoaded != 1 && paramLoad.expectedCount > 0) {
    luaLcdDrawGauge(0, 1, COL2, BAR_HEIGHT, paramLoad.id, paramLoad.expectedCount);
  } else {
    const char* textToDisplay = display.titleShowWarn ? display.elrsInfo :
                            (paramLoad.allLoaded == 1) ? device.name : TR_EXTERNALRF; // "External TX...";
    uint8_t textLen = display.titleShowWarn ? 20 : 20;
    lcdDrawSizedText(COL1, 0, textToDisplay, textLen, INVERS);
  }
}

static void lcd_warn() {
  showMessageBox(display.elrsInfo);
  lcdDrawText(LCD_W/2 - 2 * FW, WARNING_LINE_Y + 4*FH+2, TR_ENTER);
}

static void handleDevicePageEvent(event_t event) {
  if (allocatedParamsCount == 0) { // if there is no param yet
    return;
  } else {
    // Will stuck on main page because back button is not present
    // if (getParamById(backButtonId)/*->nameLength*/ == nullptr) { // if back button is not assigned yet, means there is no param yet.
    //   return;
    // }
  }

  Parameter * param = getParam(ui.lineIndex);

  if (event == EVT_VIRTUAL_EXIT) {
    if (s_editMode) {
      s_editMode = 0;
      paramLoad.timeout = getTime() + 200;
      paramLoad.id = param->id;
      paramLoad.chunk = 0;
      paramDataLen = 0;
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_READ, paramLoad.id, paramLoad.chunk);
    } else {
      if (paramLoad.currentFolderId == 0 && paramLoad.allLoaded == 1) {
        if (device.id != 0xEE) {
          changeDeviceId(0xEE); // change device id clear expectedParamsCount, therefore the next ping will do reloadAllParam()
        } else {
//          reloadAllParam(); // paramBackExec does it
        }
        crossfireTelemetryPing();
      }
      paramBackExec();
    }
  } else if (event == EVT_VIRTUAL_ENTER) {
    if (linkstat.flags > 0x1F) {
      linkstat.flags = 0;
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, 0x2E, 0x00);
    } else {
      if (param != 0 && param->nameLength > 0) {
        if (param->type < TYPE_FOLDER) {
          s_editMode = (s_editMode) ? 0 : 1;
        }
        if (!s_editMode) {
          if (param->type == TYPE_COMMAND) {
            // For commands, request this param's data again
            // Do this before save() to allow save to override
            paramLoad.id = param->id;
            paramLoad.chunk = 0;
            paramDataLen = 0;
          }
          // with a short delay to allow the module EEPROM to commit
          paramLoad.timeout = getTime() + 20;
          // Also push the next bad/good update further out
          display.linkstatTimeout = paramLoad.timeout + 100;
          getFunctions(param->type).save(param);
          if (param->type < TYPE_FOLDER) {
            // For editable param types
            // Reload all editable fields at the same level
            clearData();
            reloadAllParam();
            paramLoad.id = paramLoad.currentFolderId + 1; // Start loading from first folder item
          }
        }
      }
    }
  } else if (s_editMode) {
    if (param->type == TYPE_STRING) {
      return;
    }
//    if (event == EVT_VIRTUAL_NEXT) {
//      incrParam(1);
//    } else if (event == EVT_VIRTUAL_PREV) {
//      incrParam(-1);
//    }
    // smaller but missing step support (FLOAT)
    param->value = checkIncDec(event, param->value, paramGetMin(param), paramGetMax(param), 0);
  } else {
    if (event == EVT_VIRTUAL_NEXT) {
      selectParam(1);
    } else if (event == EVT_VIRTUAL_PREV) {
      selectParam(-1);
    }
  }
}

static void runDevicePage(event_t event) {
  currentEvent = event;
  handleDevicePageEvent(event);

  lcd_title();

  if (linkstat.flags > 0x1F) {
    lcd_warn();
  } else {
    Parameter * param;
    for (uint32_t y = 1; y < MAX_LINE_INDEX + 2; y++) {
      if (ui.pageOffset + y > allocatedParamsCount) break;
      param = getParam(ui.pageOffset + y);
      if (param == nullptr || param->nameLength == 0) {
        break;
      }
      uint8_t attr = (ui.lineIndex == (ui.pageOffset + y)) ? ((s_editMode && BLINK) + INVERS) : 0;
      if (param->type < TYPE_FOLDER || param->type == TYPE_INFO) { // if not folder, command, or back
        lcdDrawSizedText(COL1, y * TEXT_SIZE + TEXT_YOFFSET, (char *)&buffer[param->offset], param->nameLength, 0);
      }
      getFunctions(param->type).display(param, y * TEXT_SIZE + TEXT_YOFFSET, attr);
    }
  }
}

static uint8_t popupCompat(event_t event) {
  showMessageBox((char *)&paramData[ui.paramPopup->infoOffset]);
  lcdDrawText(WARNING_LINE_X, WARNING_LINE_Y+4*FH+2, STR_POPUPS_ENTER_EXIT);

  if (event == EVT_VIRTUAL_EXIT) {
    return RESULT_CANCEL;
  } else if (event == EVT_VIRTUAL_ENTER) {
    return RESULT_OK;
  }
  return RESULT_NONE;
}

static void runPopupPage(event_t event) {
  if (event == EVT_VIRTUAL_EXIT) {
    crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, ui.paramPopup->id, STATUS_CANCEL);
    paramLoad.timeout = getTime() + 200;
  }

  uint8_t result = RESULT_NONE;
  if (ui.paramPopup->status == STATUS_READY && ui.paramPopup->lastStatus != STATUS_READY) { // stopped
    reloadAllParam();
    ui.paramPopup = nullptr;
  } else if (ui.paramPopup->status == STATUS_CONFIRMATION_NEEDED) {
    result = popupCompat(event);
    ui.paramPopup->lastStatus = ui.paramPopup->status;
    if (result == RESULT_OK) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, ui.paramPopup->id, STATUS_CONFIRM);
      paramLoad.timeout = getTime() + ui.paramPopup->timeout; // we are expecting an immediate response
      ui.paramPopup->status = STATUS_CONFIRM;
    } else if (result == RESULT_CANCEL) {
      ui.paramPopup = nullptr;
    }
  } else if (ui.paramPopup->status == STATUS_PROGRESS) {
    result = popupCompat(event);
    ui.paramPopup->lastStatus = ui.paramPopup->status;
    if (result == RESULT_CANCEL) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, ui.paramPopup->id, STATUS_CANCEL);
      paramLoad.timeout = getTime() + ui.paramPopup->timeout;
      ui.paramPopup = nullptr;
    }
  }
}

void elrsStop() {
  registerCrossfireTelemetryCallback(nullptr);
  // reloadAllParam();
  paramBackExec();
  ui.paramPopup = nullptr;
  device.id = 0xEE;
  device.handsetId = 0xEF;
  globalData.cToolRunning = 0;
  clearData();
  popMenu();
}

void elrsRun(event_t event) {
  if (globalData.cToolRunning == 0) {
    globalData.cToolRunning = 1;
    registerCrossfireTelemetryCallback(refreshNextCallback);
  }

  if (event == EVT_KEY_LONG(KEY_EXIT)) {
    elrsStop();
  } else { 
    if (ui.paramPopup != nullptr) {
      runPopupPage(event);
    } else {
      runDevicePage(event);
    }
    refreshNext();
  }
}
