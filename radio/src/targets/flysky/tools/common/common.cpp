
#include "opentx.h"

#if defined(PCBI6X_ELRS) || defined(PCBI6X_ELRS_BIND)
void crsfNotEnabledPopup(event_t event)
{
  if (!isModuleCrossfire(EXTERNAL_MODULE)) {
    // dialogs.lua drawNoModule()
    lcdDrawText(7, 0, STR_ELRS_NOT_FOUND, MIDSIZE);
    lcdDrawText(2, 14, STR_ELRS_ENABLE_CRSF);
    if (event == EVT_KEY_BREAK(KEY_EXIT)) popMenu();
    return;
  }
}
#endif

// The editor's chars in the order of the Lua tool (EdgeTX nameChars):
// " a-z 0-9 _ - , ." are 0..40, a negative index is the lower case letter.
// zchar ends with ". ,": without the swap it is the order of the firmware's editName().
static uint8_t editChar(int32_t idx)
{
  if (idx > 38) idx ^= 39 ^ 40;
  return zchar2char(idx);
}

// text_edit.lua, itself a port of the EdgeTX editName(): edit a string, then draw
// it with the cursor kept on screen. Everything the tool needs to edit a string.
bool editString(char * str, uint8_t * cur, event_t event, uint32_t maxLen,
                coord_t x, coord_t y, LcdFlags flags, bool editing)
{
  bool ended = false;
  if (editing) {
    char * p;
    uint32_t c;
    if (event == EVT_KEY_BREAK(KEY_ENTER)) {
      if (*cur >= maxLen - 1) goto commit;
      (*cur)++;
    }
    p = str + *cur;
    if (*p == '\0') {
      // past the end
      p[0] = ' ';
      p[1] = '\0';
    }
    c = *p;
    if (event == EVT_KEY_BREAK(KEY_EXIT)) {
commit:
      // trailing spaces are stripped
      char * end = str + strlen(str);
      while (end > str && end[-1] == ' ') end--;
      *end = '\0';
      ended = true;
    }
    else if (event == EVT_KEY_LONG(KEY_ENTER)) {
      killEvents(event);
      if (c == ' ') goto commit;
      if (((c | 0x20) - 'a') < 26) *p = c ^ 0x20; // toggle case
    }
    else if (event != EVT_KEY_BREAK(KEY_ENTER)) {
      // -26..-1: a lower case letter, 1..26: an upper case one (27, '0', for a foreign char)
      int32_t idx = LEN_STD_CHARS;
      while (idx > -27 && editChar(idx) != c) idx--;
      int32_t now = idx < 0 ? -idx : idx;
      int32_t pos = checkIncDec(event, now, 0, LEN_STD_CHARS, 0);
      // letters are lower case, unless an upper case one is changed
      if (pos != now) *p = editChar(((uint32_t)(idx - 1) < 26 || pos > 26) ? pos : -pos);
    }
  }
  // 9 chars fit the row: slide the string so the cursor stays on screen
  const char * s = str;
  const char * cursor = nullptr;
  if (editing) {
    cursor = s + *cur;
    if (*cur > 8) s = cursor - 8;
  }
  if (!cursor && *s == '\0') s = "---";
  lcdDrawSizedText(x, y, s, 9, flags);
  if (cursor) {
    coord_t cx = x;
    if (cursor != s) cx += getTextWidth(s, cursor - s);
    lcdDrawChar(cx, y, *cursor, INVERS);
  }
  return ended;
}