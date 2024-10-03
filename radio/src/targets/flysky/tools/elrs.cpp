/**
 * ExpressLRS V3 configurator for i6X based on elrsV2/3.lua
 * @author Jan Kozak (ajjjjjjjj)
 *
 */
#include "opentx.h"
#include "tiny_string.cpp"

enum COMMAND_STEP {
    STEP_IDLE = 0,
    STEP_CLICK = 1,       // user has clicked the command to execute
    STEP_EXECUTING = 2,   // command is executing
    STEP_CONFIRM = 3,     // command pending user OK
    STEP_CONFIRMED = 4,   // user has confirmed
    STEP_CANCEL = 5,      // user has requested cancel
    STEP_QUERY = 6,       // UI is requesting status update
};

#define TYPE_UINT8				   0
#define TYPE_INT8				   1
#define TYPE_UINT16				   2
#define TYPE_INT16				   3
#define TYPE_FLOAT				   8
#define TYPE_SELECT                9
#define TYPE_STRING				  10
#define TYPE_FOLDER				  11
#define TYPE_INFO				  12
#define TYPE_COMMAND			  13
#define TYPE_BACK                 14
#define TYPE_DEVICE               15
#define TYPE_DEVICES_FOLDER       16

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
PACK(struct Parameter {
  uint16_t offset;
  uint8_t nameLength;
  union {
    uint8_t min;          // INT8
    uint8_t timeout;      // COMMAND
    uint8_t valuesLength; // SELECT, min always 0
    uint8_t prec;         // FLOAT
  };
  union {
    uint8_t unitLength;
    uint8_t lastStatus;   // COMMAND
  };
  union {
    uint8_t value;
    uint8_t status;       // COMMAND, must be alias to value, because save expects it!
  };
  uint8_t type;
  uint8_t max;            // INT8, SELECT
  uint8_t id;
});

struct ParamFunctions {
  void (*load)(Parameter*, uint8_t *, uint8_t);
  void (*save)(Parameter*);
  void (*display)(Parameter*, uint8_t, uint8_t);
};

static constexpr uint16_t BUFFER_SIZE = 512;
static uint8_t *buffer = &reusableBuffer.cToolData[0];
static uint16_t bufferOffset = 0;

// last POPUP_MSG_MAX_LEN of PARAM_DATA_TAIL_SIZE are reused for popup message
static constexpr uint8_t PARAM_DATA_TAIL_SIZE = 40; // max popup packet size
static constexpr uint8_t POPUP_MSG_MAX_LEN = 24; // popup hard limit = 24
static constexpr uint8_t POPUP_MSG_OFFSET = PARAM_DATA_TAIL_SIZE - POPUP_MSG_MAX_LEN;

static uint8_t *paramData = &reusableBuffer.cToolData[BUFFER_SIZE];
static uint8_t paramDataLen = 0;

static constexpr uint8_t PARAMS_MAX_COUNT = 16;
static constexpr uint8_t PARAMS_SIZE = PARAMS_MAX_COUNT * sizeof(Parameter);
static Parameter *params = (Parameter *)&reusableBuffer.cToolData[BUFFER_SIZE + PARAM_DATA_TAIL_SIZE];
static uint8_t allocatedParamsCount = 0;

static constexpr uint8_t DEVICES_MAX_COUNT = 8;
static uint8_t *deviceIds = &reusableBuffer.cToolData[BUFFER_SIZE + PARAM_DATA_TAIL_SIZE + PARAMS_SIZE];
//static uint8_t deviceIds[DEVICES_MAX_COUNT];
static uint8_t devicesLen = 0;

static constexpr uint8_t backButtonId = 100;
static constexpr uint8_t otherDevicesId = 101;

#define BTN_NONE 0
#define BTN_REQUESTED 1
#define BTN_ADDED 2
static uint8_t otherDevicesState = BTN_NONE;

static uint8_t deviceId = 0xEE;
static uint8_t handsetId = 0xEF;

static constexpr uint8_t DEVICE_NAME_MAX_LEN = 20;
//static uint8_t *deviceName = &reusableBuffer.cToolData[BUFFER_SIZE + PARAM_DATA_TAIL_SIZE + PARAMS_SIZE + DEVICES_MAX_COUNT];
static char deviceName[DEVICE_NAME_MAX_LEN];
static uint8_t lineIndex = 1;
static uint8_t pageOffset = 0;
static uint8_t edit = 0;
static Parameter * paramPopup = nullptr;
static tmr10ms_t paramTimeout = 0;
static uint8_t paramId = 1;
static uint8_t paramChunk = 0;

