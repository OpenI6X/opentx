/**
 * ExpressLRS V3 configurator for i6X based on elrsV2/3.lua
 * @author Jan Kozak (ajjjjjjjj)
 *
 * Limitations vs elrsV3.lua:
 * - no int16, float, string fields support, but not used by ExpressLRS anyway,
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
#define TYPE_INT8				     1
#define TYPE_UINT16				   2
#define TYPE_INT16				   3
#define TYPE_FLOAT				   8
#define TYPE_TEXT_SELECTION  9
#define TYPE_STRING				  10
#define TYPE_FOLDER				  11
#define TYPE_INFO				    12
#define TYPE_COMMAND			  13
#define TYPE_BACK           14
#define TYPE_DEVICE         15
#define TYPE_DEVICES_FOLDER 16

#define CRSF_FRAMETYPE_DEVICE_PING 0x28
#define CRSF_FRAMETYPE_DEVICE_INFO 0x29
#define CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY 0x2B
#define CRSF_FRAMETYPE_PARAMETER_READ 0x2C
#define CRSF_FRAMETYPE_PARAMETER_WRITE 0x2D
#define CRSF_FRAMETYPE_ELRS_STATUS 0x2E

PACK(struct FieldProps {
  uint16_t offset;
  uint8_t nameLength;
  union {
    uint8_t min;
    uint8_t timeout;
  };
  union {
    uint8_t valuesLength;
    uint8_t lastStatus;
  };
  uint8_t max;
  uint8_t unitLength;
  uint8_t type;
  union {
    uint8_t value;
    uint8_t status;
  };
  uint8_t id;
});

struct FieldFunctions {
  void (*load)(FieldProps*, uint8_t *, uint8_t);
  void (*save)(FieldProps*);
  void (*display)(FieldProps*, uint8_t, uint8_t);
};

static constexpr uint16_t BUFFER_SIZE = 462;
static uint8_t *buffer = &reusableBuffer.cToolData[0];
static uint16_t bufferOffset = 0;

// last 25b are also used for popup messages
static constexpr uint8_t FIELD_DATA_BUFFER_SIZE = 176; // 8 + 56 + 56 + 56 
static uint8_t *fieldData = &reusableBuffer.cToolData[BUFFER_SIZE];

// Reuse tail of fieldData for popup messages
static constexpr uint8_t POPUP_MSG_MAX_LEN = 24; // popup hard limit is 32
static constexpr uint8_t POPUP_MSG_OFFSET = FIELD_DATA_BUFFER_SIZE - POPUP_MSG_MAX_LEN;
static uint8_t fieldDataLen = 0;

static constexpr uint8_t FIELDS_MAX_COUNT = 14;
static constexpr uint8_t FIELDS_SIZE = FIELDS_MAX_COUNT * sizeof(FieldProps);
static FieldProps *fields = (FieldProps *)&reusableBuffer.cToolData[BUFFER_SIZE + FIELD_DATA_BUFFER_SIZE];
static uint8_t allocatedFieldsCount = 0;

static constexpr uint8_t DEVICES_MAX_COUNT = 4;
static uint8_t *deviceIds = &reusableBuffer.cToolData[BUFFER_SIZE + FIELD_DATA_BUFFER_SIZE + FIELDS_SIZE];
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
//static uint8_t *deviceName = &reusableBuffer.cToolData[BUFFER_SIZE + FIELD_DATA_BUFFER_SIZE + FIELDS_SIZE + DEVICES_MAX_COUNT];
static char deviceName[DEVICE_NAME_MAX_LEN];
static uint8_t lineIndex = 1;
static uint8_t pageOffset = 0;
static uint8_t edit = 0;
static FieldProps * fieldPopup = nullptr;
static tmr10ms_t fieldTimeout = 0;
static uint8_t fieldId = 1;
static uint8_t fieldChunk = 0;

static char goodBadPkt[11] = "";
static uint8_t elrsFlags = 0;
static constexpr uint8_t ELRS_FLAGS_INFO_MAX_LEN = 20;
//static char *elrsFlagsInfo = (char *)&reusableBuffer.cToolData[BUFFER_SIZE + FIELD_DATA_BUFFER_SIZE + FIELDS_SIZE + DEVICES_MAX_COUNT + DEVICE_NAME_MAX_LEN];
static char elrsFlagsInfo[ELRS_FLAGS_INFO_MAX_LEN] = "";
static uint8_t expectedFieldsCount = 0;

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

static void storeField(FieldProps * field);
static void clearFields();
static void addBackButton();
static void reloadAllField();
static FieldProps * getField(uint8_t line);
static void fieldBackExec(FieldProps * field);
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

static void crossfireTelemetryPush4(const uint8_t cmd, const uint8_t third, const uint8_t fourth) {
//  TRACE("crsf push %x  %x  %x", cmd, third, fourth);
  uint8_t crsfPushData[4] = { deviceId, handsetId, third, fourth };
  crossfireTelemetryPush(cmd, crsfPushData, 4);
}

static void crossfireTelemetryPing(){
  const uint8_t crsfPushData[2] = { 0x00, 0xEA };
  crossfireTelemetryPush(CRSF_FRAMETYPE_DEVICE_PING, (uint8_t *) crsfPushData, 2);
}

static void clearFields() {
//  TRACE("clearFields %d", allocatedFieldsCount);
  memclear(fields, FIELDS_SIZE);
  otherDevicesState = BTN_NONE;
  allocatedFieldsCount = 0;
}

// Both buttons must be added as last ones because i cannot overwrite existing Id
static void addBackButton() {
  FieldProps backBtnField;
  backBtnField.id = backButtonId;
  backBtnField.nameLength = 1; // mark as present
  backBtnField.type = TYPE_BACK;
  storeField(&backBtnField);
}

static void addOtherDevicesButton() {
  FieldProps otherDevicesField;
  otherDevicesField.id = otherDevicesId;
  otherDevicesField.nameLength = 1;
  otherDevicesField.type = TYPE_DEVICES_FOLDER;
  storeField(&otherDevicesField);
  otherDevicesState = BTN_ADDED;
}

static void reloadAllField() {
//  TRACE("reloadAllField");
  allParamsLoaded = 0;
  fieldId = 1;
  fieldChunk = 0;
  fieldDataLen = 0;
  bufferOffset = 0;
}

static FieldProps * getFieldById(const uint8_t id) {
  for (uint32_t i = 0; i < allocatedFieldsCount; i++) {
    FieldProps * field = &fields[i];
    if (id == field->id) {
      return field;
    }
  }
  return nullptr;
}

/**
 * Store field at its location or add new one if not found.
 */
