/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

// ExpressLRS Bind tool (targets/flysky/tools/elrs_bind.cpp), driven through
// guiMain() like on the radio: the CRSF frames it sends, its history in the spare
// EEPROM bytes 96..127 and the main elements of its pages.
// Configure with -DPCBI6X_ELRS_BIND=YES.

#include "gtests.h"
#include "telemetry/crossfire.h"
#include "stamp.h"
#include <string>
#include <vector>

#if defined(PCBI6X_ELRS_BIND)

extern uint8_t eeprom[];
void elrsBindRun(event_t event);
void guiMain(event_t evt);
uint8_t crc8(const uint8_t * ptr, uint32_t len);

namespace {

const event_t ENTER      = EVT_KEY_BREAK(KEY_ENTER);
const event_t ENTER_LONG = EVT_KEY_LONG(KEY_ENTER);
const event_t EXIT       = EVT_KEY_BREAK(KEY_EXIT);
const event_t DOWN       = EVT_KEY_FIRST(KEY_DOWN);
const event_t UP         = EVT_KEY_FIRST(KEY_UP);

const int HIST_ADDR = 96; // sizeof(EeFs): the 32 bytes up to RESV are not used by the file system
const uint8_t HIST_MAGIC = 0xB1;
const size_t PHRASE_MAX = 52; // of the editor, as in the Lua tool: a single MSP frame
const size_t STORED_MAX = 30; // of the history: 31 bytes, NUL terminated

class ElrsBindTest : public OpenTxTest
{
  protected:
    uint8_t lastFrameSize = 0;

    void SetUp() override
    {
      OpenTxTest::SetUp();
      TELEMETRY_RESET();
      telemetryData.rssi.reset();
      g_model.moduleData[EXTERNAL_MODULE].type = MODULE_TYPE_CROSSFIRE;
      memset(&eeprom[HIST_ADDR], 0xFF, 32);                  // never written
      memset(&reusableBuffer, 0x5A, sizeof(reusableBuffer)); // left by another menu
      registerCrossfireTelemetryCallback(nullptr);
      outputTelemetryBufferSize = 0;
      warningText = nullptr;
      warningResult = 0;
      popupMenuItemsCount = 0;
      s_editMode = 0;
      menuEvent = 0;
      menuLevel = 0;
      menuHandlers[0] = menuMainView;
    }

    void TearDown() override
    {
      registerCrossfireTelemetryCallback(nullptr);
      outputTelemetryBufferSize = 0;
      warningText = nullptr;
      s_editMode = 0;
      menuEvent = 0;
      menuLevel = 0;
      menuHandlers[0] = menuMainView;
    }

    bool running() const { return menuLevel == 1 && menuHandlers[1] == elrsBindRun; }

    // One GUI frame. Returns the CRSF frame the tool pushed as "type: payload", or "".
    // The one slot output buffer is emptied as the pulses do.
    std::string frame(event_t event = 0)
    {
      guiMain(event);
      std::string sent;
      lastFrameSize = outputTelemetryBufferSize;
      if (lastFrameSize) {
        const uint8_t * b = outputTelemetryBuffer;
        EXPECT_LE(lastFrameSize, sizeof(outputTelemetryBuffer));
        EXPECT_EQ(b[0], CSRF_ADDRESS_TX);
        EXPECT_EQ(b[1] + 2, lastFrameSize);
        EXPECT_EQ(b[lastFrameSize - 1], crc8(&b[2], lastFrameSize - 3));
        char tmp[8];
        snprintf(tmp, sizeof(tmp), "%02X:", b[2]);
        sent = tmp;
        for (int i = 3; i < lastFrameSize - 1; i++) {
          snprintf(tmp, sizeof(tmp), " %02X", b[i]);
          sent += tmp;
        }
        outputTelemetryBufferSize = 0;
      }
      return sent;
    }

    // n frames; the last frame that was pushed
    std::string frames(int n)
    {
      std::string sent;
      while (n--) {
        std::string s = frame();
        if (!s.empty()) sent = s;
      }
      return sent;
    }

    // From the TOOLS menu: the first frame of the tool
    std::string open()
    {
      pushMenu(elrsBindRun);
      return frame();
    }