static char goodBadPkt[11] = "";
static uint8_t elrsFlags = 0;
static constexpr uint8_t ELRS_FLAGS_INFO_MAX_LEN = 20;
//static char *elrsFlagsInfo = (char *)&reusableBuffer.cToolData[BUFFER_SIZE + PARAM_DATA_TAIL_SIZE + PARAMS_SIZE + DEVICES_MAX_COUNT + DEVICE_NAME_MAX_LEN];
static char elrsFlagsInfo[ELRS_FLAGS_INFO_MAX_LEN] = "";
static uint8_t expectedParamsCount = 0;

static tmr10ms_t devicesRefreshTimeout = 50;
static uint8_t allParamsLoaded = 0;
static uint8_t currentFolderId = 0; // folder id
static int8_t expectedChunks = -1;
static uint8_t deviceIsELRS_TX = 0;
static tmr10ms_t linkstatTimeout = 100;
static uint8_t titleShowWarn = 0;
static tmr10ms_t titleShowWarnTimeout = 100;

static constexpr uint8_t COL1          =  0;
static constexpr uint8_t COL2          = 70;
static constexpr uint8_t maxLineIndex  =  6;
static constexpr uint8_t textYoffset   =  3;
static constexpr uint8_t textSize      =  8;

#define getTime           get_tmr10ms
#define EVT_VIRTUAL_EXIT  EVT_KEY_BREAK(KEY_EXIT)
#define EVT_VIRTUAL_ENTER EVT_KEY_BREAK(KEY_ENTER)
#define EVT_VIRTUAL_NEXT  EVT_KEY_FIRST(KEY_DOWN)
#define EVT_VIRTUAL_PREV  EVT_KEY_FIRST(KEY_UP)

static constexpr uint8_t RESULT_NONE = 0;
static constexpr uint8_t RESULT_OK = 2;
static constexpr uint8_t RESULT_CANCEL = 1;

static void storeParam(Parameter * param);
static void clearParams();
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
  paramData = &reusableBuffer.cToolData[bufferOffset + 0 /* offset */];
  paramDataLen = 0;
}

static void crossfireTelemetryCmd(const uint8_t cmd, const uint8_t index, const uint8_t value) {
  // TRACE("crsf cmd %x %x %x", cmd, index, value);
  uint8_t crsfPushData[4] = { deviceId, handsetId, index, value };
  crossfireTelemetryPush(cmd, crsfPushData, sizeof(crsfPushData));
}

static void crossfireTelemetryPing(){
  const uint8_t crsfPushData[2] = { 0x00, 0xEA };
  crossfireTelemetryPush(CRSF_FRAMETYPE_DEVICE_PING, (uint8_t *) crsfPushData, 2);
}

static void clearParams() {
//  TRACE("clearParams %d", allocatedParamsCount);
  memclear(params, PARAMS_SIZE);
  otherDevicesState = BTN_NONE;
  allocatedParamsCount = 0;
}

// Both buttons must be added as last ones because i cannot overwrite existing Id
static void addBackButton() {
  Parameter backBtnParam;
  backBtnParam.id = backButtonId;
  backBtnParam.nameLength = 1; // mark as present
  backBtnParam.type = TYPE_BACK;
  storeParam(&backBtnParam);
}

static void addOtherDevicesButton() {
  Parameter otherDevicesParam;
  otherDevicesParam.id = otherDevicesId;
  otherDevicesParam.nameLength = 1;
  otherDevicesParam.type = TYPE_DEVICES_FOLDER;
  storeParam(&otherDevicesParam);
  otherDevicesState = BTN_ADDED;
}

