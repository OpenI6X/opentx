/**
 * ExpressLRS V2 lua configuration script port to C.
 * 
 * Limitations:
 * - no multiple devices, only ExpressLRS transmitter,
 * - no integer/float/string fields support, ExpressLRS uses only selection anyway,
 * - field unit ie.: "mW" is not displayed,
 * - info fields display only label without value,
 * - names are trimmed to 12 characters,
 * - dynamically remove bracketed strings, "AUX" and "mW" from values to save RAM.
 */

#include <stdio.h>
#include <stdlib.h>
#include "opentx.h"

#define PACKED __attribute__((packed))

extern uint8_t cScriptRunning;

#define NAME_MAX_LEN 12

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

uint8_t *namesBuffer = reusableBuffer.MSC_BOT_Data;
uint8_t namesBufferOffset = 0;
uint8_t *valuesBuffer = &reusableBuffer.MSC_BOT_Data[256]; 
uint8_t valuesBufferOffset = 0;

#define deviceId 0xEE
#define handsetId 0xEF

char deviceName[16];
uint8_t lineIndex = 1;
uint8_t pageOffset = 0;
uint8_t edit = 0; 
uint8_t charIndex = 1;
FieldProps * fieldPopup = 0;
tmr10ms_t fieldTimeout = 0; 
uint8_t fieldId = 1;
uint8_t fieldChunk = 0;

#define FIELD_DATA_MAX_LEN 102

uint8_t fieldData[FIELD_DATA_MAX_LEN]; 
uint8_t fieldDataLen = 0;

FieldProps fields[26]; 
uint8_t fieldsLen = 0;

char goodBadPkt[11] = "?/???    ?";
uint8_t elrsFlags = 0;
char elrsFlagsInfo[16] = ""; 
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

void luaLcdDrawGauge(coord_t x, coord_t y, coord_t w, coord_t h, int32_t val, int32_t max)
{
  lcdDrawRect(x, y, w+1, h, 0xff);
  uint8_t len = limit((uint8_t)1, uint8_t(w*val/max), uint8_t(w));
  lcdDrawSolidFilledRect(x+1, y+1, len, h-2);
}

void allocateFields();
void reloadAllField();
FieldProps * getField(uint8_t line);
void UIbackExec(FieldProps * field);
void parseDeviceInfoMessage(uint8_t* data);
void parseParameterInfoMessage(uint8_t* data, uint8_t length);
void parseElrsInfoMessage(uint8_t* data);
void refreshNext(uint8_t command, uint8_t* data, uint8_t length);
void runPopupPage(event_t event);
void runDevicePage(event_t event);
void lcd_title();
void lcd_warn();
void handleDevicePageEvent(event_t event);


void crossfireTelemetryPush4(const uint8_t cmd, const uint8_t third, const uint8_t fourth) {
  // TRACE("crsf push %x", cmd);
  uint8_t crsfPushData[4] { deviceId, handsetId, third, fourth };
  crossfireTelemetryPush(cmd, crsfPushData, 4);
}

void crossfireTelemetryPing(){
  uint8_t crsfPushData[2] = { 0x00, 0xEA };
  crossfireTelemetryPush(0x28, crsfPushData, 2);
}