    // A frame from the module, through the firmware's CRSF parser
    void receive(uint8_t type, std::vector<uint8_t> payload)
    {
      uint8_t buf[64];
      buf[0] = 0xEA;
      buf[1] = payload.size() + 2;
      buf[2] = type;
      memcpy(&buf[3], payload.data(), payload.size());
      buf[3 + payload.size()] = crc8(&buf[2], payload.size() + 1);
      processCrossfireTelemetryFrame(buf, payload.size() + 4);
    }

    // MSP_ELRS_RXTX_CONFIG / UID answer of the device at origin
    void uidAnswer(uint8_t origin, std::vector<uint8_t> uid)
    {
      std::vector<uint8_t> payload = { CSRF_ADDRESS_RADIO, origin, 0x30, 7, 0x2D, 0x00 };
      payload.insert(payload.end(), uid.begin(), uid.end());
      payload.push_back(0x00); // MSP crc
      receive(0x7B, payload);
    }

    // Link statistics: the receiver is connected (RQly > 0) or not
    void link(bool up)
    {
      receive(LINK_ID, { 60, 60, (uint8_t)(up ? 100 : 0), 10, 0, 4, 3, 60, 100, 10 });
    }

    void seedHistory(std::vector<std::string> items)
    {
      memset(&eeprom[HIST_ADDR], 0, 32);
      eeprom[HIST_ADDR] = HIST_MAGIC;
      uint8_t * p = &eeprom[HIST_ADDR + 1];
      for (auto & item : items) {
        ASSERT_LE(p + item.size() + 1, &eeprom[HIST_ADDR + 32]);
        memcpy(p, item.c_str(), item.size());
        p += item.size() + 1;
      }
    }

    std::vector<std::string> history()
    {
      std::vector<std::string> items;
      if (eeprom[HIST_ADDR] != HIST_MAGIC) return items;
      const char * p = (const char *)&eeprom[HIST_ADDR + 1];
      const char * end = p + 31;
      while (p < end && *p) {
        size_t len = strnlen(p, end - p);
        items.push_back(std::string(p, len));
        p += len + 1;
      }
      return items;
    }

    // Is this text on the LCD, drawn at (x, y) with flags?
    bool shows(coord_t x, coord_t y, const char * text, LcdFlags flags = 0)
    {
      display_t tool[DISPLAY_BUFFER_SIZE];
      memcpy(tool, displayBuf, sizeof(tool));
      lcdClear();
      lcdDrawText(x, y, text, flags);
      coord_t left = (flags & RIGHT) ? lcdLastLeftPos : x;
      coord_t right = (flags & RIGHT) ? x : lcdLastRightPos;
      if (right > LCD_W) right = LCD_W;
      coord_t height = (FONTSIZE(flags) == SMLSIZE) ? 6 : (FONTSIZE(flags) == MIDSIZE) ? 10 : 7;
      bool same = (right > left);
      for (coord_t py = y; py < y + height; py++) {
        for (coord_t px = left; px < right; px++) {
          bool expected = displayBuf[(py / 8) * LCD_W + px] & (1 << (py % 8));
          bool actual = tool[(py / 8) * LCD_W + px] & (1 << (py % 8));
          if (expected != actual) same = false;
        }
      }
      memcpy(displayBuf, tool, sizeof(tool));
      return same;
    }

    // rows are 8 px apart, the first one at y = 11
    static coord_t row(int n) { return n * FH + FH + 3; }

    // phrase row -> the [Set] row, press it
    std::string set()
    {
      frame(DOWN);
      frame(DOWN);
      frame(DOWN);
      return frame(ENTER);
    }
};

} // namespace