static void reloadAllParam() {
//  TRACE("reloadAllParam");
  allParamsLoaded = 0;
  paramId = 1;
  paramChunk = 0;
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

static uint32_t paramGetValue(Parameter * param, uint16_t offset, uint8_t size) {
  uint32_t result = 0;
  for (uint32_t i = 0; i < size; i++) {
    result = (result << 8) + buffer[param->offset + param->nameLength + offset + i];
  }
  return result;
}

// static int32_t getMin(Parameter * param) {
//   uint8_t size = (param->type <= TYPE_INT16) ? 2 : 4;
//   return paramGetValue(param, 1 * size, size);
// }

// static int32_t getMax(Parameter * param) {
//   uint8_t size = (param->type <= TYPE_INT16) ? 2 : 4;
//   return paramGetValue(param, 2 * size, size);
// }

// static uint32_t getStep(Parameter * param) {
//   return paramGetValue(param, 3 * 4, 4);
// }

/**
 * Get param from line index taking only loaded current folder params into account.
 */
static Parameter * getParam(const uint8_t line) {
  return &params[line - 1];
}

static void incrParam(int32_t step) {
  Parameter * param = getParam(lineIndex);
  // int32_t value, min = 0, max = 0;
  if (param->type <= TYPE_INT8) {
    param->value = limit<int32_t>(param->min, param->value + step, param->max);
  } 
  // else if (param->type <= TYPE_INT16) {
  //   value = 0;
  //   min = getMin(param);
  //   max = getMax(param);
  // } else if (param->type == TYPE_FLOAT) {
  //   value = paramGetValue(param, 0, 4);
  //   min = getMin(param);
  //   max = getMax(param);
  //   step *= getStep(param);
  //   value = limit<int32_t>(min, value + step, max);
  //   // TODO save to buffer paramSetValue
  // } else if (param->type == TYPE_SELECT) {
  //   param->value = limit<int32_t>(0, param->value + step, param->max);
  // }
}

static void selectParam(int8_t step) {
  int32_t newLineIndex = lineIndex;
  Parameter * param;
  do {
    newLineIndex = newLineIndex + step;
    if (newLineIndex <= 0) {
      newLineIndex = allocatedParamsCount;
    } else if (newLineIndex == 1 + allocatedParamsCount) {
      newLineIndex = 1;
      pageOffset = 0;
    }
    param = getParam(newLineIndex);
  } while (newLineIndex != lineIndex && (param == 0 || param->nameLength == 0));
  lineIndex = newLineIndex;
  if (lineIndex > maxLineIndex + pageOffset) {
    pageOffset = lineIndex - maxLineIndex;
  } else if (lineIndex <= pageOffset) {
    pageOffset = lineIndex - 1;
  }
}

static uint8_t getDevice(uint8_t devId) {
//   TRACE("getDevice %x", devId);
  for (uint32_t i = 0; i < devicesLen; i++) {
    if (deviceIds[i] == devId) {
      return deviceIds[i];
    }
  }
  return 0;
}

static void unitLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  uint8_t unitLen = strlen((char*)&data[offset]);
  param->unitLength = unitLen;
  bufferPush((char*)&data[offset], unitLen);
}

static void unitDisplay(Parameter * param, uint8_t y, uint16_t offset) {
  lcdDrawSizedText(lcdLastRightPos, y, (char *)&buffer[offset], param->unitLength, 0);
}

static void paramIntegerDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  switch (param->type) {
    case TYPE_UINT8:
      lcdDrawNumber(COL2, y, (uint8_t)param->value, attr);
      break;
    case TYPE_INT8:
      lcdDrawNumber(COL2, y, (int8_t)param->value, attr);
      break;
    case TYPE_UINT16:
      lcdDrawNumber(COL2, y, (uint16_t)paramGetValue(param, 0, 2), attr);
      break;
    case TYPE_INT16:
      lcdDrawNumber(COL2, y, (int16_t)paramGetValue(param, 0, 2), attr);
      break;
  }
  unitDisplay(param, y, param->offset + param->nameLength + ((param->type >= TYPE_UINT16) ? 6 : 0));
}

static void paramInt8Load(Parameter * param, uint8_t * data, uint8_t offset) {
  param->value = data[offset + 0];
  param->min = data[offset + 1];
  param->max = data[offset + 2];
  unitLoad(param, data, offset + 4);
}

static void paramInt16Load(Parameter * param, uint8_t * data, uint8_t offset) {
  bufferPush((char *)&data[offset + 0], 2 + 2 + 2); // value + min + max at once
  unitLoad(param, data, offset + 8);
}

static void paramIntSave(Parameter * param) {
  crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, param->id, param->value);
}

static void paramFloatDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  char tmpString[12];
  strAppendSigned(tmpString, paramGetValue(param, 0, 4));
  if (param->prec > 0) { // insert dot
    uint8_t pos = strlen(tmpString) - param->prec;
    memmove(&tmpString[pos + 1], &tmpString[pos], param->prec + 1);
    tmpString[pos] = '.';
  }
  lcdDrawText(COL2, y, tmpString, attr);
  unitDisplay(param, y, param->offset + param->nameLength + 4 * 4);
}