static void storeField(FieldProps * field) {
//   TRACE("storeField id %d", field->id);
  FieldProps * storedField = getFieldById(field->id);
  if (storedField == nullptr) {
    storedField = &fields[allocatedFieldsCount];
    allocatedFieldsCount++;
//    TRACE("allocFieldsCount %d", allocatedFieldsCount);
  }
  memcpy(storedField, field, sizeof(FieldProps));
}

/**
 * Get field from line index taking only loaded current folder fields into account.
 */
static FieldProps * getField(const uint8_t line) {
  return &fields[line - 1];
}

static void incrField(int8_t step) {
  FieldProps * field = getField(lineIndex);
  int32_t min = 0, max = 0;
  if (field->type <= TYPE_INT16) {
    min = field->min;
    max = field->max;
  } else if (field->type == TYPE_TEXT_SELECTION) {
//    min = 0;
    max = field->max;
  }
  field->value = limit<int32_t>(min, field->value + step, max);
}

static void selectField(int8_t step) {
  int32_t newLineIndex = lineIndex;
  FieldProps * field;
  do {
    newLineIndex = newLineIndex + step;
    if (newLineIndex <= 0) {
      newLineIndex = allocatedFieldsCount;
    } else if (newLineIndex == 1 + allocatedFieldsCount) {
      newLineIndex = 1;
      pageOffset = 0;
    }
    field = getField(newLineIndex);
  } while (newLineIndex != lineIndex && (field == 0 || field->nameLength == 0));
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

static void unitLoad(FieldProps * field, uint8_t * data, uint8_t offset) {
  uint8_t unitLen = strlen((char*)&data[offset]);
  //if (unitLen > 10) unitLen = 0; // Workaround for "Output Mode" missing last 2 bytes, proper solution would be to never read over packet length
  field->unitLength = unitLen;
  if (field->type < TYPE_STRING && unitLen > 0) {
    bufferPush((char*)&data[offset], unitLen);
  }
}

// UINT8
static void fieldIntegerDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  lcdDrawNumber(COL2, y, field->value, attr);
  lcdDrawSizedText(lcdLastRightPos, y, (char *)&buffer[field->offset + field->nameLength + field->valuesLength], field->unitLength, attr);
}