TEST_F(ElrsBindTest, firstRunAsksForTheUid)
{
  // MSP_REQ, MSP_ELRS_RXTX_CONFIG (0x2D), subcommand UID, to the TX module
  EXPECT_EQ(open(), "7A: EE EA 30 01 2D 00");
  EXPECT_TRUE(running());
  frame();
  EXPECT_TRUE(shows(1, 1, "ExpressLRS Bind Tool", INVERS));
  EXPECT_TRUE(shows(LCD_W - 1, 1, "-", INVERS | RIGHT));
  EXPECT_TRUE(shows(0, row(0), "Phrase"));
  EXPECT_TRUE(shows(70, row(0), "---", INVERS));
  EXPECT_TRUE(shows(0, row(1), "Target"));
  EXPECT_TRUE(shows(70, row(1), "TX"));
  EXPECT_TRUE(shows(0, row(2), "Updating...", SMLSIZE));
  EXPECT_TRUE(shows(10, row(3), "[Set]", BOLD));
  EXPECT_TRUE(shows(0, row(4), "> History", BOLD));
  EXPECT_TRUE(shows(10, row(5), "[-- EXIT (" VERSION ") --]", BOLD));

  // opening the tool writes nothing
  EXPECT_EQ(eeprom[HIST_ADDR], 0xFF);

  uidAnswer(CSRF_ADDRESS_TX, { 13, 213, 105, 32, 0, 1 });
  link(true);
  frame();
  EXPECT_TRUE(shows(0, row(2), "TX: 13,213,105,32,0,1", SMLSIZE));
  EXPECT_TRUE(shows(LCD_W - 1, 1, "C", INVERS | RIGHT));
  EXPECT_EQ(frames(30), ""); // answered: no retry

  // shows() does tell texts and attributes apart
  EXPECT_FALSE(shows(0, row(2), "Updating...", SMLSIZE));
  EXPECT_FALSE(shows(0, row(2), "TX: 13,213,105,32,0,7", SMLSIZE));
  EXPECT_FALSE(shows(0, row(2), "TX: 13,213,105,32,0,1"));
  EXPECT_FALSE(shows(70, row(0), "---"));
  EXPECT_FALSE(shows(70, row(1), "TX", INVERS));
  EXPECT_FALSE(shows(70, row(1), "RX"));
  EXPECT_FALSE(shows(10, row(3), "[Set]"));
  EXPECT_FALSE(shows(LCD_W - 1, 1, "-", INVERS | RIGHT));
}

TEST_F(ElrsBindTest, givesUpAfter12Requests)
{
  int requests = (open() == "7A: EE EA 30 01 2D 00");
  for (int i = 0; i < 12; i++) {
    EXPECT_EQ(frames(9), "") << i;
    std::string sent = frame(); // every 10 frames, 0.5 s
    if (i < 11) {
      EXPECT_EQ(sent, "7A: EE EA 30 01 2D 00") << i;
      requests++;
    }
    else {
      EXPECT_EQ(sent, "");
    }
  }
  EXPECT_EQ(requests, 12);
  frame();
  EXPECT_TRUE(shows(0, row(2), "No response (needs ELRS 4.1+)", SMLSIZE));
  EXPECT_EQ(frames(40), "");

  // OK on the status row: again
  frame(DOWN);
  frame(DOWN);
  EXPECT_EQ(frame(ENTER), "7A: EE EA 30 01 2D 00");
}

TEST_F(ElrsBindTest, setWritesThePhraseAndRemembersIt)
{
  seedHistory({ "hello fpv" });
  open();
  uidAnswer(CSRF_ADDRESS_TX, { 1, 2, 3, 4, 5, 6 });
  frame();
  EXPECT_TRUE(shows(70, row(0), "hello fpv", INVERS)); // the newest one is the phrase

  // MSP_WRITE, MSP_ELRS_RXTX_CONFIG, subcommand BIND_PHRASE
  EXPECT_EQ(set(), "7C: EE EA 30 0A 2D 01 68 65 6C 6C 6F 20 66 70 76");
  frame();
  EXPECT_TRUE(shows(0, row(2), "Setting transmitter...", SMLSIZE));
  EXPECT_EQ(history(), std::vector<std::string>({ "hello fpv" }));

  // the UID is read again a second later
  EXPECT_EQ(frames(18), "");
  EXPECT_EQ(frames(2), "7A: EE EA 30 01 2D 00");
}

