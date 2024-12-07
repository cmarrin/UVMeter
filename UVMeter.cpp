/*-------------------------------------------------------------------------
    This source file is a part of UVMeter
    For the latest info, see https://github.com/cmarrin/UVMeter
    Copyright (c) 2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "UVMeter.h"

#ifdef ARDUINO
#include "Font_8x8_8pt.h"
#include "Font_Compact_5pt.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#else
uint8_t Wire;
uint8_t Font_8x8_8pt = 0;
uint8_t Font_Compact_5pt = 0;
uint8_t FreeSans9pt7b = 0;
uint8_t FreeSans12pt7b = 0;

#endif

Adafruit_SSD1306 _display(128, 64, &Wire, -1);

void
UVMeter::setup()
{
    _display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
    _display.clearDisplay();
    _display.display();
    _display.setTextSize(1);
    _display.setTextColor(WHITE);
    Application::setup();
    if (_clock) {
        _clock->setup();
    }

    _buttonManager.addButton(mil::Button(SelectButton, SelectButton, false, mil::Button::PinMode::Pullup));

    if (!uv.begin()) {
        cout << "******** Failed to communicate with VEML6075 sensor, check wiring?";
  }
}   

void
UVMeter::loop()
{
    Application::loop();
    if (_clock) {
        _clock->loop();
    }
    
    // Sometimes the SSD1306 display is called from an alternate thread through
    // the Ticker. This can happen when showMain is called after the showDone
    // timer fires. It can also happen when a button is pressed and the debounce
    // timer fires. When the display() function is called it causes a crash.
    // The stack trace seems to indicate that this might be caused by allocating
    // or freeing string storage. This might only happen on the ESP8266 since
    // it uses some hackery to do timer events without having a legit RTOS. At
    // any rate setting a flag and calling display() from the main loop fixes
    // the problem.
    if (_needDisplay) {
        _display.display();
        _needDisplay = false;
    }
}   

void
UVMeter::showString(mil::Message m)
{
    CPString s;
    uint8_t size = 0;
    
    switch(m) {
        case mil::Message::NetConfig:
            s = F("Config WiFi\nConnect to\n");
            s += ConfigPortalName;
            s += F("\npress [sel]\nto retry.");
            break;
        case mil::Message::Startup:
            s = F("UVMeter\nv0.1");
            size = 2;
            break;
        case mil::Message::Connecting:
            s = F("Connecting...");
            size = 1;
            break;
        case mil::Message::NetFail:
            s = F("Network failed, press [select] to retry.");
            break;
        case mil::Message::UpdateFail:
            s = F("Time or weather update failed, press [select] to retry.");
            break;
        case mil::Message::AskRestart:
            s = F("Restart? (long press for yes)");
            break;
        case mil::Message::AskResetNetwork:
            s = F("Reset network? (long press for yes)");
            break;
        case mil::Message::VerifyResetNetwork:
            s = F("Are you sure? (long press for yes)");
            break;
        default:
            s = F("Unknown string error");
            break;
    }

    _display.clearDisplay();
    showString(s.c_str(), size, MessageOffset);
    if (isInCallback()) {
        _needDisplay = true;
    } else {
        _display.display();
    }
    
    startShowDoneTimer(2000);
}

void
UVMeter::showMain(bool force)
{
    _display.clearDisplay();

    CPString string;
    time_t t = _clock->currentTime();
	struct tm timeinfo;
    gmtime_r(&t, &timeinfo);
    uint8_t hour = timeinfo.tm_hour;

    if (hour == 0) {
        hour = 12;
    } else if (hour >= 12) {
        if (hour > 12) {
            hour -= 12;
        }
    }
    string += ToString(hour);
    string += ":";

    uint8_t minute = timeinfo.tm_min;
    if (minute < 10) {
        string += "0";
    }
    
    string += ToString(minute);
    showString(string.c_str(), 1, TimeDateOffset);
    
    // Now show the UV values. Print with 1 decimal digit
    float v1 = uv.uva();
    if (v1 < 0) {
        v1 = 0;
    }
    int i1 = int(v1);
    int d1 = int((v1 - i1) * 10);
    
    float v2 = uv.uvb();
    if (v2 < 0) {
        v2 = 0;
    }
    int i2 = int(v2);
    int d2 = int((v2 - i2) * 10);
    string = ToString(i1) + "." + ToString(d1) + " " + ToString(i2) + "." + ToString(d2);
    
    showString(string.c_str(), 2, MainOffset);
    if (isInCallback()) {
        _needDisplay = true;
    } else {
        _display.display();
    }
}

void
UVMeter::showSecondary()
{
}

void UVMeter::showString(const char* s, uint8_t size, uint8_t yOffset)
{
    _display.setTextSize(1);
    _display.setTextColor(WHITE);
    _display.setCursor(0, yOffset);
    
    switch (size) {
        default:    _display.setFont(&Font_8x8_8pt); break;
        case 1:     _display.setFont(&FreeSans9pt7b); break;
        case 2:     _display.setFont(&FreeSans12pt7b); break;
    }
        
    _display.print(s); 
}

void
UVMeter::handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event)
{
	if (button.id() == SelectButton) {
		if (event == mil::ButtonManager::Event::Click) {
			sendInput(mil::Input::Click, true);
		} else if (event == mil::ButtonManager::Event::LongPress) {
			sendInput(mil::Input::LongPress, true);
		}
	}
}
