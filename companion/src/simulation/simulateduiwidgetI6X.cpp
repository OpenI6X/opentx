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

// NOTE: RadioUiAction(NUMBER,...): NUMBER relates to enum EnumKeys in radio/src/targets/flysky/board.h
// Artwork: images/simulator/I6X/generate_i6x_skin.py (button rectangles must match it)

#include "simulateduiwidget.h"
#include "ui_simulateduiwidgetI6X.h"

SimulatedUIWidgetI6X::SimulatedUIWidgetI6X(SimulatorInterface * simulator, QWidget * parent):
  SimulatedUIWidget(simulator, parent),
  ui(new Ui::SimulatedUIWidgetI6X)
{
  RadioUiAction * act;

  ui->setupUi(this);

  // add actions in order of appearance on the help menu

  act = new RadioUiAction(3, QList<int>() << Qt::Key_Up << Qt::Key_PageUp, SIMU_STR_HLP_KEYS_GO_UP % "|" % SIMU_STR_HLP_MOUSE_UP, SIMU_STR_HLP_ACT_UP);
  addRadioWidget(ui->leftbuttons->addArea(QRect(18, 34, 85, 43), "I6X/i6x_left_up.png", act));

  act = new RadioUiAction(2, QList<int>() << Qt::Key_Down << Qt::Key_PageDown, SIMU_STR_HLP_KEYS_GO_DN % "|" % SIMU_STR_HLP_MOUSE_DN, SIMU_STR_HLP_ACT_DN);
  addRadioWidget(ui->leftbuttons->addArea(QRect(18, 92, 85, 43), "I6X/i6x_left_down.png", act));

  act = new RadioUiAction(0, QList<int>() << Qt::Key_Enter << Qt::Key_Return, SIMU_STR_HLP_KEY_ENTER % "|" % SIMU_STR_HLP_MOUSE_MID, SIMU_STR_HLP_ACT_MENU);
  addRadioWidget(ui->rightbuttons->addArea(QRect(18, 34, 85, 43), "I6X/i6x_right_ok.png", act));
  m_mouseMidClickAction = act;

  act = new RadioUiAction(1, QList<int>() << Qt::Key_Delete << Qt::Key_Escape << Qt::Key_Backspace, SIMU_STR_HLP_KEYS_EXIT, SIMU_STR_HLP_ACT_EXIT);
  addRadioWidget(ui->rightbuttons->addArea(QRect(18, 92, 85, 43), "I6X/i6x_right_cancel.png", act));

  // BIND key: the firmware reports it as KEY_RIGHT
  act = new RadioUiAction(4, QList<int>() << Qt::Key_Right << Qt::Key_B, SIMU_STR_HLP_KEY_RGT % "|B", tr("BIND"));
  addRadioWidget(ui->leftbuttons->addArea(QRect(14, 152, 57, 25), "I6X/i6x_left_bind.png", act));

  addRadioWidget(ui->leftbuttons->addArea(QRect(82, 150, 27, 27), "I6X/i6x_left_photo.png", m_screenshotAction));

  m_backlightColors << QColor(200, 214, 225);  // i6X blue-white
  m_backlightColors << QColor(166, 247, 159);
  m_backlightColors << QColor(247, 159, 166);
  m_backlightColors << QColor(255, 195, 151);
  m_backlightColors << QColor(247, 242, 159);

  setLcd(ui->lcd);
}

SimulatedUIWidgetI6X::~SimulatedUIWidgetI6X()
{
  delete ui;
}

void SimulatedUIWidgetI6X::setLightOn(bool enable)
{
  // The frame has no backlight variants; the LCD background colour shows the backlight.
}
