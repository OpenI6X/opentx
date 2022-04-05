/**
 * ExpressLRS V2 lua configuration script port to C.
 * 
 * Limitations:
 * - no integer/float/string fields support, ExpressLRS uses only selection anyway,
 * - field unit is not displayed,
 * - dynamically shorten values strings ("AUX" -> "A") to save RAM.
 */

#include "opentx.h"
#include "tiny_string.cpp"

#define PACKED __attribute__((packed))

extern uint8_t cScriptRunning;

// disabled, there is enough space now
#define INFO_MAX_LEN 5

struct FieldProps {
  uint8_t nameOffset;     
  uint8_t nameLength;
  uint8_t valuesOffset;  
  uint8_t valuesLength;
  uint8_t parent;
  uint8_t type;// : 4;
  uint8_t value;// : 4;
  uint8_t id;// : 5;         
  // uint8_t hidden : 1;
  // uint8_t spare : 2;     
} PACKED;

struct FieldFunctions {
  void (*load)(FieldProps*, uint8_t *, uint8_t);
  void (*save)(FieldProps*);
  void (*display)(FieldProps*, uint8_t, uint8_t);
};

#define NAMES_BUFFER_SIZE 192 // 156 + margin for future options
#define VALUES_BUFFER_SIZE 176 // 144 + margin for future options
static uint8_t *namesBuffer = reusableBuffer.MSC_BOT_Data;
uint8_t namesBufferOffset = 0;
static uint8_t *valuesBuffer = &reusableBuffer.MSC_BOT_Data[NAMES_BUFFER_SIZE];
uint8_t valuesBufferOffset = 0;

// 84 + safe margin, ideally without trimming 144
// but last 25 are used for popup messages
#define FIELD_DATA_MAX_LEN (512 - NAMES_BUFFER_SIZE - VALUES_BUFFER_SIZE) // 120+
static uint8_t *fieldData = &reusableBuffer.MSC_BOT_Data[NAMES_BUFFER_SIZE + VALUES_BUFFER_SIZE];
// static uint8_t fieldData[FIELD_DATA_MAX_LEN];
uint8_t fieldDataLen = 0;

#define FIELDS_MAX_COUNT 32 // 32 * 8 = 256b // 30 + 2 margin for future fields
static FieldProps fields[FIELDS_MAX_COUNT]; // = (FieldProps *)&reusableBuffer.MSC_BOT_Data[NAMES_BUFFER_SIZE + VALUES_BUFFER_SIZE];
uint8_t fieldsLen = 0;

#define DEVICES_MAX_COUNT 4
static uint8_t deviceIds[DEVICES_MAX_COUNT];
uint8_t devicesLen = 0;
uint8_t otherDevicesId = 255;

uint8_t deviceId = 0xEE;
uint8_t handsetId = 0xEF;

static char deviceName[16];
uint8_t lineIndex = 1;
uint8_t pageOffset = 0;
uint8_t edit = 0; 
uint8_t charIndex = 1;
FieldProps * fieldPopup = 0;
tmr10ms_t fieldTimeout = 0; 
uint8_t fieldId = 1;
uint8_t fieldChunk = 0;

static char goodBadPkt[11] = "?/???    ?";
uint8_t elrsFlags = 0;
static char elrsFlagsInfo[16] = ""; 
uint8_t fields_count = 0;
uint8_t backButtonId = 2; 
tmr10ms_t devicesRefreshTimeout = 50; 
uint8_t allParamsLoaded = 0; 
uint8_t folderAccess = 0; 
uint8_t statusComplete = 0; 
int8_t expectedChunks = -1;
uint8_t deviceIsELRS_TX = 0;
tmr10ms_t linkstatTimeout = 100;
uint8_t titleShowWarn = 0;
tmr10ms_t titleShowWarnTimeout = 100;
uint8_t reloadFolder = 0;

#define COL2           70
#define maxLineIndex   6
#define textXoffset    0
#define textYoffset    3
#define textSize       8

