#!/usr/bin/env python3
# Generates the FlySky i6X simulator skin (original artwork, GPLv2 like the rest of Companion).
# Layout follows the radio's front: UP/DOWN left of the LCD, OK/CANCEL right, BIND key below.
# Usage: python generate_i6x_skin.py   (needs Pillow; writes the PNGs next to this script)
#
# Geometry must match simulateduiwidgetI6X.cpp / .ui:
#   left buttons 120x186 | frame left 24x186 | top 256x29, LCD 256x128, bottom 256x29 |
#   frame right 24x186 | right buttons 120x186

import os
from PIL import Image, ImageDraw, ImageFont

OUT = os.path.dirname(os.path.abspath(__file__))
H = 186
PANEL_W = 120
FRAME_W = 24
LCD_W, LCD_H = 256, 128
STRIP_H = 29

BODY = (38, 40, 43)
BODY_EDGE = (58, 61, 65)
BEZEL = (14, 15, 16)
KEY = (72, 75, 80)
KEY_EDGE = (20, 21, 23)
KEY_TEXT = (225, 228, 232)
PRESSED = (120, 126, 134)
ACCENT = (240, 150, 40)
SUBTLE = (120, 124, 130)

# Button rectangles (x0, y0, x1, y1) - keep in sync with simulateduiwidgetI6X.cpp
KEY_UP = (18, 34, 102, 76)
KEY_DOWN = (18, 92, 102, 134)
KEY_BIND = (14, 152, 70, 176)
KEY_PHOTO = (82, 150, 108, 176)
KEY_OK = (18, 34, 102, 76)
KEY_CANCEL = (18, 92, 102, 134)


def font(size):
    try:
        return ImageFont.load_default(size=size)
    except TypeError:  # Pillow < 10.1
        return ImageFont.load_default()


def body(w, h):
    img = Image.new("RGB", (w, h), BODY)
    d = ImageDraw.Draw(img)
    d.line([(0, 0), (w - 1, 0)], fill=BODY_EDGE)
    d.line([(0, h - 1), (w - 1, h - 1)], fill=KEY_EDGE)
    return img


def key(d, rect, label, pressed=False, size=14, radius=9):
    d.rounded_rectangle(rect, radius=radius, fill=PRESSED if pressed else KEY,
                        outline=ACCENT if pressed else KEY_EDGE, width=2)
    if label:
        f = font(size)
        cx, cy = (rect[0] + rect[2]) / 2, (rect[1] + rect[3]) / 2
        d.text((cx, cy), label, fill=KEY_TEXT, font=f, anchor="mm")


def camera(d, rect, pressed=False):
    d.rounded_rectangle(rect, radius=5, fill=PRESSED if pressed else KEY,
                        outline=ACCENT if pressed else KEY_EDGE, width=2)
    x0, y0, x1, y1 = rect
    d.rounded_rectangle((x0 + 5, y0 + 9, x1 - 5, y1 - 6), radius=2, outline=KEY_TEXT, width=2)
    d.rectangle((x0 + 10, y0 + 6, x0 + 16, y0 + 9), fill=KEY_TEXT)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2 + 2
    d.ellipse((cx - 4, cy - 4, cx + 4, cy + 4), outline=KEY_TEXT, width=2)


def left_panel(pressed=None):
    img = body(PANEL_W, H)
    d = ImageDraw.Draw(img)
    key(d, KEY_UP, "UP", pressed == "up")
    key(d, KEY_DOWN, "DOWN", pressed == "down")
    key(d, KEY_BIND, "BIND", pressed == "bind", size=11, radius=7)
    camera(d, KEY_PHOTO, pressed == "photo")
    return img


def right_panel(pressed=None):
    img = body(PANEL_W, H)
    d = ImageDraw.Draw(img)
    key(d, KEY_OK, "OK", pressed == "ok")
    key(d, KEY_CANCEL, "CANCEL", pressed == "cancel", size=13)
    d.text((PANEL_W / 2, 164), "OpenI6X", fill=SUBTLE, font=font(12), anchor="mm")
    return img


def frame_side(left):
    img = body(FRAME_W, H)
    d = ImageDraw.Draw(img)
    # bezel band hugging the LCD (rows STRIP_H-6 .. STRIP_H+LCD_H+6)
    x = FRAME_W - 8 if left else 0
    d.rectangle((x, STRIP_H - 6, x + 7, STRIP_H + LCD_H + 5), fill=BEZEL)
    return img


def frame_strip(top):
    img = body(LCD_W, STRIP_H)
    d = ImageDraw.Draw(img)
    if top:
        d.rectangle((0, STRIP_H - 6, LCD_W - 1, STRIP_H - 1), fill=BEZEL)
        d.text((LCD_W / 2, 10), "FS-i6X  OpenTX", fill=SUBTLE, font=font(12), anchor="mm")
    else:
        d.rectangle((0, 0, LCD_W - 1, 5), fill=BEZEL)
    return img


def save(img, name):
    img.save(os.path.join(OUT, name), optimize=True)


save(left_panel(), "i6x_left.png")
for k in ("up", "down", "bind", "photo"):
    save(left_panel(k), "i6x_left_%s.png" % k)
save(right_panel(), "i6x_right.png")
for k in ("ok", "cancel"):
    save(right_panel(k), "i6x_right_%s.png" % k)
save(frame_side(True), "i6x_frame_left.png")
save(frame_side(False), "i6x_frame_right.png")
save(frame_strip(True), "i6x_frame_top.png")
save(frame_strip(False), "i6x_frame_bottom.png")
print("written to", OUT)