static void fieldUint8Load(FieldProps * field, uint8_t * data, uint8_t offset) {
  field->value = data[offset + 0];
  field->min = data[offset + 1];
  field->max = data[offset + 2];
  unitLoad(field, data, offset + 4);
}

static void fieldIntSave(FieldProps * field) {
  crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_WRITE, field->id, field->value);
}

// TEXT SELECTION
/**
 * Reused also for INFO fields value (i.e. commit sha) for 0 flash cost
 */
static void fieldTextSelectionLoad(FieldProps * field, uint8_t * data, uint8_t offset) {
  uint8_t len = strlen((char*)&data[offset]);
  field->value = data[offset + len + 1];
  field->max = data[offset + len + 3];
  len = strlen((char*)&data[offset]);
  if (field->valuesLength == 0) {
    bufferPush((char*)&data[offset], len);
    field->valuesLength = len;
  }
  unitLoad(field, data, offset + len + 5);
}

static uint8_t semicolonPos(const char * str, uint8_t last) {
  uint8_t pos = 0;
  while ((str[pos] != ';') && (pos < last)) pos++;
  return pos + 1;
}

static void fieldTextSelectionDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  const uint16_t valuesOffset = field->offset + field->nameLength;
  uint16_t start = valuesOffset;
  uint8_t len;
  uint32_t i = 0;
  while (i++ < field->value) {
    start += semicolonPos((char *)&buffer[start], field->valuesLength - (start - valuesOffset));
    if (start - valuesOffset >= field->valuesLength) {
      lcdDrawText(COL2, y, "ERR", attr);
      return;
    }
  }
  len = semicolonPos((char *)&buffer[start], field->valuesLength - (start - valuesOffset)) - 1;

  lcdDrawSizedText(COL2, y, (char *)&buffer[start], len, attr);
  lcdDrawSizedText(lcdLastRightPos, y, (char *)&buffer[field->offset + field->nameLength + field->valuesLength], field->unitLength, 0/*attr*/);
}

static void fieldStringDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  lcdDrawSizedText(COL2, y, (char *)&buffer[field->offset + field->nameLength], field->valuesLength, attr);
}

static void fieldFolderOpen(FieldProps * field) {
  //TRACE("fieldFolderOpen %d", field->id);
  lineIndex = 1;
  pageOffset = 0;
  currentFolderId = field->id;
  reloadAllField();
  if (field->type == TYPE_FOLDER) { // guard because it is reused for devices
    fieldId = field->id + 1; // UX hack: start loading from first folder item to fetch it faster
  }
  clearFields();
}

static void fieldFolderDeviceOpen(FieldProps * field) {
  // if currentFolderId == devices folder, store only devices instead of fields
  expectedFieldsCount = devicesLen;
  devicesLen = 0;
  crossfireTelemetryPing(); //broadcast with standard handset ID to get all node respond correctly
  fieldFolderOpen(field);
}

static void noopLoad(FieldProps * field, uint8_t * data, uint8_t offset) {}
static void noopSave(FieldProps * field) {}
static void noopDisplay(FieldProps * field, uint8_t y, uint8_t attr) {}

static void fieldCommandLoad(FieldProps * field, uint8_t * data, uint8_t offset) {
  field->status = data[offset];
  field->timeout = data[offset+1];
  strncpy((char *)&fieldData[POPUP_MSG_OFFSET], (char *)&data[offset+2], POPUP_MSG_MAX_LEN);
  if (field->status == STEP_IDLE) {
    fieldPopup = nullptr;
  }
}

static void fieldCommandSave(FieldProps * field) {
  if (field->status < 4) {
    field->status = STEP_CLICK;
    fieldIntSave(field); //crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_WRITE, field->id, field->status);
    fieldPopup = field;
    fieldPopup->lastStatus = 0;
    fieldTimeout = getTime() + field->timeout;
  }
}