#define tostring(c)       (char *)(c + 48)
#define getTime           get_tmr10ms
#define EVT_VIRTUAL_EXIT  EVT_KEY_BREAK(KEY_EXIT)
#define EVT_VIRTUAL_ENTER EVT_KEY_BREAK(KEY_ENTER)
#define EVT_VIRTUAL_NEXT  EVT_KEY_FIRST(KEY_DOWN)
#define EVT_VIRTUAL_PREV  EVT_KEY_FIRST(KEY_UP)

#define RESULT_OK 2
#define RESULT_CANCEL 1

static void luaLcdDrawGauge(coord_t x, coord_t y, coord_t w, coord_t h, int32_t val, int32_t max)
{
  lcdDrawRect(x, y, w+1, h, 0xff);
  uint8_t len = limit((uint8_t)1, uint8_t(w*val/max), uint8_t(w));
  lcdDrawSolidFilledRect(x+1, y+1, len, h-2);
}

static void allocateFields();
static void reloadAllField();
static FieldProps * getField(uint8_t line);
static void UIbackExec(FieldProps * field);
static void parseDeviceInfoMessage(uint8_t* data);
static void parseParameterInfoMessage(uint8_t* data, uint8_t length);
static void parseElrsInfoMessage(uint8_t* data);
static void refreshNext(uint8_t command, uint8_t* data, uint8_t length);
static void runPopupPage(event_t event);
static void runDevicePage(event_t event);
static void lcd_title();
static void lcd_warn();
static void handleDevicePageEvent(event_t event);


static void crossfireTelemetryPush4(const uint8_t cmd, const uint8_t third, const uint8_t fourth) {
  // TRACE("crsf push %x", cmd);
  uint8_t crsfPushData[4] { deviceId, handsetId, third, fourth };
  crossfireTelemetryPush(cmd, crsfPushData, 4);
}

static void crossfireTelemetryPing(){
  const uint8_t crsfPushData[2] = { 0x00, 0xEA };
  crossfireTelemetryPush(0x28, (uint8_t *) crsfPushData, 2);
}

static void allocateFields() {
  fieldsLen = fields_count + 2U/* + devicesLen*/; // + (back + other devices) + devices count
  TRACE("allocateFields: fieldsLen %d", fieldsLen);
  for (uint32_t i = 0; i < fieldsLen; i++) {
    fields[i].nameLength = 0;
    fields[i].valuesLength = 0;
  }
  backButtonId = fieldsLen - 1;
  TRACE("add back button at %d", backButtonId);
  fields[backButtonId].id = backButtonId + 1;
  fields[backButtonId].nameLength = 1; 
  fields[backButtonId].type = 14;
  fields[backButtonId].parent = (folderAccess == 0) ? 255 : folderAccess;
}

static void reloadAllField() {
  allParamsLoaded = 0;
  fieldId = 1;
  fieldChunk = 0;
  fieldDataLen = 0; 
  namesBufferOffset = 0;
  valuesBufferOffset = 0;
}

static FieldProps * getField(const uint8_t line) {
  uint32_t counter = 1;
  for (uint32_t i = 0; i < fieldsLen; i++) {
    FieldProps * field = &fields[i];
    if (folderAccess == field->parent && field->nameLength != 0/* && field->hidden == 0*/) {
      if (counter < line) {
        counter = counter + 1;
      } else {
        return field;
      }
    }
  }
  return 0;
}

static uint8_t getSemicolonCount(const char * str, const uint8_t len) {
  uint8_t count = 0;
  for (uint32_t i = 0; i < len; i++) {
    if (str[i] == ';') count++;
  }
  return count;
}

static void incrField(int8_t step) {
  FieldProps * field = getField(lineIndex);
  if (field->type == 10) {
  } else {
    uint8_t min, max = 0;
    if (field->type == 9) { 
      min = 0;
      max = getSemicolonCount((char *)&valuesBuffer[field->valuesOffset], field->valuesLength); 
    }
    field->value = limit<uint8_t>(min, field->value + step, max);
  }
}