TEST_F(ElrsBindTest, fourToSixBytesAreAUid)
{
  struct { const char * phrase; const char * sent; } cases[] = {
    { "1,2,3,4",          "7C: EE EA 30 07 2D 00 00 00 01 02 03 04" },
    { "255,0,128,7,66,1", "7C: EE EA 30 07 2D 00 FF 00 80 07 42 01" },
    { " 9 , 8,7 ,006,5 ", "7C: EE EA 30 07 2D 00 00 09 08 07 06 05" },
    { "1,2,3,4,",         "7C: EE EA 30 07 2D 00 00 00 01 02 03 04" },
    { "1,2,3",            "7C: EE EA 30 06 2D 01 31 2C 32 2C 33" },
    { "1,2,3,256",        "7C: EE EA 30 0A 2D 01 31 2C 32 2C 33 2C 32 35 36" },
    { "1,2,3,4,5,6,7",    "7C: EE EA 30 0E 2D 01 31 2C 32 2C 33 2C 34 2C 35 2C 36 2C 37" },
    { "1,2,,4,5",         "7C: EE EA 30 09 2D 01 31 2C 32 2C 2C 34 2C 35" },
    { "1,2 3,4,5",        "7C: EE EA 30 0A 2D 01 31 2C 32 20 33 2C 34 2C 35" },
    { "1,2,3,4x",         "7C: EE EA 30 09 2D 01 31 2C 32 2C 33 2C 34 78" },
  };
  for (auto & c : cases) {
    SetUp();
    seedHistory({ c.phrase });
    open();
    EXPECT_EQ(set(), c.sent) << c.phrase;
    TearDown();
  }
}

TEST_F(ElrsBindTest, theLongestPhraseOfTheHistory)
{
  std::string phrase(STORED_MAX, 'w');
  seedHistory({ phrase });
  open();
  std::string sent = set();
  EXPECT_EQ(sent.substr(0, 21), "7C: EE EA 30 1F 2D 01");
  EXPECT_EQ(lastFrameSize, STORED_MAX + 10);
  EXPECT_EQ(history(), std::vector<std::string>({ phrase }));
}

TEST_F(ElrsBindTest, bothWritesTheReceiverThenTheModule)
{
  seedHistory({ "team 7" });
  open();
  link(true);
  frame(DOWN);
  frame(ENTER); // edit the target
  frame();
  frame(UP);
  frame(UP);
  frame(UP); // stays at Both
  frame(ENTER);
  frame();
  EXPECT_TRUE(shows(70, row(1), "Both", INVERS));

  frame(DOWN);
  frame(DOWN);
  EXPECT_EQ(frame(ENTER), "7C: EC EA 30 07 2D 01 74 65 61 6D 20 37");
  frame();
  EXPECT_TRUE(shows(0, row(2), "Setting RX and disconnecting...", SMLSIZE));
  EXPECT_EQ(frames(18), "");
  EXPECT_EQ(frames(2), "7C: EE EA 30 07 2D 01 74 65 61 6D 20 37");
  frame();
  EXPECT_TRUE(shows(0, row(2), "Setting transmitter...", SMLSIZE));
  EXPECT_TRUE(shows(70, row(1), "TX"));
  EXPECT_EQ(frames(20), "7A: EE EA 30 01 2D 00");
}

TEST_F(ElrsBindTest, bindAndUnbind)
{
  open();
  frame(DOWN);
  frame(ENTER);
  frame();
  frame(UP); // RX
  frame(EXIT);
  frame();
  EXPECT_TRUE(shows(70, row(1), "RX", INVERS));
  EXPECT_TRUE(shows(10, row(4), "[Bind]", BOLD));

  // no receiver: the module enters bind mode
  frame(DOWN);
  frame(DOWN);
  frame(DOWN);
  EXPECT_EQ(frame(ENTER), "32: EE EA 10 01");
  frame();
  EXPECT_TRUE(shows(0, row(2), "Sending bind command...", SMLSIZE));
  frames(20);
  EXPECT_TRUE(shows(0, row(2), "Sent", SMLSIZE));

  // a receiver: unbind it
  link(true);
  frame();
  EXPECT_TRUE(shows(10, row(4), "[Unbind]", BOLD | INVERS));
  EXPECT_EQ(frame(ENTER), "32: EC EA 10 01");
  frame();
  EXPECT_TRUE(shows(0, row(2), "Sending unbind to RX...", SMLSIZE));
}