static void fieldUnifiedDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  const char* backPat = "[----BACK----]";
  const char* folderPat = "> %s";
  const char* otherPat = "> Other Devices";
  const char* cmdPat = "[%s]";
  const char *pat;
  uint8_t textIndent = COL1 + 9;
  if (field->type == TYPE_FOLDER) {
    pat = folderPat;
    textIndent = COL1;
  } else if (field->type == TYPE_DEVICES_FOLDER) {
    pat = otherPat;
    textIndent = COL1;
  } else if (field->type == TYPE_BACK) {
    pat = backPat;
  } else { // CMD || DEVICE
    pat = cmdPat;
  }
  char stringTmp[28];
  tiny_sprintf((char *)&stringTmp, pat, 2, field->nameLength, (char *)&buffer[field->offset]);
  lcdDrawText(textIndent, y, (char *)&stringTmp, attr | BOLD);
}

static void fieldBackExec(FieldProps * field = 0) {
  currentFolderId = 0;
  clearFields();
  reloadAllField();
  devicesLen = 0;
  expectedFieldsCount = 0;
}

static void changeDeviceId(uint8_t devId) { //change to selected device ID
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
  expectedFieldsCount = 0; //set this because next target wouldn't have the same count, and this trigger to request the new count
}

static void fieldDeviceIdSelect(FieldProps * field) {
//  TRACE("fieldDeviceIdSelect %x", field->id);
 changeDeviceId(field->id);
 crossfireTelemetryPing();
}

static void parseDeviceInfoMessage(uint8_t* data) {
  uint8_t offset;
  uint8_t id = data[2];
// TRACE("parseDevInfoMsg %x folderAcs %d, expect %d, devsLen %d", id, currentFolderId, expectedFieldsCount, devicesLen);
  offset = strlen((char*)&data[3]) + 1 + 3;
  uint8_t devId = getDevice(id);
  if (!devId) {
    deviceIds[devicesLen] = id;
    if (currentFolderId == otherDevicesId) { // if "Other Devices" opened store devices to fields
      FieldProps deviceField;
      deviceField.id = id;
      deviceField.type = TYPE_DEVICE;
      deviceField.nameLength = offset - 4;
      deviceField.offset = bufferOffset;

      bufferPush((char *)&data[3], deviceField.nameLength);
      storeField(&deviceField);
      if (devicesLen == expectedFieldsCount - 1) {
        allParamsLoaded = 1;
        fieldId = 1;
        addBackButton();
      }
    }
    devicesLen++;
  }

  if (deviceId == id && currentFolderId != otherDevicesId) {
    memcpy(&deviceName[0], (char *)&data[3], DEVICE_NAME_MAX_LEN);
    deviceIsELRS_TX = ((memcmp(&data[offset], "ELRS", 4) == 0) && (deviceId == 0xEE)) ? 1 : 0; // SerialNumber = 'E L R S' and ID is TX module
    uint8_t newFieldCount = data[offset+12];
//    TRACE("deviceId match %x, newFieldCount %d", deviceId, newFieldCount);
    reloadAllField();
    if (newFieldCount != expectedFieldsCount || newFieldCount == 0) {
      expectedFieldsCount = newFieldCount;
      clearFields();
      if (newFieldCount == 0) {
      // This device has no fields so the Loading code never starts
        allParamsLoaded = 1;
        fieldId = 1;
        addBackButton();
      }
    }
  }
}

static const FieldFunctions noopFunctions = { .load=noopLoad, .save=noopSave, .display=noopDisplay };

