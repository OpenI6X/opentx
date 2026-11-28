/*
 * ExpressLRS Bind phrase tool for the FlySky i6X.
 *
 * A C++ port of the black & white UI of the ExpressLRS Lua tool "ExpressLRS Bind"
 *   https://github.com/ExpressLRS/Lua  src/SCRIPTS/TOOLS/ExpressLRSBind
 *   (main.lua, ui/lcd.lua, history_storage.lua and from SCRIPTS/ELRS: msp.lua,
 *   defer.lua, ui/lcd/text_edit.lua, ui/lcd/dialogs.lua; commit 116eb90)
 * Same pages, rows, texts, pixel positions and CRSF frames. It sets the bind phrase
 * (or a raw UID) of the TX module, the receiver or both over MSP, which needs
 * ExpressLRS 4.1+ on the device, sends bind / unbind, and keeps a history.
 *
 * Keys: UP / DOWN select a row or change a value (with auto repeat), OK activates,
 * CANCEL leaves. In the phrase editor OK moves to the next char, a long OK toggles
 * the case (on a space: done), CANCEL is done. A long OK on a history entry deletes it.
 *
 * What differs from the Lua tool, by design:
 *  - History: the 32 spare EEPROM bytes at 96..127, between the file system header
 *    (EeFs) and the first data block, instead of a file on the SD card:
 *      [96]       HIST_MAGIC
 *      [97..127]  the phrases, newest first, each NUL terminated; an empty one ends
 *                 the list
 *    So it holds the newest phrases that fit in 31 bytes (at most 5, as in Lua). A
 *    phrase of more than 30 chars (the limit is 52, as in Lua) is sent, but not kept.
 *  - The timers count GUI frames (MENU_TASK_PERIOD, 50 ms) instead of ticks.
 *  - The page is drawn every frame. The Lua tool redraws on events and status
 *    changes only, which leaves it stale at times (BLINK does not blink, "Updating..."
 *    stays after the refresh of an unchanged UID, the page under a popup is frozen).
 *  - A long OK on a history entry opens "Delete entry?" and waits. In the Lua tool
 *    the key release answers that popup with OK, unless it comes within one frame.
 *  - "Both" follows the pending timer, not a flag of its own: changing the target
 *    between its two writes no longer rewrites the RX forever.
 *  - No "No ExpressLRS" hint about an internal module: the i6X has none for CRSF.
 *  - The EXIT row shows the firmware version (Lua: the version of the scripts).
 *
 * Size (see also the notes at the functions):
 *  - No static data at all: the state lives in reusableBuffer.cToolData, as in elrs.cpp.
 *  - The texts are STR_ELRS_* strings (untranslated.h), so TRZ compresses them. Those
 *    that depend on the target or on telemetry are "index + value" in a table; uses
 *    that are not draw calls are marked TRZ_TEXT() for radio/util/trz.py.
 *  - Rows are drawn top down by one function that tells its caller when the row is
 *    selected and a key came: no table of rows, no second pass for the keys.
 *  - Everything that goes out is built by one function, send().
 *  - The firmware does the popup, the value editing (checkIncDec, with its beeps and
 *    auto repeat), the editor's char set (zchar2char) and the text width.
 *  - The telemetry callback pointer doubles as the "tool is running" flag, so the
 *    tool does not need globalData.cToolRunning and the firmware clears the LCD.
 */

#include "opentx.h"
#include "common/common.h"

// CRSF / MSP over CRSF (ExpressLRS msptypes.h, RxTxEndpoint.cpp)
static constexpr uint8_t FRAMETYPE_MSP_REQ    = 0x7A;
static constexpr uint8_t FRAMETYPE_MSP_RESP   = 0x7B;
static constexpr uint8_t FRAMETYPE_MSP_WRITE  = 0x7C;
static constexpr uint8_t MSP_HEADER_V1        = 0x30; // v1 | start-of-frame | seq 0
static constexpr uint8_t MSP_ELRS_RXTX_CONFIG = 0x2D;
static constexpr uint8_t RXTX_UID             = 0x00;
static constexpr uint8_t RXTX_BIND_PHRASE     = 0x01;