static void paramFloatLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  bufferPush((char *)&data[offset + 0], 4 + 4 + 4); // value + min + max at once
  param->prec = data[offset + 12];
  bufferPush((char *)&data[offset + 13], 4); // step
  unitLoad(param, data, offset + 17);
}

// TEXT SELECTION
/**
 * Reused also for INFO params value (i.e. commit sha) for 0 flash cost
 */
static void paramTextSelectionLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  uint8_t len = strlen((char*)&data[offset]);
  param->value = data[offset + len + 1];
  param->max = data[offset + len + 3];
  len = strlen((char*)&data[offset]);
  if (param->valuesLength == 0) {
    bufferPush((char*)&data[offset], len);
    param->valuesLength = len;
  }
  unitLoad(param, data, offset + len + 5);
}

static uint8_t getNextItemPos(const char * str, uint8_t last) {
  uint8_t pos = 0;
  while ((str[pos] != ';') && (pos < last)) pos++;
  return pos + 1;
}

static void paramTextSelectionDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  const uint16_t valuesOffset = param->offset + param->nameLength;
  uint16_t start = valuesOffset;
  uint8_t len;
  uint32_t i = 0;
  while (i++ < param->value) {
    start += getNextItemPos((char *)&buffer[start], param->valuesLength - (start - valuesOffset));
    if (start - valuesOffset >= param->valuesLength) {
      lcdDrawText(COL2, y, "ERR", attr);
      return;
    }
  }
  len = getNextItemPos((char *)&buffer[start], param->valuesLength - (start - valuesOffset)) - 1;

  lcdDrawSizedText(COL2, y, (char *)&buffer[start], len, attr);
  unitDisplay(param, y, param->offset + param->nameLength + param->valuesLength);
}

static void paramStringDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  lcdDrawSizedText(COL2, y, (char *)&buffer[param->offset + param->nameLength], param->valuesLength, attr);
}

static void paramFolderOpen(Parameter * param) {
  //TRACE("paramFolderOpen %d", param->id);
  lineIndex = 1;
  pageOffset = 0;
  currentFolderId = param->id;
  reloadAllParam();
  if (param->type == TYPE_FOLDER) { // guard because it is reused for devices
    paramId = param->id + 1; // UX hack: start loading from first folder item to fetch it faster
  }
  clearParams();
}

static void paramFolderDeviceOpen(Parameter * param) {
  // if currentFolderId == devices folder, store only devices instead of params
  expectedParamsCount = devicesLen;
  devicesLen = 0;
  crossfireTelemetryPing(); //broadcast with standard handset ID to get all node respond correctly
  paramFolderOpen(param);
}

static void noopLoad(Parameter * param, uint8_t * data, uint8_t offset) {}
static void noopSave(Parameter * param) {}
static void noopDisplay(Parameter * param, uint8_t y, uint8_t attr) {}

static void paramCommandLoad(Parameter * param, uint8_t * data, uint8_t offset) {
  param->status = data[offset];
  param->timeout = data[offset+1];
  if (param->status == STEP_IDLE) {
    paramPopup = nullptr;
  } else {
    strncpy((char *)&paramData[POPUP_MSG_OFFSET], (char *)&data[offset+2], POPUP_MSG_MAX_LEN);
  }
}

static void paramCommandSave(Parameter * param) {
  if (param->status < STEP_CONFIRMED) {
    param->status = STEP_CLICK;
    paramIntSave(param); //crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, param->id, param->status);
    paramPopup = param;
    paramPopup->lastStatus = 0;
    paramTimeout = getTime() + param->timeout;
  }
}

static void paramUnifiedDisplay(Parameter * param, uint8_t y, uint8_t attr) {
  const char* backPat = "[----BACK----]";
  const char* folderPat = "> %s";
  const char* otherPat = "> Other Devices";
  const char* cmdPat = "[%s]";
  const char *pat;
  uint8_t textIndent = COL1 + 9;
  if (param->type == TYPE_FOLDER) {
    pat = folderPat;
    textIndent = COL1;
  } else if (param->type == TYPE_DEVICES_FOLDER) {
    pat = otherPat;
    textIndent = COL1;
  } else if (param->type == TYPE_BACK) {
    pat = backPat;
  } else { // CMD || DEVICE
    pat = cmdPat;
  }
  char tmpString[28];
  tiny_sprintf((char *)&tmpString, pat, 2, param->nameLength, (char *)&buffer[param->offset]);
  lcdDrawText(textIndent, y, (char *)&tmpString, attr | BOLD);
}