static const FieldFunctions functions[] = {
  { .load=fieldUint8Load, .save=fieldIntSave, .display=fieldIntegerDisplay }, // 1 UINT8(0)
  // { .load=noopLoad, .save=noopSave, .display=fieldIntegerDisplay }, // 2 INT8(1)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 3 UINT16(2)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 4 INT16(3)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 5 UINT32(4)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 6 INT32(5)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 7 UINT64(6)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 8 INT64(7)
  // { .load=noopLoad, .save=noopSave, .display=noopDisplay }, // 9 FLOAT(8)
  { .load=fieldTextSelectionLoad, .save=fieldIntSave, .display=fieldTextSelectionDisplay }, // 10 TEXT SELECTION(9)
  { .load=noopLoad, .save=noopSave, .display=fieldStringDisplay }, // 11 STRING(10)editing NOTIMPL
  { .load=noopLoad, .save=fieldFolderOpen, .display=fieldUnifiedDisplay }, // 12 FOLDER(11)
  { .load=fieldTextSelectionLoad, .save=noopSave, .display=fieldStringDisplay }, // 13 INFO(12)
  { .load=fieldCommandLoad, .save=fieldCommandSave, .display=fieldUnifiedDisplay }, // 14 COMMAND(13)
  { .load=noopLoad, .save=fieldBackExec, .display=fieldUnifiedDisplay }, // 15 back(14)
  { .load=noopLoad, .save=fieldDeviceIdSelect, .display=fieldUnifiedDisplay }, // 16 device(15)
  { .load=noopLoad, .save=fieldFolderDeviceOpen, .display=fieldUnifiedDisplay }, // 17 deviceFOLDER(16)
};

static FieldFunctions getFunctions(uint32_t i) {
  if (i > TYPE_UINT8) {
    if (i < TYPE_TEXT_SELECTION) return noopFunctions; // guard against not implemented types
    i -= 8;
  }
  return functions[i];
}

static void parseParameterInfoMessage(uint8_t* data, uint8_t length) {
  if (data[2] != deviceId || data[3] != fieldId) {
    fieldDataLen = 0;
    fieldChunk = 0;
    return;
  }
  if (fieldDataLen == 0) {
    expectedChunks = -1;
  }

  // Get by id or use temporary one to decide later if it should be stored
  FieldProps tempField = {0};
  FieldProps* field = getFieldById(fieldId);
  if (field == nullptr) {
    field = &tempField;
  }

  uint8_t chunksRemain = data[4];
  // If no field or the chunksRemain changed when we have data, don't continue
  if (/*field == 0 ||*/ (chunksRemain != expectedChunks && expectedChunks != -1)) {
    return;
  }
  expectedChunks = chunksRemain - 1;

//  TRACE("chunk len %d", length); // to know what is the max single chunk size
  memcpy(&fieldData[fieldDataLen], &data[5], length - 5);
  fieldDataLen += length - 5;

  if (chunksRemain > 0) {
    fieldChunk = fieldChunk + 1;
  } else {
    // TRACE("%d %s %d", fieldId, &fieldData[2], fieldDataLen);
//    DUMP(fieldData, fieldDataLen);
    fieldChunk = 0;
    if (fieldDataLen < 4) {
      fieldDataLen = 0;
      return;
    }

    field->id = fieldId;
    uint8_t parent = fieldData[0];
    uint8_t type = fieldData[1] & 0x7F;
    uint8_t hidden = fieldData[1] & 0x80;
    uint8_t offset;

    if (type > TYPE_COMMAND) {
//      TRACE("type %d", type);
      return;
    }

    if (field->nameLength != 0) {
      if (currentFolderId != parent || field->type != type/* || field->hidden != hidden*/) {
        fieldDataLen = 0;
        return;
      }
    }

    field->type = type;
    offset = strlen((char*)&fieldData[2]) + 1 + 2;

    if (parent != currentFolderId) {
      field->nameLength = 0; // mark as clear
    } else if (!hidden) {
      if (field->nameLength == 0) {
        field->nameLength = offset - 3;
        field->offset = bufferOffset;
        bufferPush((char*)&fieldData[2], field->nameLength);
      }
      getFunctions(field->type).load(field, fieldData, offset);
      storeField(field);
    }

    if (fieldPopup == nullptr) {
      if (fieldId == expectedFieldsCount) { // if we have loaded all params
        // TRACE("bufferOffset %d", bufferOffset);
        // DUMP(buffer, bufferOffset);
        // TRACE("allocatedFieldsCount %d", allocatedFieldsCount);
        allParamsLoaded = 1;
        fieldId = 1;
        if (currentFolderId == 0) {
          otherDevicesState = BTN_REQUESTED;
        } else {
          addBackButton();
        }
      } else if (allParamsLoaded == 0) {
        fieldId++; // fieldId = 1 + (fieldId % (fieldsLen-1));
      }
      fieldTimeout = getTime() + 200;
    } else {
      fieldTimeout = getTime() + fieldPopup->timeout;
    }
    fieldDataLen = 0;
  }
}