static void selectField(int8_t step) {
  int8_t newLineIndex = lineIndex;
  FieldProps * field;
  do {
    newLineIndex = newLineIndex + step;
    if (newLineIndex <= 0) {
      newLineIndex = fieldsLen - 1;
    } else if (newLineIndex == 1 + fieldsLen) {
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

static uint8_t getDevice(uint8_t deviceId) {
  TRACE("getDevice %x", deviceId);
  for (uint8_t i = 0; i < devicesLen; i++) {
    if (deviceIds[i] == deviceId) {
      return deviceIds[i];
    }
  }
  return 0;
}

static uint8_t strRemove(char * src, const char * str, const uint8_t len) {
  const char strLen = strlen(str);
  char * srcStrPtr = src;
  uint8_t removedLen = 0;
  while ((srcStrPtr = strstr(srcStrPtr, str)) && (srcStrPtr < src + len)) {
    memcpy(srcStrPtr, srcStrPtr + strLen, (src + len) - (srcStrPtr + strLen));
    removedLen += strLen;
  }

  return removedLen;
}

/**
 * Reused also for INFO fields value (i.e. commit sha) for 0 flash cost
 */
static void fieldTextSelectionLoad(FieldProps * field, uint8_t * data, uint8_t offset) {
  uint8_t len = strlen((char*)&data[offset]);
  field->value = data[offset + len + 1];
  len -= strRemove((char*)&data[offset], "UX", len); // trim AUX to A
  if (field->valuesLength == 0) {
    memcpy(&valuesBuffer[valuesBufferOffset], (char*)&data[offset], len);
    field->valuesOffset = valuesBufferOffset;
    field->valuesLength = len;
    valuesBufferOffset += len;
  }
}

static void fieldTextSelectionSave(FieldProps * field) {
  crossfireTelemetryPush4(0x2D, field->id, field->value);
}

static uint8_t semicolonPos(const char * str, uint8_t last) {
  uint8_t pos = 0;
  while ((str[pos] != ';') && (pos < last)) pos++;
  return pos + 1;
}

static void fieldTextSelectionDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  uint8_t start = field->valuesOffset;
  uint8_t len;
  uint32_t i = 0;
  while (i++ < field->value) {
    start += semicolonPos((char *)&valuesBuffer[start], field->valuesLength - (start - field->valuesOffset));
    if (start - field->valuesOffset >= field->valuesLength) {
      lcdDrawText(COL2, y, "ERR", attr);
      return;
    }
  }
  len = semicolonPos((char *)&valuesBuffer[start], field->valuesLength - (start - field->valuesOffset)) - 1;
  lcdDrawSizedText(COL2, y, (char *)&valuesBuffer[start], len , attr);
}

// shows commit hash, 56b
// no need for it since fieldTextSelectionLoad serves exactly the same purpose for info fields
// static void fieldStringLoad(FieldProps * field, uint8_t * data, uint8_t offset) {
//   field->valuesOffset = valuesBufferOffset;
//   field->valuesLength = strlen((char*)&data[offset]);
//   memcpy(&valuesBuffer[valuesBufferOffset], &data[offset], field->valuesLength);
//   valuesBufferOffset += field->valuesLength;
// }

static void fieldStringDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  lcdDrawSizedText(COL2, y, (char *)&valuesBuffer[field->valuesOffset], field->valuesLength, attr);
}

static void fieldFolderOpen(FieldProps * field) {
  TRACE("fieldFolderOpen %d", field->id);
  lineIndex = 1;
  pageOffset = 0;
  folderAccess = field->id;
  fields[backButtonId].parent = folderAccess;
  for (uint32_t i = 0; i < backButtonId; i++) {
    fields[i].valuesLength = 0;
  }
  reloadAllField();
}

static void fieldFolderDeviceOpen(FieldProps * field) {
  crossfireTelemetryPing(); //broadcast with standard handset ID to get all node respond correctly
// if folderAccess == devices folder, store only devices instead of fields
  fields_count = devicesLen;
  devicesLen = 0;
  fieldsLen = 0;
  return fieldFolderOpen(field);
}

static void noopOpen(FieldProps * field) {}

static void fieldCommandLoad(FieldProps * field, uint8_t * data, uint8_t offset) {
  field->value = data[offset];
  field->valuesOffset = data[offset+1]; 
  strcpy((char *)&fieldData[FIELD_DATA_MAX_LEN - 24 - 1], (char *)&data[offset+2]); 
  if (field->value == 0) { 
    fieldPopup = 0; 
  }
}

static void fieldCommandSave(FieldProps * field) {
  if (field->value < 4) { 
    field->value = 1; 
    fieldTextSelectionSave(field); //crossfireTelemetryPush4(0x2D, field->id, field->value);
    fieldPopup = field;
    fieldPopup->valuesLength = 0; 
    fieldTimeout = getTime() + field->valuesOffset; 
  }
}

static void fieldUnifiedDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  const char* backPat = "[----BACK----]";
  const char* folderPat = "> %s";
  const char* otherPat = "> Other Devices";
  const char* cmdPat = "[%s]";
  uint8_t textIndent = textXoffset + 9;
  char *pat;
  if (field->type == 11) { // FOLDER
    pat = (char *)folderPat;
    textIndent = textXoffset;
  } else if (field->type == 16) { // deviceFOLDER
    pat = (char *)otherPat;
    textIndent = textXoffset;
  } else if (field->type == 14) { // BACK
    pat = (char *)backPat;
  } else { // CMD || DEVICE
    pat = (char *)cmdPat;
  }
  char stringTmp[24];
  tiny_sprintf((char *)&stringTmp, pat, field->nameLength, 1, (char *)&namesBuffer[field->nameOffset]);
  lcdDrawText(textIndent, y, (char *)&stringTmp, attr | BOLD);
}