// GUI frames
static constexpr uint8_t UID_RETRY_FRAMES = 10; // 0.5 s
static constexpr uint8_t UID_MAX_ATTEMPTS = 12; // 6 s, outlasts a module reboot
static constexpr uint8_t FOLLOWUP_FRAMES  = 20; // 1 s, settle after a write

// History
static constexpr uint8_t  PHRASE_MAX = 52; // fits a single MSP frame (msp.lua)
static constexpr uint8_t  HIST_SIZE  = 31;
static constexpr uint8_t  HIST_MAX   = 5;
static constexpr uint8_t  HIST_MAGIC = 0xB1;
static constexpr uint16_t HIST_ADDR  = sizeof(EeFs);
static_assert(sizeof(EeFs) + 1 + HIST_SIZE <= RESV, "No spare EEPROM bytes after EeFs");
static_assert(PHRASE_MAX + 10 <= TELEMETRY_OUTPUT_BUFFER_SIZE, "A phrase does not fit a CRSF frame");

enum { TARGET_TX, TARGET_RX, TARGET_BOTH };
enum { DEFER_NONE, DEFER_REQUEST_UID, DEFER_START_UID, DEFER_SEND_SET, DEFER_SENT };
enum { PAGE_MAIN, PAGE_HISTORY };
enum { ROW_PHRASE, ROW_TARGET }; // the first rows of the main page; the rest is counted

static constexpr uint8_t EVT_CONFIRMED = 1; // no key event: the popup was answered with OK

static constexpr uint8_t COL2  = 70;
static constexpr uint8_t ROW_Y = FH + 3; // of the first row

// The texts that are picked by the target or by telemetry: "index + value"
enum { IDX_TARGET = 0, IDX_SETTING = 3, IDX_BIND = 6, IDX_SENDING = 8 };
static const char * const indexed[] = {
  TRZ_TEXT(STR_ELRS_TX), TRZ_TEXT(STR_ELRS_RX), TRZ_TEXT(STR_ELRS_BOTH),
  TRZ_TEXT(STR_ELRS_SETTING_TX), TRZ_TEXT(STR_ELRS_SETTING_RX), TRZ_TEXT(STR_ELRS_SETTING_BOTH),
  TRZ_TEXT(STR_ELRS_BIND), TRZ_TEXT(STR_ELRS_UNBIND),
  TRZ_TEXT(STR_ELRS_SENDING_BIND), TRZ_TEXT(STR_ELRS_SENDING_UNBIND),
};

struct BindData {
  const char * status;          // text; nullptr shows the UID
  uint8_t deferFn;              // DEFER_*: one pending callback, a new one replaces it
  uint8_t deferFrames;
  uint8_t target;
  uint8_t attempts;             // UID requests without an answer
  uint8_t cur;                  // phrase editor cursor
  uint8_t tele;                 // RX telemetry is alive
  union {
    struct {
      uint8_t page;
      uint8_t line;             // selected row
    };
    uint16_t pageLine;          // a page number goes to its first row
  };
  uint8_t rows;                 // on the page, as drawn last
  uint8_t row;                  // next row to draw
  uint8_t event;                // for the selected row
  uint8_t selected;             // flags of the selected row: INVERS, + BLINK while editing
  uint8_t reachable;            // not the RX without a link
  uint8_t uidFrom;              // the answer: origin, 4 MSP header bytes, UID
  uint8_t mspHeader[4];
  uint8_t uid[6];
  char phrase[PHRASE_MAX + 2];
  uint8_t magic;                // the EEPROM image: magic, hist[HIST_SIZE]
  char hist[2 * HIST_SIZE + 2]; // the upper half stays 0
};
static_assert(sizeof(BindData) <= CTOOL_DATA_SIZE, "BindData exceeds cToolData");
static_assert(offsetof(BindData, hist) == offsetof(BindData, magic) + 1, "EEPROM image");
static_assert(offsetof(BindData, uid) == offsetof(BindData, uidFrom) + 5, "MSP answer");