static void paramBackExec(Parameter * param = 0) {
  currentFolderId = 0;
  clearParams();
  reloadAllParam();
  devicesLen = 0;
  expectedParamsCount = 0;
}

static void changeDeviceId(uint8_t devId) {
  //TRACE("changeDeviceId %x", devId);
  currentFolderId = 0;
  deviceIsELRS_TX = 0;
  elrsFlags = 0;
  //if the selected device ID (target) is a TX Module, we use our Lua ID, so TX Flag that user is using our LUA
  if (devId == 0xEE) {
    handsetId = 0xEF;
  } else { //else we would act like the legacy lua
    handsetId = 0xEA;
  }
  deviceId = devId;
  expectedParamsCount = 0; //set this because next target wouldn't have the same count, and this trigger to request the new count
}

static void paramDeviceIdSelect(Parameter * param) {
//  TRACE("paramDeviceIdSelect %x", param->id);
 changeDeviceId(param->id);
 crossfireTelemetryPing();
}

static void parseDeviceInfoMessage(uint8_t* data) {
  uint8_t offset;
  uint8_t id = data[2];
// TRACE("parseDev:%x folder:%d, expect:%d, devs:%d", id, currentFolderId, expectedParamsCount, devicesLen);
  offset = strlen((char*)&data[3]) + 1 + 3;
  uint8_t devId = getDevice(id);
  if (!devId) {
    deviceIds[devicesLen] = id;
    devicesLen++;
    if (currentFolderId == otherDevicesId) { // if "Other Devices" opened store devices to params
      Parameter deviceParam;
      deviceParam.id = id;
      deviceParam.type = TYPE_DEVICE;
      deviceParam.nameLength = offset - 4;
      deviceParam.offset = bufferOffset;

      bufferPush((char *)&data[3], deviceParam.nameLength);
      storeParam(&deviceParam);
      if (devicesLen == expectedParamsCount) { // was it the last one?
        allParamsLoaded = 1;
        paramId = 1;
        addBackButton();
      }
    }
  }

  if (deviceId == id && currentFolderId != otherDevicesId) {
    memcpy(&deviceName[0], (char *)&data[3], DEVICE_NAME_MAX_LEN);
    deviceIsELRS_TX = ((memcmp(&data[offset], "ELRS", 4) == 0) && (deviceId == 0xEE)) ? 1 : 0; // SerialNumber = 'E L R S' and ID is TX module
    uint8_t newParamCount = data[offset+12];
//    TRACE("deviceId match %x, newParamCount %d", deviceId, newParamCount);
    reloadAllParam();
    if (newParamCount != expectedParamsCount || newParamCount == 0) {
      expectedParamsCount = newParamCount;
      clearParams();
      if (newParamCount == 0) {
      // This device has no params so the Loading code never starts
        allParamsLoaded = 1;
        paramId = 1;
        addBackButton();
      }
    }
  }
}

static const ParamFunctions noopFunctions = { .load=noopLoad, .save=noopSave, .display=noopDisplay };

static const ParamFunctions functions[] = {
  { .load=paramInt8Load, .save=paramIntSave, .display=paramIntegerDisplay }, // 1 UINT8(0)
  { .load=paramInt8Load, .save=paramIntSave, .display=paramIntegerDisplay }, // 2 INT8(1)
  { .load=paramInt16Load, .save=noopSave, .display=paramIntegerDisplay }, // 3 UINT16(2)
  { .load=paramInt16Load, .save=noopSave, .display=paramIntegerDisplay }, // 4  INT16(3)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 5
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 6
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 7
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 8
  { .load=paramFloatLoad, .save=noopSave, .display=paramFloatDisplay }, // 9 FLOAT(8)
  { .load=paramTextSelectionLoad, .save=paramIntSave, .display=paramTextSelectionDisplay }, // 10 TEXT SELECTION(9)
  { .load=noopLoad, .save=noopSave, .display=paramStringDisplay }, // 11 STRING(10) editing
  { .load=noopLoad, .save=paramFolderOpen, .display=paramUnifiedDisplay }, // 12 FOLDER(11)
  { .load=paramTextSelectionLoad, .save=noopSave, .display=paramStringDisplay }, // 13 INFO(12)
  { .load=paramCommandLoad, .save=paramCommandSave, .display=paramUnifiedDisplay }, // 14 COMMAND(13)
  { .load=noopLoad, .save=paramBackExec, .display=paramUnifiedDisplay }, // 15 back(14)
  { .load=noopLoad, .save=paramDeviceIdSelect, .display=paramUnifiedDisplay }, // 16 device(15)
  { .load=noopLoad, .save=paramFolderDeviceOpen, .display=paramUnifiedDisplay }, // 17 deviceFOLDER(16)
};