static void UIbackExec(FieldProps * field = 0) {
  folderAccess = 0;
  fields[backButtonId].parent = 255;
  for (uint32_t i = 0; i < backButtonId; i++) {
    fields[i].valuesLength = 0;
  }
  reloadAllField();
  devicesLen = 0;
  fields_count = 0;
}

static void changeDeviceId(uint8_t devId) { //change to selected device ID
  TRACE("changeDeviceId %x", devId);
  folderAccess = 0;
  deviceIsELRS_TX = 0;
  elrsFlags = 0;
  //if the selected device ID (target) is a TX Module, we use our Lua ID, so TX Flag that user is using our LUA
  if (devId == 0xEE) {
    handsetId = 0xEF;
  } else { //else we would act like the legacy lua
    handsetId = 0xEA;
  }
  deviceId = devId;
  fields_count = 0; //set this because next target wouldn't have the same count, and this trigger to request the new count
}

static void fieldDeviceIdSelect(FieldProps * field) {
  TRACE("fieldDeviceIdSelect %x", field->id);
//  DeviceProps * device = getDevice(field->id);
 changeDeviceId(field->id);
 crossfireTelemetryPing();
}

// kopiuj devices do fields ustawiając parent na "Other devices"
static void createDeviceFields() { // put other devices in the field list
  TRACE("createDeviceFields %d", devicesLen);
//  TRACE("move backbutton from %d to %d", backButtonId, fields_count + 2 + devicesLen);
 fields[fields_count + 2 /* + devicesLen */].id = fields[backButtonId].id;
 fields[fields_count + 2 /* + devicesLen */].nameLength = fields[backButtonId].nameLength;
 fields[fields_count + 2 /* + devicesLen */].type = fields[backButtonId].type;
 fields[fields_count + 2 /* + devicesLen */].parent = fields[backButtonId].parent;
 backButtonId = fields_count + 2 /* + devicesLen */; // move back button to the end of the list, so it will always show up at the bottom.
//  for (uint32_t i = 0; i < devicesLen; i++) {
//    TRACE("createDeviceFields at %d", fields_count+2+i);
//    fields[fields_count+2+i].id = devices[i].id; // fields_count+2+i + 1;
//    fields[fields_count+2+i].nameOffset = devices[i].nameOffset;
//    fields[fields_count+2+i].nameLength = devices[i].nameLength;
//    fields[fields_count+2+i].type = 15;
//    if (devices[i].id == deviceId) {
//      // fields[fields_count+1+i] = {id = fields_count+1+i, name=devices[i].name, parent = 255, type=15}
//      fields[fields_count+2+i].parent = 255;
//    } else {
//      // fields[fields_count+1+i] = {id = fields_count+1+i, name=devices[i].name, parent = fields_count+1, type=15}
//      fields[fields_count+2+i].parent = fields_count+1+1;
//    }
//  }
 fieldsLen = fields_count + 2 /* + devicesLen */ + 1;
//  TRACE("fieldsLen %d", fieldsLen);
}

