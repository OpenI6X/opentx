/**
 * ExpressLRS V2 lua configuration script port to C.
 * 
 * Limitations:
 * - no multiple devices, only ExpressLRS transmitter,
 * - no integer/float/string fields support, ExpressLRS uses only selection anyway,
 * - field unit is not displayed,
 * - dynamically shorten values strings ("AUX" -> "A") to save RAM.
 * 
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
uint8_t *namesBuffer = reusableBuffer.MSC_BOT_Data;
uint8_t namesBufferOffset = 0;
uint8_t *valuesBuffer = &reusableBuffer.MSC_BOT_Data[NAMES_BUFFER_SIZE]; 
uint8_t valuesBufferOffset = 0;

// 84 + safe margin, ideally without trimming 144
// but last 25 are used for popup messages
#define FIELD_DATA_MAX_LEN 120U
static uint8_t *fieldData = &reusableBuffer.MSC_BOT_Data[NAMES_BUFFER_SIZE + VALUES_BUFFER_SIZE];
// static uint8_t fieldData[FIELD_DATA_MAX_LEN];
uint8_t fieldDataLen = 0;

#define FIELDS_MAX_COUNT 32 // 32 * 8 = 256b // 30 + 2 margin for future fields
FieldProps fields[FIELDS_MAX_COUNT]; // = (FieldProps *)&reusableBuffer.MSC_BOT_Data[NAMES_BUFFER_SIZE + VALUES_BUFFER_SIZE]; 
uint8_t fieldsLen = 0;

#define deviceId 0xEE
#define handsetId 0xEF

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
// uint8_t deviceIsELRS_TX = 0;
tmr10ms_t linkstatTimeout = 100;
tmr10ms_t titleShowWarnTimeout = 100;
uint8_t titleShowWarn = 0;

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
  uint8_t crsfPushData[2] = { 0x00, 0xEA };
  crossfireTelemetryPush(0x28, crsfPushData, 2);
}

static void allocateFields() {
  for (uint32_t i = 0; i < fields_count + 1U + 0U; i++) {
    fields[i].nameLength = 0;
    fields[i].valuesLength = 0;
  }
  fieldsLen = fields_count + 1U + 0U;
  backButtonId = fields_count;

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

static uint8_t strRemoveTo(char * src, const char * str, const uint8_t len) {
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
  if (field->valuesLength == 0) {
    memcpy(&valuesBuffer[valuesBufferOffset], (char*)&data[offset], len);
    field->valuesOffset = valuesBufferOffset;
    field->valuesLength = len;
    valuesBufferOffset += len;
  }
  offset += len + 1;
  field->value = data[offset];
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
  lineIndex = 1;
  pageOffset = 0;
  folderAccess = field->id;
  fields[backButtonId].parent = folderAccess;
  for (uint32_t i = 0; i < backButtonId; i++) {
    fields[i].valuesLength = 0;
  }
  reloadAllField();
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
  const char* cmdPat = "[%s]";
  uint8_t textIndent = textXoffset + 9;
  char *pat;
  if (field->type == 11) {
    pat = (char *)folderPat;
    textIndent = textXoffset;
  } else if (field->type == 14) { 
    pat = (char *)backPat;
  } else { 
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
}

static void parseDeviceInfoMessage(uint8_t* data) {
  uint8_t offset;
  uint8_t id = data[2];
  TRACE("parseDeviceInfoMessage %x", id);
  offset = strlen((char*)&data[3]) + 1 + 3; 
  if ( deviceId == id) { 
    memcpy(deviceName, (char *)&data[3], 16);
    // deviceIsELRS_TX = 1; // ((fieldGetValue(data,offset,4) == 0x454C5253) and (deviceId == 0xEE)) or nil -- SerialNumber = 'E L R S' and ID is TX module
    uint8_t newFieldCount = data[offset+12];
    reloadAllField();
    if (newFieldCount != fields_count || newFieldCount == 0) {
      fields_count = newFieldCount;
      allocateFields();
      if (newFieldCount == 0) {
        allParamsLoaded = 1;
        fieldId = 1;
      }
    }
  }
}

static const FieldFunctions functions[] = {
  { .load=fieldTextSelectionLoad, .save=fieldTextSelectionSave, .display=fieldTextSelectionDisplay }, 
  { .load=nullptr, .save=noopOpen, .display=fieldStringDisplay }, 
  { .load=nullptr, .save=fieldFolderOpen, .display=fieldUnifiedDisplay }, 
  { .load=fieldTextSelectionLoad, .save=noopOpen, .display=fieldStringDisplay }, 
  { .load=fieldCommandLoad, .save=fieldCommandSave, .display=fieldUnifiedDisplay }, 
  { .load=nullptr, .save=UIbackExec, .display=fieldUnifiedDisplay } 
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
  // trim values
  uint8_t selectionPos = strlen((char*)&fieldData[2]) + 1 + 2;
  fieldDataLen -= strRemoveTo((char*)&fieldData[selectionPos], "UX", fieldDataLen - selectionPos); // trim AUX to A
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
      if (functions[field->type - 9].load) {
        functions[field->type - 9].load(field, fieldData, offset);
      }
    }

    if (fieldPopup == 0) { 
      if (fieldId == fields_count) {
        TRACE("namesBufferOffset %d", namesBufferOffset);
        DUMP(namesBuffer, NAMES_BUFFER_SIZE);
        TRACE("valuesBufferOffset %d", valuesBufferOffset);
        DUMP(valuesBuffer, VALUES_BUFFER_SIZE);
        allParamsLoaded = 1;
        fieldId = 1;
      } else {
        fieldId++; // fieldId = 1 + (fieldId % (fieldsLen-1));
      }
      fieldTimeout = getTime() + 200;
    } else {
      fieldTimeout = getTime() + fieldPopup->valuesOffset; 
    }
    statusComplete = 1;
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
  char state = (elrsFlags & 1) ? 'C' : '-';
  tiny_sprintf(goodBadPkt, "%u/%u   %c", 0, 3, badPkt, goodPkt, state);

  // If flags are changing, reset the warning timeout to display/hide message immediately
  if (data[6] != elrsFlags) {
    elrsFlags = data[6];
    titleShowWarnTimeout = 0;
  }
  strcpy(elrsFlagsInfo, (char*)&data[7]); 
}

static void refreshNext(uint8_t command = 0, uint8_t* data = 0, uint8_t length = 0) {
  if (command == 0x29) {
    parseDeviceInfoMessage(data);
  } else if (command == 0x2B) {
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
      fieldTimeout = time + 50; 
    }
  }

  if (time > linkstatTimeout) {
    // if (!deviceIsELRS_TX && allParamsLoaded == 1) {
    //   goodBadPkt[0] = '\0';
    // } else {
      crossfireTelemetryPush4(0x2D, 0x0, 0x0); 
    // }
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
        // reloadAllField();
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
      if (field != 0 && field->nameLength > 0) {
        if (field->type == 10) { 
        } else if (field->type < 11) {
          edit = 1 - edit;
        }
        if (!edit) {
          if (field->type < 11 || field->type == 13) {
            fieldTimeout = getTime() + 20;
            fieldId = field->id;
            fieldChunk = 0;
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
        if (functions[field->type - 9].display) {
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