static inline BindData * bindData()
{
  return (BindData *)(void *)reusableBuffer.cToolData;
}

// The state has a fixed address, so GCC loads the address of each member it needs
// from a 4 byte literal. Hiding the pointer makes them register + offset instead,
// and lets the state be passed on in a register.
#define OPAQUE(ptr) __asm__("" : "+r"(ptr))

static void setTimeout(BindData * d, uint32_t fn, uint32_t frames)
{
  d->deferFn = fn;
  d->deferFrames = frames;
}

// 4-6 comma separated decimal bytes (1-3 digits, spaces around them are fine) are a
// UID, left padded with zeros to 6. uid: 6 zeros.
static bool parseUid(const char * p, uint8_t * uid)
{
  uint32_t count = 0;
  for (;;) {
    while (*p == ' ') p++;
    const char * first = p;
    uint32_t value = 0;
    uint32_t digit;
    while ((digit = *p - '0') < 10) {
      value = value * 10 + digit;
      p++;
    }
    uint32_t digits = p - first;
    while (*p == ' ') p++;
    if (digits - 1 > 2 || value > 255 || count == 6) return false;
    memmove(uid, uid + 1, 5);
    uid[5] = value;
    count++;
    if (*p == ',') {
      if (*++p) continue; // a comma at the very end is accepted, as in Lua
    }
    else if (*p) {
      return false;
    }
    return count > 3;
  }
}

// Every frame the tool sends:
//  - MSP_REQ:    MSP_ELRS_RXTX_CONFIG, read the UID                {dest, origin, 0x30, 1, 0x2D, 0}
//  - MSP_WRITE:  MSP_ELRS_RXTX_CONFIG, write a UID                 {..., 7, 0x2D, 0, uid[6]}
//                or the bind phrase s                              {..., 1 + n, 0x2D, 1, s[n]}
//  - COMMAND_ID: bind (dest TX) or unbind (dest RX)                {dest, origin, 0x10, 1}
// No MSP crc and no chunking: the ExpressLRS endpoint reads the payload by offset.
static void send(uint32_t type, uint32_t dest, const char * s)
{
  union {
    uint8_t b[6 + PHRASE_MAX + 2];
    uint32_t w[3];
  } frame;
  frame.w[1] = 0; // subcommand RXTX_UID, and the zeros parseUid() wants
  frame.w[2] = 0;
  frame.b[0] = dest;
  frame.b[1] = CSRF_ADDRESS_RADIO;
  frame.b[2] = MSP_HEADER_V1;
  frame.b[4] = MSP_ELRS_RXTX_CONFIG;
  uint32_t len = 0;
  if (s) {
    len = 6;
    if (!parseUid(s, &frame.b[6])) {
      frame.b[5] = RXTX_BIND_PHRASE;
      len = strlen(s);
      memcpy(&frame.b[6], s, len);
    }
  }
  frame.b[3] = len + 1;
  len += 6;
  if (type == COMMAND_ID) {
    frame.b[2] = SUBCOMMAND_CRSF;
    len = 4;
  }
  crossfireTelemetryPush(type, frame.b, len);
}

// ---------------------------------------------------------------------------
// History (history_storage.lua)
// ---------------------------------------------------------------------------

static char * histNext(char * p)
{
  return p + strlen(p) + 1;
}

static void histSave(BindData * d)
{
  eepromWriteBlock(&d->magic, HIST_ADDR, 1 + HIST_SIZE);
}