static void parseDeviceInfoMessage(uint8_t* data) {
  uint8_t offset;
  uint8_t id = data[2];
  TRACE("parseDeviceInfoMessage %x folderAcc %d, f_c %d, devLen %d", id, folderAccess, fields_count, devicesLen);
  offset = strlen((char*)&data[3]) + 1 + 3;
  uint8_t devId = getDevice(id);
  if (!devId) {
    deviceIds[devicesLen] = id;
    if (folderAccess == otherDevicesId) { // if "Other Devices" opened store devices to fields
      fields[devicesLen].id = id;
      fields[devicesLen].type = 15;
      fields[devicesLen].nameLength = offset - 4;
      fields[devicesLen].nameOffset = namesBufferOffset;
      memcpy(&namesBuffer[namesBufferOffset], &data[3], fields[devicesLen].nameLength);
      namesBufferOffset += fields[devicesLen].nameLength;
      if (fields[devicesLen].id == deviceId) {
        fields[devicesLen].parent = 255; // hide current device
      } else {
        fields[devicesLen].parent = otherDevicesId; // set parent to "Other Devices"
      }
      if (devicesLen == fields_count - 1) {
        allParamsLoaded = 1;
        fieldId = 1;
        createDeviceFields();
      }
    }
    devicesLen++;
  }

  if (deviceId == id && folderAccess != otherDevicesId) {
    memcpy(deviceName, (char *)&data[3], 16);
    deviceIsELRS_TX = ((memcmp(&data[offset], "ELRS", 4) == 0) && (deviceId == 0xEE)) ? 1 : 0; // SerialNumber = 'E L R S' and ID is TX module
    uint8_t newFieldCount = data[offset+12];
    TRACE("deviceId match %x, newFieldsCount %d", deviceId, newFieldCount);
    reloadAllField();
    if (newFieldCount != fields_count || newFieldCount == 0) {
      fields_count = newFieldCount;
      allocateFields();
      TRACE("add other devices at %d", fields_count+1);
      otherDevicesId = fields_count+0+1;
      fields[fields_count+0].id = otherDevicesId; // add "Other Devices"
      fields[fields_count+0].nameLength = 1;
      fields[fields_count+0].parent = 255; // hidden initally
      fields[fields_count+0].type = 16;
      if (newFieldCount == 0) {
        allParamsLoaded = 1;
        fieldId = 1;
        createDeviceFields();
      }
    }
  }
}

static const FieldFunctions functions[] = {
  /*
   * 1 UINT8(0)
   * 2 INT8(1)
   * 3 UINT16(2)
   * 4 INT16(3)
   * nil
   * nil
   * nil
   * nil
   * 9 FLOAT(8)
   */
  { .load=fieldTextSelectionLoad, .save=fieldTextSelectionSave, .display=fieldTextSelectionDisplay }, // 10 SELECT(9)
  { .load=nullptr, .save=noopOpen, .display=fieldStringDisplay }, // 11 STRING(10)
  { .load=nullptr, .save=fieldFolderOpen, .display=fieldUnifiedDisplay }, // 12 FOLDER(11)
  { .load=fieldTextSelectionLoad, .save=noopOpen, .display=fieldStringDisplay }, // 13 INFO(12)
  { .load=fieldCommandLoad, .save=fieldCommandSave, .display=fieldUnifiedDisplay }, // 14 COMMAND(13)
  { .load=nullptr, .save=UIbackExec, .display=fieldUnifiedDisplay }, // 15 back(14)
  { .load=nullptr, .save=fieldDeviceIdSelect, .display=fieldUnifiedDisplay }, // 16 device(15)
  { .load=nullptr, .save=fieldFolderDeviceOpen, .display=fieldUnifiedDisplay } // 17 deviceFOLDER(16)
};