static ParamFunctions getFunctions(uint32_t i) {
  if (i > TYPE_INT16) {
    if (i < TYPE_FLOAT) return noopFunctions; // guard against not implemented types
    i -= 4;
  }
  return functions[i];
}

static void parseParameterInfoMessage(uint8_t* data, uint8_t length) {
  // TRACE("parse %d...", data[3]);
  // DUMP(&data[4], length - 4);
  if (data[2] != deviceId || data[3] != paramId) {
    paramDataLen = 0;
    paramChunk = 0;
    return;
  }
  if (paramDataLen == 0) {
    expectedChunks = -1;
  }

  // Get by id or use temporary one to decide later if it should be stored
  Parameter tempParam = {0};
  Parameter* param = getParamById(paramId);
  if (param == nullptr) {
    param = &tempParam;
  }

  uint8_t chunksRemain = data[4];
  // If no param or the chunksRemain changed when we have data, don't continue
  if (/*param == 0 ||*/ (chunksRemain != expectedChunks && expectedChunks != -1)) {
    return;
  }
  expectedChunks = chunksRemain - 1;

  // skip on first chunk of not current folder
  if (paramDataLen == 0 && data[5] != currentFolderId) {
    if (paramId == expectedParamsCount) {
      allParamsLoaded = 1;
      paramId = 1;
      if (currentFolderId == 0) {
        otherDevicesState = BTN_REQUESTED;
      } else {
        addBackButton();
      }
    }
    paramChunk = 0;
    paramId++;
    return;
  }

  memcpy(&paramData[paramDataLen], &data[5], length - 5);
  paramDataLen += length - 5;

  if (chunksRemain > 0) {
    paramChunk = paramChunk + 1;
  } else {
    paramChunk = 0;
    if (paramDataLen < 4) {
      paramDataLen = 0;
      return;
    }

    param->id = paramId;
    uint8_t parent = paramData[0];
    uint8_t type = paramData[1] & 0x7F;
    uint8_t hidden = paramData[1] & 0x80;

    if (param->nameLength != 0) {
      if (currentFolderId != parent || param->type != type/* || param->hidden != hidden*/) {
        paramDataLen = 0;
        return;
      }
    }

    param->type = type;
    uint8_t nameLen = strlen((char*)&paramData[2]);

    if (parent != currentFolderId) {
      param->nameLength = 0; // mark as clear
    } else if (!hidden) {
      if (param->nameLength == 0) {
        param->nameLength = nameLen;
        param->offset = bufferOffset;
        bufferPush((char*)&paramData[2], param->nameLength);
      }
      getFunctions(param->type).load(param, paramData, 2 + nameLen + 1);
      storeParam(param);
    }

    if (paramPopup == nullptr) {
      if (paramId == expectedParamsCount) { // if we have loaded all params
        allParamsLoaded = 1;
        paramId = 1;
        if (currentFolderId == 0) {
          otherDevicesState = BTN_REQUESTED;
        } else {
          addBackButton();
        }
      } else if (allParamsLoaded == 0) {
        paramId++; // paramId = 1 + (paramId % (paramsLen-1));
      }
      paramTimeout = getTime() + 200;
    } else {
      paramTimeout = getTime() + paramPopup->timeout;
    }
    resetParamData();
  }
}

