

void crsfNotEnabledPopup(event_t event);

// Edit a string in place and draw it on the main page (9 chars fit a row).
//  - `str` is the string, `cur` the cursor (in/out), `event` the firmware key
//    event (used when `editing`), `maxLen` the string capacity;
//  - `x, y` is the top left of the drawing, `flags` the drawing flags of the
//    string; an empty string is drawn as "---" when not editing.
// Returns true when editing ended (CANCEL, or OK at the last character).
bool editString(char * str, uint8_t * cur, event_t event, uint32_t maxLen,
                coord_t x, coord_t y, LcdFlags flags, bool editing);