TEST_F(ElrsBindTest, phraseEditor)
{
  open();
  EXPECT_EQ(frame(ENTER), ""); // phrase row: edit
  frame();
  frame(UP);           // a
  frame(ENTER_LONG);   // A
  frame(ENTER);
  frame(UP);
  frame(UP);           // b
  frame(ENTER);
  for (int i = 0; i < 28; i++) frame(UP); // 1
  frame(ENTER);        // a space ...
  frame(ENTER);
  frame(EXIT);         // ... that is stripped
  EXPECT_FALSE(s_editMode);
  frame();
  EXPECT_TRUE(shows(70, row(0), "Ab1", INVERS));
  EXPECT_EQ(set(), "7C: EE EA 30 04 2D 01 41 62 31");
  EXPECT_EQ(history(), std::vector<std::string>({ "Ab1" }));

  // a phrase ends at PHRASE_MAX chars: the longest frame, which is sent but, as it
  // does not fit the 31 bytes, not kept
  frame(UP);
  frame(UP);
  frame(UP);
  frame(ENTER);
  frame();
  size_t typed = 0;
  for (; typed < PHRASE_MAX + 5 && s_editMode; typed++) {
    frame(UP);
    frame(ENTER);
  }
  EXPECT_EQ(typed, PHRASE_MAX);
  EXPECT_FALSE(s_editMode);
  std::string sent = set();
  EXPECT_EQ(sent.substr(0, 21), "7C: EE EA 30 35 2D 01"); // 0x35: 1 + 52
  EXPECT_EQ(lastFrameSize, PHRASE_MAX + 10);
  EXPECT_LE(lastFrameSize, sizeof(outputTelemetryBuffer));
  EXPECT_EQ(history(), std::vector<std::string>({ "Ab1" }));
}

TEST_F(ElrsBindTest, aPhraseTooLongForTheHistoryIsSentButNotKept)
{
  // 30 chars fit
  seedHistory({ "x" });
  open();
  frame(ENTER);                                       // edit "x"
  frame();
  for (size_t i = 0; i < STORED_MAX; i++) {
    frame(UP);                                        // y, then a for each new space
    frame(ENTER);
  }
  frame(EXIT);                                        // the space under the cursor is stripped
  frame();
  set();
  EXPECT_EQ(lastFrameSize, STORED_MAX + 10);
  ASSERT_EQ(history().size(), 1u);                    // "x" had to go
  std::string kept = history()[0];
  EXPECT_EQ(kept.size(), STORED_MAX);

  // 31 do not: the history stays as it is
  frame(UP);
  frame(UP);
  frame(UP);
  frame(ENTER);
  frame();
  for (size_t i = 0; i < STORED_MAX; i++) frame(ENTER);
  frame(UP);
  frame(EXIT);
  frame();
  set();
  EXPECT_EQ(lastFrameSize, STORED_MAX + 1 + 10);
  EXPECT_EQ(history(), std::vector<std::string>({ kept }));
}

TEST_F(ElrsBindTest, historyKeepsTheNewestThatFit)
{
  // 16 + 15 of the 31 bytes
  seedHistory({ "aaaaaaaaaaaaaaa", "bbbbbbbbbbbbbb" });
  open();

  // > History, second entry: it becomes the phrase
  frame(DOWN);
  frame(DOWN);
  frame(DOWN);
  frame(DOWN);
  frame(ENTER);
  frame();
  EXPECT_TRUE(shows(0, row(0), "aaaaaaaaaaaaaaa", INVERS));
  EXPECT_TRUE(shows(0, row(1), "bbbbbbbbbbbbbb"));
  EXPECT_TRUE(shows(10, row(2), "[Clear All]", BOLD));
  EXPECT_TRUE(shows(10, row(3), "[---- BACK ----]", BOLD));
  frame(DOWN);
  frame(ENTER);
  frame();
  EXPECT_TRUE(shows(0, row(0), "Phrase"));
  EXPECT_TRUE(shows(70, row(0), "bbbbbbbbb", INVERS));

  // used again: to the front, nothing is lost
  set();
  EXPECT_EQ(history(), std::vector<std::string>({ "bbbbbbbbbbbbbb", "aaaaaaaaaaaaaaa" }));

  // a new one: the oldest no longer fits
  frame(UP);
  frame(UP);
  frame(UP);
  frame(ENTER);
  frame();
  frame(UP); // c
  frame(EXIT);
  set();
  EXPECT_EQ(history(), std::vector<std::string>({ "cbbbbbbbbbbbbb", "bbbbbbbbbbbbbb" }));
}