static void parseParameterInfoMessage(uint8_t* data, uint8_t length) {
  if (data[2] != deviceId || data[3] != fieldId) {
    fieldDataLen = 0; 
    fieldChunk = 0;
    return;
  }
  if (fieldDataLen == 0) {
    expectedChunks = -1;
  }
  if (fieldId == reloadFolder) { // if we finally receive the folder id, reset the pending reload folder flag
    reloadFolder = 0;
  }
  FieldProps* field = &fields[fieldId - 1];
  uint8_t chunks = data[4];
  if (field == 0 || (chunks != expectedChunks && expectedChunks != -1)) {
    return; 
  }
  expectedChunks = chunks - 1;
  for (uint32_t i = 5; i < length; i++) {
    fieldData[fieldDataLen++] = data[i];
  }
  TRACE("length %d", length); // to know what is the max single chunk size

  if (chunks > 0) {
    fieldChunk = fieldChunk + 1;
    statusComplete = 0;
  } else {
    TRACE("%d, %s, %d", fieldId, &fieldData[2], fieldDataLen);
    DUMP(fieldData, fieldDataLen);
    fieldChunk = 0;
    if (fieldDataLen < 4) { 
      fieldDataLen = 0; 
      return; 
    }

    field->id = fieldId;
    uint8_t parent = fieldData[0]; 
    uint8_t type = fieldData[1] & 0x7F;
    uint8_t hidden = (fieldData[1] & 0x80) ? 1 : 0; 
    uint8_t offset;
    if (field->nameLength != 0) {
      if (field->parent != parent || field->type != type/* || field->hidden != hidden*/) {
        fieldDataLen = 0; 
        return; 
      }
    }
    field->parent = parent;
    field->type = type;
    // field->hidden = hidden;
    offset = strlen((char*)&fieldData[2]) + 1 + 2;

    if (parent != folderAccess || type < 9) { // not current folder or usupported type
      field->nameLength = 0; // mark as clear
    } else {
      if (field->nameLength == 0 && !hidden) {
        field->nameLength = offset - 3; // (field->type == 12/*info*/) ? min(offset - 3, INFO_MAX_LEN) : offset - 3;
        field->nameOffset = namesBufferOffset;
        memcpy(&namesBuffer[namesBufferOffset], &fieldData[2], field->nameLength); 
        namesBufferOffset += field->nameLength;
      }
      if (field->type >= 9 && functions[field->type - 9].load) {
        functions[field->type - 9].load(field, fieldData, offset);
      }
    }

    if (fieldPopup == 0) { 
      if (fieldId == fields_count) { // if we have loaded all params
        TRACE("namesBufferOffset %d", namesBufferOffset);
        DUMP(namesBuffer, NAMES_BUFFER_SIZE);
        TRACE("valuesBufferOffset %d", valuesBufferOffset);
        DUMP(valuesBuffer, VALUES_BUFFER_SIZE);
        allParamsLoaded = 1;
        fieldId = 1;
        createDeviceFields();
      } else if (allParamsLoaded == 0) {
        fieldId++; // fieldId = 1 + (fieldId % (fieldsLen-1));
      } else if (reloadFolder != 0) { // if we still have to reload the folder name
        fieldId = reloadFolder;
        fieldChunk = 0;
        statusComplete = 0;
      }
      fieldTimeout = getTime() + 200;
    } else {
      fieldTimeout = getTime() + fieldPopup->valuesOffset; 
    }
    if (reloadFolder == 0) {
      statusComplete = 1;  // status is not complete, we got to reload the folder
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
  uint16_t goodPkt = (data[4]*256) + data[5];
  uint8_t newFlags = data[6];
  // If flags are changing, reset the warning timeout to display/hide message immediately
  if (newFlags != elrsFlags) {
    elrsFlags = newFlags;
    titleShowWarnTimeout = 0;
  }
  strcpy(elrsFlagsInfo, (char*)&data[7]);

  char state = (elrsFlags & 1) ? 'C' : '-';
  tiny_sprintf(goodBadPkt, "%u/%u   %c", 0, 3, badPkt, goodPkt, state);
}

static void refreshNext(uint8_t command = 0, uint8_t* data = 0, uint8_t length = 0) {
  if (command == 0x29) {
    parseDeviceInfoMessage(data);
  } else if (command == 0x2B && folderAccess != otherDevicesId /* !devicesFolderOpened */) {
    parseParameterInfoMessage(data, length);
    if (allParamsLoaded < 1 || statusComplete == 0) {
      fieldTimeout = 0; 
    }
  } else if (command == 0x2E) {
    parseElrsInfoMessage(data);
  }

  tmr10ms_t time = getTime();
  if (fieldPopup != 0) {
    if (time > fieldTimeout && fieldPopup->value != 3) {
      crossfireTelemetryPush4(0x2D, fieldPopup->id, 6); 
      fieldTimeout = time + fieldPopup->valuesOffset; 
    }
  } else if (time > devicesRefreshTimeout && fields_count < 1) {
    devicesRefreshTimeout = time + 100; 
    crossfireTelemetryPing(); 
  } else if (time > fieldTimeout && fields_count != 0 && !edit) {
    if (allParamsLoaded < 1 || statusComplete == 0) {
      crossfireTelemetryPush4(0x2C, fieldId, fieldChunk); 
      fieldTimeout = time + 50; // 0.5s
    }
  }

  if (time > linkstatTimeout) {
    if (!deviceIsELRS_TX && allParamsLoaded == 1) {
      goodBadPkt[0] = '\0';
      // enable both line below to do what the legacy lua is doing which is reloading all params in an interval
      // reloadAllField()
      // linkstatTimeout = time + 300 //reload all param every 3s if not elrs
    } else {
      crossfireTelemetryPush4(0x2D, 0x0, 0x0); 
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
  if (titleShowWarn) {
    lcdDrawText(LCD_W, 1, tostring(elrsFlags), RIGHT); 
  } else {
    lcdDrawText(LCD_W - 1, 1, goodBadPkt, RIGHT);
    lcdDrawVerticalLine(LCD_W - 10, 0, barHeight, SOLID, INVERS); 
  }

  if (allParamsLoaded != 1 && fields_count > 0) {
    lcdDrawFilledRect(COL2, 0, LCD_W, barHeight, SOLID);
    luaLcdDrawGauge(0, 0, COL2, barHeight, fieldId, fields_count); // 136b
  } else {
    lcdDrawFilledRect(0, 0, LCD_W, barHeight, SOLID);
    if (titleShowWarn) {
      lcdDrawText(textXoffset, 1, elrsFlagsInfo, INVERS);
    } else {
      lcdDrawSizedText(textXoffset, 1, (allParamsLoaded == 1) ? deviceName : "Loading...", 16, INVERS);
    }
  }
}

static void lcd_warn() {
  lcdDrawText(textXoffset, textSize*2, "Error:");
  lcdDrawText(textXoffset, textSize*3, elrsFlagsInfo);
  lcdDrawText(LCD_W/2, textSize*5, "[OK]", BLINK + INVERS + CENTERED);
}

static void handleDevicePageEvent(event_t event) {
  if (fieldsLen == 0) { 
    return;
  } else {
    if (fields[backButtonId].nameLength == 0) { 
      return;
    }
  }

  if (event == EVT_VIRTUAL_EXIT) { 
    if (edit) { 
      edit = 0;
      FieldProps * field = getField(lineIndex);
      fieldTimeout = getTime() + 200; 
      fieldId = field->id;
      fieldChunk = 0;
      fieldDataLen = 0; 
      crossfireTelemetryPush4(0x2C, fieldId, fieldChunk); 
    } else {
      if (folderAccess == 0 && allParamsLoaded == 1) { 
        if (deviceId != 0xEE) {
          changeDeviceId(0xEE); // change device id clear the fields_count, therefore the next ping will do reloadAllField()
        } else {
          reloadAllField();
        }
        crossfireTelemetryPing();
      }
      UIbackExec();
    }
  } else if (event == EVT_VIRTUAL_ENTER) {        
    if (elrsFlags > 0x1F) {
      elrsFlags = 0;
      crossfireTelemetryPush4(0x2D, 0x2E, 0x00);
    } else {
      FieldProps * field = getField(lineIndex);
      if (field != 0 && field->nameLength > 0 && field->type >= 9) {
        if (field->type == 10) { 
        } else if (field->type < 11) {
          edit = 1 - edit;
        }
        if (!edit) {
          if (field->type < 11 || field->type == 13) {
            // For editable field types and commands, request this field's
            // data again, with a short delay to allow the module EEPROM to
            // commit. Do this before save() to allow save to override
            fieldTimeout = getTime() + 20;
            fieldId = field->id;
            fieldChunk = 0;
            statusComplete = 0;
            if (field->parent) {
              // if it is inside a folder, then we reload the folder
              reloadFolder = field->parent;
              fields[field->parent - 1].nameLength = 0;
            }
            fieldDataLen = 0;
          }
          functions[field->type - 9].save(field);
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

  FieldProps * field;
  if (devicesLen > 1) { // show Other Devices folder
    fields[fields_count+0].parent = 0;
  }
  if (elrsFlags > 0x1F) {
    lcd_warn();
  } else {
    for (uint32_t y = 1; y < maxLineIndex+2; y++) {
      if (pageOffset+y >= fieldsLen) break;
      field = getField(pageOffset+y);
      if (field == 0) {
        break;
      } else if (field->nameLength > 0) {
        uint8_t attr = (lineIndex == (pageOffset+y)) ? ((edit && BLINK) + INVERS) : 0;
        if (field->type < 11 or field->type == 12) { 
          lcdDrawSizedText(textXoffset, y*textSize+textYoffset, (char *)&namesBuffer[field->nameOffset], field->nameLength, 0);
        }
        if (field->type >= 9 && functions[field->type - 9].display) {
          functions[field->type - 9].display(field, y*textSize+textYoffset, attr);
        }
      }
    }
  }
}

static uint8_t popupCompat(event_t event) {
  showMessageBox((char *)&fieldData[FIELD_DATA_MAX_LEN - 24 - 1]);
  lcdDrawText(WARNING_LINE_X, WARNING_LINE_Y+2*FH, STR_POPUPS_ENTER_EXIT);

  if (event == EVT_VIRTUAL_EXIT) {
    return RESULT_CANCEL; 
  } else if (event == EVT_VIRTUAL_ENTER) {
    return RESULT_OK; 
  }
  return 0; 
}

static void runPopupPage(event_t event) {
  if (event == EVT_VIRTUAL_EXIT) {
    crossfireTelemetryPush4(0x2D, fieldPopup->id, 5);
    fieldTimeout = getTime() + 200; 
  }

  uint8_t result = 0;
  if (fieldPopup->value == 0 && fieldPopup->valuesLength != 0) { 
      popupCompat(event);
      reloadAllField();
      fieldPopup = 0;
  } else if (fieldPopup->value == 3) { 
    result = popupCompat(event);
    if (fieldPopup != 0) {
      fieldPopup->valuesLength = fieldPopup->value;
    }
    if (result == RESULT_OK) {
      crossfireTelemetryPush4(0x2D, fieldPopup->id, 4); 
      fieldTimeout = getTime() + fieldPopup->valuesOffset; 
      fieldPopup->value = 4; 
    } else if (result == RESULT_CANCEL) {
      fieldPopup = 0;
    }
  } else if (fieldPopup->value == 2) { 
    result = popupCompat(event);
    if (fieldPopup != 0) {
      fieldPopup->valuesLength = fieldPopup->value;
    }
    if (result == RESULT_CANCEL) {
      crossfireTelemetryPush4(0x2D, fieldPopup->id, 5); 
      fieldTimeout = getTime() + fieldPopup->valuesOffset; 
      fieldPopup = 0;
    }
  }
}

void ELRSV2_stop() {
  registerCrossfireTelemetryCallback(nullptr);
  // reloadAllField();
  UIbackExec(); 
  fieldPopup = 0;
  deviceId = 0xEE;
  handsetId = 0xEF;
  if (cScriptRunning) {
    cScriptRunning = 0;
    // memset(reusableBuffer.MSC_BOT_Data, 0, 512);
    popMenu();
  }
}

void ELRSV2_run(event_t event) {
  if (cScriptRunning == 0) { 
    cScriptRunning = 1;
    fields_count = 0;
    fieldsLen = 0;
    registerCrossfireTelemetryCallback(refreshNext);
  }

  if (event == EVT_KEY_LONG(KEY_EXIT)) {
    ELRSV2_stop();
  } else { 
    if (fieldPopup != 0) {
      runPopupPage(event);
    } else {
      runDevicePage(event);
    }

    refreshNext();
  }
}