static void parseElrsInfoMessage(uint8_t* data) {
  if (data[2] != deviceId) {
    fieldDataLen = 0;
    fieldChunk = 0;
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
  tiny_sprintf(goodBadPkt, "%u/%u   %c", 3, badPkt, goodPkt, state);
}

static void refreshNextCallback(uint8_t command, uint8_t* data, uint8_t length) {
  if (command == CRSF_FRAMETYPE_DEVICE_INFO) {
    parseDeviceInfoMessage(data);
  } else if (command == CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY && currentFolderId != otherDevicesId /* !devicesFolderOpened */) {
    parseParameterInfoMessage(data, length);
    if (allParamsLoaded < 1) {
      fieldTimeout = 0; // request next chunk immediately
    }
  } else if (command == CRSF_FRAMETYPE_ELRS_STATUS) {
    parseElrsInfoMessage(data);
  }
}

static void refreshNext() {
  tmr10ms_t time = getTime();
  if (fieldPopup != nullptr) {
    if (time > fieldTimeout && fieldPopup->status != STEP_CONFIRM) {
      crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_WRITE, fieldPopup->id, 6); // lcsQuery
      fieldTimeout = time + fieldPopup->timeout; // + popup timeout
    }
  } else if (time > devicesRefreshTimeout && expectedFieldsCount < 1) {
    devicesRefreshTimeout = time + 100;
    crossfireTelemetryPing();
  } else if (time > fieldTimeout && expectedFieldsCount != 0) {
    if (allParamsLoaded < 1) {
      crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_READ, fieldId, fieldChunk);
      fieldTimeout = time + 50; // 0.5s
    }
  }

  if (time > linkstatTimeout) {
    if (!deviceIsELRS_TX && allParamsLoaded == 1) {
      goodBadPkt[0] = '\0';
    } else {
      crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_WRITE, 0x0, 0x0); // request linkstat
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
  if (allParamsLoaded != 1 && expectedFieldsCount > 0) {
    luaLcdDrawGauge(0, 1, COL2, barHeight, fieldId, expectedFieldsCount);
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
  if (allocatedFieldsCount == 0) { // if there is no field yet
    return;
  } else {
    // Will stuck on main page because back button is not present
    // if (getFieldById(backButtonId)/*->nameLength*/ == nullptr) { // if back button is not assigned yet, means there is no field yet.
    //   return;
    // }
  }

  if (event == EVT_VIRTUAL_EXIT) {
    if (edit) {
      edit = 0;
      FieldProps * field = getField(lineIndex);
      fieldTimeout = getTime() + 200;
      fieldId = field->id;
      fieldChunk = 0;
      fieldDataLen = 0;
      crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_READ, fieldId, fieldChunk);
    } else {
      if (currentFolderId == 0 && allParamsLoaded == 1) {
        if (deviceId != 0xEE) {
          changeDeviceId(0xEE); // change device id clear expectedFieldsCount, therefore the next ping will do reloadAllField()
        } else {
//          reloadAllField(); // fieldBackExec does it
        }
        crossfireTelemetryPing();
      }
      fieldBackExec();
    }
  } else if (event == EVT_VIRTUAL_ENTER) {
    if (elrsFlags > 0x1F) {
      elrsFlags = 0;
      crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_WRITE, 0x2E, 0x00);
    } else {
      FieldProps * field = getField(lineIndex);
      if (field != 0 && field->nameLength > 0) {
        if (field->type == TYPE_STRING) {
          ; // not implemented
        } else if (field->type < TYPE_FOLDER) {
          edit = 1 - edit;
        }
        if (!edit) {
          if (field->type == TYPE_COMMAND) {
            // For commands, request this field's
            // data again, with a short delay to allow the module EEPROM to
            // commit. Do this before save() to allow save to override
            fieldId = field->id;
            fieldChunk = 0;
            fieldDataLen = 0;
          }
          fieldTimeout = getTime() + 20;
          getFunctions(field->type).save(field);
          if (field->type < TYPE_FOLDER) {
            // For editable field types reload whole folder, but do it after save
            clearFields();
            reloadAllField();
            fieldId = currentFolderId + 1; // Start loading from first folder item
          }
        }
      }
    }
  } else if (edit) {
    if (event == EVT_VIRTUAL_NEXT) {
      incrField(1);
    } else if (event == EVT_VIRTUAL_PREV) {
      incrField(-1);
    }
  } else {
    if (event == EVT_VIRTUAL_NEXT) {
      selectField(1);
    } else if (event == EVT_VIRTUAL_PREV) {
      selectField(-1);
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
    FieldProps * field;
    for (uint32_t y = 1; y < maxLineIndex+2; y++) {
      if (pageOffset+y > allocatedFieldsCount) break;
      field = getField(pageOffset+y);
      if (field == nullptr) {
        break;
      } else if (field->nameLength > 0) {
        uint8_t attr = (lineIndex == (pageOffset+y)) ? ((edit && BLINK) + INVERS) : 0;
        if (field->type < TYPE_FOLDER || field->type == TYPE_INFO) {
          lcdDrawSizedText(COL1, y*textSize+textYoffset, (char *)&buffer[field->offset], field->nameLength, 0);
        }
        getFunctions(field->type).display(field, y*textSize+textYoffset, attr);
      }
    }
  }
}