// The phrase goes to the front. One that is in the list already moves there, else
// the oldest ones that no longer fit are dropped. One that is too long is not kept.
static void histAdd(BindData * d)
{
  char * hist = d->hist;
  OPAQUE(hist);
  uint32_t len = strlen(d->phrase) + 1;
  if (len > HIST_SIZE) return;
  char * p = hist;
  while (*p && strncmp(p, d->phrase, HIST_SIZE)) p = histNext(p);
  if (*p == '\0') p = hist + HIST_SIZE - len; // not there: the tail falls off
  memmove(hist + len, hist, p - hist);
  memcpy(hist, d->phrase, len);
  for (p = &hist[HIST_SIZE - 1]; *p; p--) *p = '\0'; // an entry cut by the shift
  p = hist;
  for (uint32_t i = HIST_MAX; i; i--) p = histNext(p);
  *p = '\0';
  eepromWriteBlock((uint8_t *)hist - 1, HIST_ADDR, 1 + HIST_SIZE); // histSave()
}

// ---------------------------------------------------------------------------
// App (main.lua)
// ---------------------------------------------------------------------------

// Bounded: devices before 4.1 never answer
static void requestUid(BindData * d)
{
  uint32_t target = d->target;
  const char * status = TRZ_TEXT(STR_ELRS_IDLE);
  // reachable: the TX, or the RX over a live link
  if (target <= (uint32_t)TELEMETRY_STREAMING()) {
    status = TRZ_TEXT(STR_ELRS_NO_RESPONSE);
    if (d->attempts < UID_MAX_ATTEMPTS) {
      d->attempts++;
      status = TRZ_TEXT(STR_ELRS_UPDATING);
      send(FRAMETYPE_MSP_REQ, CSRF_ADDRESS_TX - 2 * target, nullptr);
      setTimeout(d, DEFER_REQUEST_UID, UID_RETRY_FRAMES);
    }
  }
  d->status = status;
}

static void startUidRequest(BindData * d)
{
  d->attempts = 0;
  requestUid(d);
}

// Both: the RX first, as the write drops it off the link, then (second) the TX
static void sendSet(BindData * d, uint32_t second)
{
  if (d->phrase[0] == '\0') return;

  uint32_t target = d->target;
  uint32_t next = DEFER_START_UID;
  if (target == TARGET_BOTH) {
    if (second)
      d->target = target = TARGET_TX;
    else
      next = DEFER_SEND_SET;
  }
  d->status = indexed[IDX_SETTING + target];
  send(FRAMETYPE_MSP_WRITE, target == TARGET_TX ? CSRF_ADDRESS_TX : CSRF_ADDRESS_RX, d->phrase);
  histAdd(d);
  setTimeout(d, next, FOLLOWUP_FRAMES);
}

// The answer to the UID request, a single frame v1 MSP_RESP for the handset
// (msp.lua decodeResponse / decodeUid). data[0] is the frame type, so the indexes
// are those of the Lua tables. Runs in the telemetry task, above the menus.
static void elrsBindCallback(uint8_t command, uint8_t * data, uint8_t length)
{
  if (command != FRAMETYPE_MSP_RESP) return;
  if (data[1] != CSRF_ADDRESS_RADIO) return;
  if ((data[3] & 0x70) != MSP_HEADER_V1) return; // start-of-frame, v1
  uint8_t size = data[4];
  if (size < 7 || length <= 5 + size) return;
  if (data[5] != MSP_ELRS_RXTX_CONFIG || data[6] != RXTX_UID) return;
  BindData * d = bindData();
  memcpy(&d->uidFrom, &data[2], 5 + 6);
  d->deferFn = DEFER_NONE;
  d->status = nullptr;
  d->attempts = 0;
}

// ---------------------------------------------------------------------------
// UI (ui/lcd.lua)
// ---------------------------------------------------------------------------

// Draws the next row. Returns the event when the row is the selected one.
static uint32_t drawRow(coord_t x, LcdFlags flags, const char * s)
{
  BindData * d = bindData();
  uint32_t row = d->row++;
  uint32_t event = 0;
  if (row == d->line) {
    flags |= d->selected;
    event = d->event;
  }
  lcdDrawText(x, row * FH + ROW_Y, s, flags);
  return event;
}