void allocateFields() {
  for (uint32_t i=0; i < fields_count +1U + 0U; i++) {
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

void reloadAllField() {
  allParamsLoaded = 0;
  fieldId = 1;
  fieldChunk = 0;
  fieldDataLen = 0; 
  namesBufferOffset = 0;
  valuesBufferOffset = 0;
}

FieldProps * getField(const uint8_t line) {
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

uint8_t getSemicolonCount(const char * str, const uint8_t len) {
  uint8_t count = 0;
  for (uint32_t i = 0; i < len; i++) {
    if (str[i] == ';') count++;
  }
  return count;
}

void incrField(int8_t step) {
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

void selectField(int8_t step) {
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

void strRemove(char * src, char * str) {
  char strLen = strlen(str);
  char * srcStrPtr = src;
  while (srcStrPtr = strstr(srcStrPtr, str)) {
    strcpy(srcStrPtr, srcStrPtr + strLen);
  }
}

void strRemoveBetween(char * src, char * begin, char * end) {
    char endLen = strlen(end);
    char * srcStrPtr = src;
    char * srcStrPtr2;
    while (srcStrPtr = strstr(srcStrPtr, begin)) {
        if (srcStrPtr2 = strstr(srcStrPtr, end)) {
            strcpy(srcStrPtr, srcStrPtr2 + 0 /*endLen - 1*/);
        } else {
            break;
        }
    }
}

void fieldTextSelectionLoad(FieldProps * field, uint8_t * data, uint8_t offset) {
  uint8_t len = strlen((char*)&data[offset]);
  uint8_t sLen = len;
  if (field->valuesLength == 0) {
    strRemove((char*)&data[offset], "AUX");
    strRemove((char*)&data[offset], "mW");
    strRemoveBetween((char*)&data[offset], "Hz", ";");
    sLen = strlen((char*)&data[offset]);
    memcpy(&valuesBuffer[valuesBufferOffset], (char*)&data[offset], sLen);
    field->valuesOffset = valuesBufferOffset;
    field->valuesLength = sLen;
    valuesBufferOffset += sLen;
  }
  offset += len + 1;
  field->value = data[offset];
}

void fieldTextSelectionSave(FieldProps * field) {
  crossfireTelemetryPush4(0x2D, field->id, field->value);
}

uint8_t semicolonPos(const char * str, uint8_t last) {
  uint8_t pos = 0;
  while ((str[pos] != ';') && (pos < last)) pos++;
  return pos + 1;
}

void fieldTextSelectionDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
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

void fieldStringDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  lcdDrawSizedText(COL2, y, (char *)&valuesBuffer[field->valuesOffset], field->valuesLength, attr);
}

void fieldFolderOpen(FieldProps * field) {
  lineIndex = 1;
  pageOffset = 0;
  folderAccess = field->id;
  fields[backButtonId].parent = folderAccess;
}

void fieldCommandLoad(FieldProps * field, uint8_t * data, uint8_t offset) {
  field->value = data[offset]; 
  field->valuesOffset = data[offset+1]; 
  strcpy((char *)&fieldData[FIELD_DATA_MAX_LEN - 24 - 1], (char *)&data[offset+2]); 
  if (field->value == 0) { 
    fieldPopup = 0; 
  }
}

void fieldCommandSave(FieldProps * field) {
  if (field->value < 4) { 
    field->value = 1; 
    fieldTextSelectionSave(field); //crossfireTelemetryPush4(0x2D, field->id, field->value);
    fieldPopup = field;
    fieldPopup->valuesLength = 0; 
    fieldTimeout = getTime() + field->valuesOffset; 
  }
}

void fieldUnifiedDisplay(FieldProps * field, uint8_t y, uint8_t attr) {
  const char* backPat = "[----BACK----]";
  const char* folderPat = "> %.*s";
  const char* cmdPat = "[%.*s]";
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
  sprintf((char *)&stringTmp, pat, field->nameLength, (char *)&namesBuffer[field->nameOffset]);
  lcdDrawText(textIndent, y, (char *)&stringTmp, attr | BOLD);
}

void UIbackExec(FieldProps * field = 0) {
  folderAccess = 0;
  fields[backButtonId].parent = 255;
}

void parseDeviceInfoMessage(uint8_t* data) {
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

const FieldFunctions functions[] = {
  { .load=fieldTextSelectionLoad, .save=fieldTextSelectionSave, .display=fieldTextSelectionDisplay }, 
  { .load=nullptr, .save=nullptr, .display=fieldStringDisplay }, 
  { .load=nullptr, .save=fieldFolderOpen, .display=fieldUnifiedDisplay }, 
  { .load=nullptr, .save=nullptr, .display=fieldStringDisplay }, 
  { .load=fieldCommandLoad, .save=fieldCommandSave, .display=fieldUnifiedDisplay }, 
  { .load=nullptr, .save=UIbackExec, .display=fieldUnifiedDisplay } 
};

void parseParameterInfoMessage(uint8_t* data, uint8_t length) {
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
  for (uint32_t i=5; i< length; i++) {
    fieldData[fieldDataLen++] = data[i];
  }
  if (chunks > 0) {
    fieldChunk = fieldChunk + 1;
    statusComplete = 0;
  } else { 
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
    if (field->nameLength == 0 && !hidden) {
      field->nameLength = min(offset - 3, NAME_MAX_LEN);
      field->nameOffset = namesBufferOffset;
      memcpy(&namesBuffer[namesBufferOffset], &fieldData[2], field->nameLength); 
      namesBufferOffset += field->nameLength;
    }
    if (functions[field->type - 9].load) {
      functions[field->type - 9].load(field, fieldData, offset);
    }

    if (fieldPopup == 0) { 
      if (fieldId == fields_count) {
        TRACE("namesBufferOffset %d", namesBufferOffset);
        DUMP(namesBuffer, 256);
        TRACE("valuesBufferOffset %d", valuesBufferOffset);
        DUMP(valuesBuffer, 256);
        allParamsLoaded = 1;
        fieldId = 1;
      } else {
        fieldId = 1 + (fieldId % (fieldsLen-1));
      }
      fieldTimeout = getTime() + 200;
    } else {
      fieldTimeout = getTime() + fieldPopup->valuesOffset; 
    }
    statusComplete = 1;
    fieldDataLen = 0; 
  }
}

void parseElrsInfoMessage(uint8_t* data) {
  if (data[2] != deviceId) {
    fieldDataLen = 0; 
    fieldChunk = 0;
    return;
  }

  uint8_t badPkt = data[3];
  uint16_t goodPkt = (data[4]*256) + data[5];
  char state = (elrsFlags & 1) ? 'C' : '-';
  sprintf(goodBadPkt, "%u/%u   %c", badPkt, goodPkt, state); 

  // If flags are changing, reset the warning timeout to display/hide message immediately
  if (data[6] != elrsFlags) {
    elrsFlags = data[6];
    titleShowWarnTimeout = 0;
  }
  strcpy(elrsFlagsInfo, (char*)&data[7]); 
}

void refreshNext(uint8_t command = 0, uint8_t* data = 0, uint8_t length = 0) {
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

void lcd_title() {
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

void lcd_warn() {
  lcdDrawText(textSize*3, textSize*2, elrsFlagsInfo, 0);
  lcdDrawText(textSize*10, textSize*6, "ok", BLINK + INVERS);
}

void handleDevicePageEvent(event_t event) {
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
        reloadAllField();
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

void runDevicePage(event_t event) {
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

uint8_t popupCompat(event_t event) {
  showMessageBox((char *)&fieldData[FIELD_DATA_MAX_LEN - 24 - 1]);
  lcdDrawText(WARNING_LINE_X, WARNING_LINE_Y+2*FH, STR_POPUPS_ENTER_EXIT);

  if (event == EVT_VIRTUAL_EXIT) {
    return RESULT_CANCEL; 
  } else if (event == EVT_VIRTUAL_ENTER) {
    return RESULT_OK; 
  }
  return 0; 
}

void runPopupPage(event_t event) {
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
  reloadAllField(); 
  UIbackExec(); 
  fieldPopup = 0;
  if (cScriptRunning) {
    cScriptRunning = 0;
    memset(reusableBuffer.MSC_BOT_Data, 0, 512);
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