static uint8_t popupCompat(event_t event) {
  showMessageBox((char *)&fieldData[POPUP_MSG_OFFSET]);
  lcdDrawText(WARNING_LINE_X, WARNING_LINE_Y+2*FH, STR_POPUPS_ENTER_EXIT);

  if (event == EVT_VIRTUAL_EXIT) {
    return RESULT_CANCEL;
  } else if (event == EVT_VIRTUAL_ENTER) {
    return RESULT_OK;
  }
  return RESULT_NONE;
}

static void runPopupPage(event_t event) {
  if (event == EVT_VIRTUAL_EXIT) {
    crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_WRITE, fieldPopup->id, STEP_CANCEL);
    fieldTimeout = getTime() + 200;
  }

  uint8_t result = RESULT_NONE;
  if (fieldPopup->status == STEP_IDLE && fieldPopup->lastStatus != STEP_IDLE) { // stopped
      popupCompat(event);
      reloadAllField();
      fieldPopup = nullptr;
  } else if (fieldPopup->status == STEP_CONFIRM) { // confirmation required
    result = popupCompat(event);
    fieldPopup->lastStatus = fieldPopup->status;
    if (result == RESULT_OK) {
      crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_WRITE, fieldPopup->id, STEP_CONFIRMED); // lcsConfirmed
      fieldTimeout = getTime() + fieldPopup->timeout; // we are expecting an immediate response
      fieldPopup->status = STEP_CONFIRMED;
    } else if (result == RESULT_CANCEL) {
      fieldPopup = nullptr;
    }
  } else if (fieldPopup->status == STEP_EXECUTING) { // running
    result = popupCompat(event);
    fieldPopup->lastStatus = fieldPopup->status;
    if (result == RESULT_CANCEL) {
      crossfireTelemetryPush4(CRSF_FRAMETYPE_PARAMETER_WRITE, fieldPopup->id, STEP_CANCEL);
      fieldTimeout = getTime() + fieldPopup->timeout;
      fieldPopup = nullptr;
    }
  }
}

void elrsStop() {
  registerCrossfireTelemetryCallback(nullptr);
  // reloadAllField();
  fieldBackExec();
  fieldPopup = nullptr;
  deviceId = 0xEE;
  handsetId = 0xEF;

//  if (globalData.cToolRunning) {
    globalData.cToolRunning = 0;
    memclear(reusableBuffer.cToolData, sizeof(reusableBuffer.cToolData));
    popMenu();
//  }
}

void elrsRun(event_t event) {
  if (globalData.cToolRunning == 0) {
    globalData.cToolRunning = 1;
    registerCrossfireTelemetryCallback(refreshNextCallback);
  }

  if (event == EVT_KEY_LONG(KEY_EXIT)) {
    elrsStop();
  } else { 
    if (fieldPopup != nullptr) {
      runPopupPage(event);
    } else {
      runDevicePage(event);
    }

    refreshNext();
  }
}