static void parseElrsInfoMessage(uint8_t* data) {
  if (data[2] != deviceId) {
    paramDataLen = 0;
    paramChunk = 0;
    return;
  }

  uint8_t badPkt = data[3];
  uint16_t goodPkt = (data[4] << 8) + data[5];
  uint8_t newFlags = data[6];
  // If flags are changing, reset the warning timeout to display/hide message immediately
  if (newFlags != elrsFlags) {
    elrsFlags = newFlags;
    titleShowWarnTimeout = 0;
  }
  strncpy(elrsFlagsInfo, (char*)&data[7], ELRS_FLAGS_INFO_MAX_LEN);

  char state = (elrsFlags & 1) ? 'C' : '-';
  tiny_sprintf(goodBadPkt, "%d/%d   %c", 3, badPkt, goodPkt, state);
}

static void refreshNextCallback(uint8_t command, uint8_t* data, uint8_t length) {
  if (command == CRSF_FRAMETYPE_DEVICE_INFO) {
    parseDeviceInfoMessage(data);
  } else if (command == CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY && currentFolderId != otherDevicesId /* !devicesFolderOpened */) {
    parseParameterInfoMessage(data, length);
    if (allParamsLoaded < 1) {
      paramTimeout = 0; // request next chunk immediately
    }
  } else if (command == CRSF_FRAMETYPE_ELRS_STATUS) {
    parseElrsInfoMessage(data);
  }
}

static void refreshNext() {
  tmr10ms_t time = getTime();
  if (paramPopup != nullptr) {
    if (time > paramTimeout && paramPopup->status != STEP_CONFIRM) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, paramPopup->id, 6); // lcsQuery
      paramTimeout = time + paramPopup->timeout; // + popup timeout
    }
  } else if (time > devicesRefreshTimeout && expectedParamsCount < 1) {
    devicesRefreshTimeout = time + 100;
    crossfireTelemetryPing();
  } else if (time > paramTimeout && expectedParamsCount != 0) {
    if (allParamsLoaded < 1) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_READ, paramId, paramChunk);
      paramTimeout = time + 50; // 0.5s
    }
  }

  if (time > linkstatTimeout) {
    if (!deviceIsELRS_TX && allParamsLoaded == 1) {
      goodBadPkt[0] = '\0';
    } else {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, 0x0, 0x0); // request linkstat
    }
    linkstatTimeout = time + 100;
  }
  if (time > titleShowWarnTimeout) {
    titleShowWarn = (elrsFlags > 3) ? !titleShowWarn : 0;
    titleShowWarnTimeout = time + 100;
  }
}

static void lcd_title() {
  lcdClear();

  const uint8_t barHeight = 9;
  if (!titleShowWarn) {
    lcdDrawText(LCD_W - 1, 1, goodBadPkt, RIGHT);
    lcdDrawVerticalLine(LCD_W - 10, 0, barHeight, SOLID, INVERS);
  }

  lcdDrawFilledRect(0, 0, LCD_W, barHeight, SOLID);
  if (allParamsLoaded != 1 && expectedParamsCount > 0) {
    luaLcdDrawGauge(0, 1, COL2, barHeight, paramId, expectedParamsCount);
  } else {
    if (titleShowWarn) {
      lcdDrawSizedText(COL1, 1, elrsFlagsInfo, ELRS_FLAGS_INFO_MAX_LEN, INVERS);
    } else {
      lcdDrawSizedText(COL1, 1, (allParamsLoaded == 1) ? (char *)&deviceName[0] : "Loading...", DEVICE_NAME_MAX_LEN, INVERS);
    }
  }
}