TEST_F(ElrsBindTest, historyIsFiveEntriesAtMost)
{
  seedHistory({ "e", "d", "c", "b", "a" });
  open();
  frame(ENTER);
  frame();
  frame(UP); // e -> f
  frame(EXIT);
  set();
  EXPECT_EQ(history(), std::vector<std::string>({ "f", "e", "d", "c", "b" }));
}

TEST_F(ElrsBindTest, deleteAnEntryAndClearAll)
{
  seedHistory({ "one", "two", "three" });
  open();
  frame(UP);
  frame(UP);
  frame(ENTER); // > History
  frame();
  frame(DOWN);  // "two"

  frame(ENTER_LONG);
  EXPECT_NE(warningText, nullptr);
  EXPECT_STREQ(warningText, "Delete entry?");
  frame(EXIT); // no
  frame();
  EXPECT_EQ(history().size(), 3u);

  frame(ENTER_LONG);
  frame(ENTER); // yes
  frames(2);
  EXPECT_EQ(history(), std::vector<std::string>({ "one", "three" }));
  EXPECT_TRUE(shows(0, row(1), "three", INVERS));

  frame(DOWN); // [Clear All]
  frame(ENTER);
  EXPECT_STREQ(warningText, "Clear history?");
  frame(ENTER);
  frames(3);
  EXPECT_TRUE(history().empty());
  EXPECT_EQ(eeprom[HIST_ADDR], HIST_MAGIC);
  EXPECT_TRUE(shows(10, row(0), "[Clear All]", BOLD));
  EXPECT_TRUE(shows(10, row(1), "[---- BACK ----]", BOLD | INVERS));
}

TEST_F(ElrsBindTest, foreignEepromBytesAreNoHistory)
{
  // not ours
  for (int i = 0; i < 32; i++) eeprom[HIST_ADDR + i] = 'a' + i % 26;
  open();
  frame();
  EXPECT_TRUE(shows(70, row(0), "---", INVERS));
  EXPECT_EQ(set(), ""); // no phrase: nothing to set
  TearDown();

  // ours, but without the end of the list: still no phrase longer than the limit
  SetUp();
  memset(&eeprom[HIST_ADDR], 'x', 32);
  eeprom[HIST_ADDR] = HIST_MAGIC;
  open();
  set();
  EXPECT_LE(lastFrameSize, sizeof(outputTelemetryBuffer));
  ASSERT_FALSE(history().empty());
  EXPECT_EQ(history()[0].size(), STORED_MAX);
}

TEST_F(ElrsBindTest, leavingStopsTheCallback)
{
  open();
  EXPECT_NE(crossfireTelemetryCallback, nullptr);
  frame(EXIT);
  EXPECT_FALSE(running());
  EXPECT_EQ(crossfireTelemetryCallback, nullptr);

  // the [EXIT] row
  menuEvent = 0;
  open();
  EXPECT_NE(crossfireTelemetryCallback, nullptr);
  frame(UP);
  frame(ENTER);
  EXPECT_FALSE(running());
  EXPECT_EQ(crossfireTelemetryCallback, nullptr);
}

TEST_F(ElrsBindTest, noCrossfireModule)
{
  g_model.moduleData[EXTERNAL_MODULE].type = MODULE_TYPE_NONE;
  EXPECT_EQ(open(), "");
  EXPECT_TRUE(shows(2, 0, " No ExpressLRS", MIDSIZE));
  EXPECT_TRUE(shows(2, 14, "Enable a CRSF External"));
  EXPECT_TRUE(shows(2, 22, "module in Model settings"));
  EXPECT_EQ(crossfireTelemetryCallback, nullptr);
  EXPECT_EQ(frames(30), "");
  frame(EXIT);
  EXPECT_FALSE(running());
}

#endif // PCBI6X_ELRS_BIND
