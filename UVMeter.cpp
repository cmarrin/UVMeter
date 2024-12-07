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

static uint8_t Font_8x8_8pt_yAdvance = Font_8x8_8pt.yAdvance;
static uint8_t Font_Compact_5pt_yAdvance = Font_Compact_5pt.yAdvance;
#else
uint8_t Wire;
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
    switch(m) {
        case mil::Message::NetConfig:
            s = F("\aConfig WiFi\nConnect to\n");
            s += ConfigPortalName;
            s += F("\npress [sel]\nto retry.");
            break;
        case mil::Message::Startup:
            s = F("UVMeter\nv0.1");
            break;
        case mil::Message::Connecting:
            s = F("\aConnecting...");
            break;
        case mil::Message::NetFail:
            s = F("\aNetwork failed, press [select] to retry.");
            break;
        case mil::Message::UpdateFail:
            s = F("\aTime or weather update failed, press [select] to retry.");
            break;
        case mil::Message::AskRestart:
            s = F("\aRestart? (long press for yes)");
            break;
        case mil::Message::AskResetNetwork:
            s = F("\aReset network? (long press for yes)");
            break;
        case mil::Message::VerifyResetNetwork:
            s = F("\aAre you sure? (long press for yes)");
            break;
        default:
            s = F("Unknown string error");
            break;
    }

    _display.clearDisplay();
    showString(s.c_str(), MessageLine);
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
    showString(string.c_str(), TimeDateLine);
    
    // Now show the UV values. Print with 1 decimal digit
    float v1 = uv.uva();
    if (v1 < 0) {
        v1 = 0;
    }
    int i1 = int(v1);
    int d1 = int((v1 - i1) * 10);
    float v2 = uv.uvb();
    if (v1 < 0) {
        v1 = 0;
    }
    int i2 = int(v2);
    int d2 = int((v2 - i2) * 10);
    string = ToString(i1) + "." + ToString(d1) + " " + ToString(i2) + "." + ToString(d2);
    
    showString(string.c_str(), MainLine);
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

void UVMeter::showString(const char* s, uint8_t line)
{
    _display.setTextSize(1);
    _display.setTextColor(WHITE);
    
    if (*s == '\a') {
        _display.setFont(&Font_Compact_5pt);
        _display.setCursor(0, line + Font_Compact_5pt_yAdvance);
        s += 1;
    } else {
        _display.setFont(&Font_8x8_8pt);
        _display.setCursor(0, line + Font_8x8_8pt_yAdvance);
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