// "[...]" rows
static uint32_t button(const char * s)
{
  return drawRow(s == STR_ELRS_HISTORY ? 0 : 10, BOLD, s);
}

static void stop()
{
  registerCrossfireTelemetryCallback(nullptr);
  popMenu();
}

void elrsBindRun(event_t event)
{
  BindData * d = bindData();
  OPAQUE(d);

  if (!isModuleCrossfire(EXTERNAL_MODULE)) {
    crsfNotEnabledPopup(event);
    return;
  }

  if (crossfireTelemetryCallback != elrsBindCallback) {
    // first run
    memclear(d, sizeof(BindData));
    eepromReadBlock(&d->magic, HIST_ADDR, 1 + HIST_SIZE);
    if (d->magic != HIST_MAGIC) {
      // never written, or not ours: an empty list (what follows is never looked at)
      d->magic = HIST_MAGIC;
      d->hist[0] = '\0';
    }
    d->hist[HIST_SIZE - 1] = '\0'; // whatever it holds, the list ends here
    strcpy(d->phrase, d->hist);
    registerCrossfireTelemetryCallback(elrsBindCallback);
    startUidRequest(d);
  }
  else {
    // defer.lua poll()
    uint32_t fn = d->deferFn;
    if (fn != DEFER_NONE && --d->deferFrames == 0) {
      d->deferFn = DEFER_NONE; // the callback may schedule a successor
      if (fn == DEFER_SENT)
        d->status = TRZ_TEXT(STR_ELRS_SENT);
      else if (fn == DEFER_SEND_SET)
        sendSet(d, true);
      else if (fn == DEFER_START_UID)
        startUidRequest(d);
      else
        requestUid(d);
    }
  }

  uint32_t tele = TELEMETRY_STREAMING();
  d->tele = tele;
  d->reachable = (d->target ^ TARGET_RX) | tele;

  // "Delete entry?" / "Clear history?" was confirmed: tell the selected row
  if (warningResult) {
    warningResult = 0;
    event = EVT_CONFIRMED;
  }

  uint32_t line = d->line;

  // --- keys ---
  LcdFlags selected = INVERS;
  if (s_editMode > 0) {
    selected = INVERS | BLINK;
    if (line != ROW_PHRASE) {
      // target
      if (event == EVT_KEY_BREAK(KEY_ENTER) || event == EVT_KEY_BREAK(KEY_EXIT))
        s_editMode = 0;
      else
        d->target = checkIncDec(event, d->target, TARGET_TX, TARGET_BOTH, 0);
      event = 0;
    }
    // the phrase is edited by editString() below, which consumes the key and draws it
  }
  else if (event == EVT_KEY_BREAK(KEY_EXIT)) {
    if (d->page == PAGE_MAIN)
      stop();
    else
      line = d->page = PAGE_MAIN;
  }
  else if (event == EVT_KEY_FIRST(KEY_DOWN)) {
    if (++line >= d->rows) line = 0;
  }
  else if (event == EVT_KEY_FIRST(KEY_UP)) {
    if (line == 0) line = d->rows;
    line--;
  }
  d->line = line;
  d->event = event;
  d->selected = selected;
  d->row = 0;

  // --- title ---
  lcdDrawText(0, 0, STR_ELRS_TITLE);
  lcdDrawChar(LCD_W - 7, 0, tele ? 'C' : '-'); // C while RX telemetry is alive
//  lcdDrawSolidFilledRect(0, 0, LCD_W, 9);
  lcdInvertLine(0);

  // --- rows; no scrolling: a page has at most 7 ---
  if (d->page == PAGE_MAIN) {
    lcdDrawText(0, ROW_Y, STR_ELRS_LABELS);

    // phrase (text_edit.lua, itself a port of the EdgeTX editName())
    d->row = ROW_TARGET;
    bool editing = s_editMode > 0 && line == ROW_PHRASE;
    LcdFlags flags = (line == ROW_PHRASE && !editing) ? INVERS : 0;
    if (line == ROW_PHRASE && !editing && event == EVT_KEY_BREAK(KEY_ENTER) && d->reachable) {
      d->cur = 0;
      s_editMode = EDIT_MODIFY_FIELD;
    }
    if (editString(d->phrase, &d->cur, event, PHRASE_MAX, COL2, ROW_Y, flags, editing))
      s_editMode = 0;

    // target
    if (drawRow(COL2, 0, indexed[IDX_TARGET + d->target]) == EVT_KEY_BREAK(KEY_ENTER)) {
      s_editMode = EDIT_MODIFY_FIELD;
    }

    // status or UID; compact to fit 128 px at SMLSIZE
    union {
      char c[4 + 6 * 4];
      uint32_t prefix;
    } uid;
    const char * s = d->status;
    if (!s) {
      s = uid.c;
      // "TX: " or "RX: " in one go: 'T' - 0xEE == 'R' - 0xEC
      uid.prefix = d->uidFrom + ((' ' << 24) + (':' << 16) + ('X' << 8) + 'T' - CSRF_ADDRESS_TX);
      char * b = &uid.c[4];
      for (uint32_t i = 0; i < 6; i++) {
        b = strAppendUnsigned(b, d->uid[i]);
        *b++ = ',';
      }
      b[-1] = '\0';
    }
    if (drawRow(0, SMLSIZE, s) == EVT_KEY_BREAK(KEY_ENTER)) {
      startUidRequest(d);
    }

    if (button(TRZ_TEXT(STR_ELRS_SET)) == EVT_KEY_BREAK(KEY_ENTER) && d->reachable) {
      sendSet(d, d->deferFn == DEFER_SEND_SET);
    }
    if (d->target == TARGET_RX && button(indexed[IDX_BIND + tele]) == EVT_KEY_BREAK(KEY_ENTER)) {
      // to the TX: enter bind mode; to the connected RX: unbind
      d->status = indexed[IDX_SENDING + tele];
      send(COMMAND_ID, CSRF_ADDRESS_TX - 2 * tele, nullptr);
      setTimeout(d, DEFER_SENT, FOLLOWUP_FRAMES);
    }
    if (button(TRZ_TEXT(STR_ELRS_HISTORY)) == EVT_KEY_BREAK(KEY_ENTER)) {
      d->pageLine = PAGE_HISTORY;
    }
    if (button(TRZ_TEXT(STR_ELRS_EXIT)) == EVT_KEY_BREAK(KEY_ENTER)) {
      stop();
    }
  }
  else {
    for (char * p = d->hist; *p; ) {
      uint32_t e = drawRow(0, 0, p);
      if (e == EVT_CONFIRMED) {
        memmove(p, histNext(p), HIST_SIZE); // the zeros come from the upper half
        histSave(d);
        continue;
      }
      if (e == EVT_KEY_BREAK(KEY_ENTER)) {
        strcpy(d->phrase, p);
        d->pageLine = PAGE_MAIN;
      }
      else if (e == EVT_KEY_LONG(KEY_ENTER)) {
        killEvents(e); // or the release answers the popup
        POPUP_CONFIRMATION(STR_ELRS_DELETE_ENTRY);
      }
      p = histNext(p);
    }
    uint32_t e = button(TRZ_TEXT(STR_ELRS_CLEAR));
    if (e == EVT_KEY_BREAK(KEY_ENTER)) {
      POPUP_CONFIRMATION(STR_ELRS_CLEAR_HISTORY);
    }
    else if (e == EVT_CONFIRMED) {
      memclear(d->hist, HIST_SIZE);
      histSave(d);
    }
    if (button(TRZ_TEXT(STR_ELRS_BACK)) == EVT_KEY_BREAK(KEY_ENTER)) {
      d->pageLine = PAGE_MAIN;
    }
  }

  // the selection stays on the page, e.g. after [Clear All]
  uint32_t rows = d->row;
  d->rows = rows;
  if (d->line >= rows) d->line = rows - 1;
}