static void lcd_warn() {
  lcdDrawText(COL1, textSize*2, "Error:");
  lcdDrawText(COL1, textSize*3, elrsFlagsInfo);
  lcdDrawText(LCD_W/2, textSize*5, TR_ENTER, BLINK + INVERS + CENTERED);
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

  if (event == EVT_VIRTUAL_EXIT) {
    if (edit) {
      edit = 0;
      Parameter * param = getParam(lineIndex);
      paramTimeout = getTime() + 200;
      paramId = param->id;
      paramChunk = 0;
      paramDataLen = 0;
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_READ, paramId, paramChunk);
    } else {
      if (currentFolderId == 0 && allParamsLoaded == 1) {
        if (deviceId != 0xEE) {
          changeDeviceId(0xEE); // change device id clear expectedParamsCount, therefore the next ping will do reloadAllParam()
        } else {
//          reloadAllParam(); // paramBackExec does it
        }
        crossfireTelemetryPing();
      }
      paramBackExec();
    }
  } else if (event == EVT_VIRTUAL_ENTER) {
    if (elrsFlags > 0x1F) {
      elrsFlags = 0;
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, 0x2E, 0x00);
    } else {
      Parameter * param = getParam(lineIndex);
      if (param != 0 && param->nameLength > 0) {
        if (param->type == TYPE_STRING) {
          ; // not implemented
        } else if (param->type < TYPE_FOLDER) {
          edit = 1 - edit;
        }
        if (!edit) {
          if (param->type == TYPE_COMMAND) {
            // For commands, request this param's
            // data again, with a short delay to allow the module EEPROM to
            // commit. Do this before save() to allow save to override
            paramId = param->id;
            paramChunk = 0;
            paramDataLen = 0;
          }
          paramTimeout = getTime() + 20;
          getFunctions(param->type).save(param);
          if (param->type < TYPE_FOLDER) {
            // For editable param types reload whole folder, but do it after save
            clearParams();
            reloadAllParam();
            paramId = currentFolderId + 1; // Start loading from first folder item
          }
        }
      }
    }
  } else if (edit) {
    if (event == EVT_VIRTUAL_NEXT) {
      incrParam(1);
    } else if (event == EVT_VIRTUAL_PREV) {
      incrParam(-1);
    }
  } else {
    if (event == EVT_VIRTUAL_NEXT) {
      selectParam(1);
    } else if (event == EVT_VIRTUAL_PREV) {
      selectParam(-1);
    }
  }
}

static void runDevicePage(event_t event) {
  handleDevicePageEvent(event);

  lcd_title();

  if (devicesLen > 1 && otherDevicesState == BTN_REQUESTED) {
    addOtherDevicesButton();
  }
  if (elrsFlags > 0x1F) {
    lcd_warn();
  } else {
    Parameter * param;
    for (uint32_t y = 1; y < maxLineIndex + 2; y++) {
      if (pageOffset + y > allocatedParamsCount) break;
      param = getParam(pageOffset + y);
      if (param == nullptr) {
        break;
      } else if (param->nameLength > 0) {
        uint8_t attr = (lineIndex == (pageOffset+y)) ? ((edit && BLINK) + INVERS) : 0;
        if (param->type < TYPE_FOLDER || param->type == TYPE_INFO) {
          lcdDrawSizedText(COL1, y * textSize+textYoffset, (char *)&buffer[param->offset], param->nameLength, 0);
        }
        getFunctions(param->type).display(param, y*textSize+textYoffset, attr);
      }
    }
  }
}

static uint8_t popupCompat(event_t event) {
  showMessageBox((char *)&paramData[POPUP_MSG_OFFSET]);
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
    crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, paramPopup->id, STEP_CANCEL);
    paramTimeout = getTime() + 200;
  }

  uint8_t result = RESULT_NONE;
  if (paramPopup->status == STEP_IDLE && paramPopup->lastStatus != STEP_IDLE) { // stopped
      popupCompat(event);
      reloadAllParam();
      paramPopup = nullptr;
  } else if (paramPopup->status == STEP_CONFIRM) { // confirmation required
    result = popupCompat(event);
    paramPopup->lastStatus = paramPopup->status;
    if (result == RESULT_OK) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, paramPopup->id, STEP_CONFIRMED); // lcsConfirmed
      paramTimeout = getTime() + paramPopup->timeout; // we are expecting an immediate response
      paramPopup->status = STEP_CONFIRMED;
    } else if (result == RESULT_CANCEL) {
      paramPopup = nullptr;
    }
  } else if (paramPopup->status == STEP_EXECUTING) { // running
    result = popupCompat(event);
    paramPopup->lastStatus = paramPopup->status;
    if (result == RESULT_CANCEL) {
      crossfireTelemetryCmd(CRSF_FRAMETYPE_PARAMETER_WRITE, paramPopup->id, STEP_CANCEL);
      paramTimeout = getTime() + paramPopup->timeout;
      paramPopup = nullptr;
    }
  }
}

void elrsStop() {
  registerCrossfireTelemetryCallback(nullptr);
  // reloadAllParam();
  paramBackExec();
  paramPopup = nullptr;
  deviceId = 0xEE;
  handsetId = 0xEF;

  globalData.cToolRunning = 0;
  memset(reusableBuffer.cToolData, 0, sizeof(reusableBuffer.cToolData));
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
    if (paramPopup != nullptr) {
      runPopupPage(event);
    } else {
      runDevicePage(event);
    }

    refreshNext();
  }
}
