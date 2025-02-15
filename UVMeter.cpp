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
    _display.setTextColor(SSD1306_WHITE);
    Application::setup();
    if (_clock) {
        _clock->setup();
    }

    _buttonManager.addButton(mil::Button(SelectButton, SelectButton, false, mil::Button::PinMode::Float));

    if (!uv.begin()) {
        cout << "******** Failed to communicate with VEML6075 sensor, check wiring?";
    }
    
    // Setup sleep timer. When it fires, device will go to deep sleep
	_sleepTimer.once(TimeToSleep, [this]() {
        cout << "***** GOING TO SLEEP...\n";
        _display.ssd1306_command(SSD1306_DISPLAYOFF);
        esp_deep_sleep_enable_gpio_wakeup(1 << WakeButton, ESP_GPIO_WAKEUP_GPIO_LOW);
        esp_deep_sleep_start();
    });
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
    CPString s1, s2;
    uint8_t size = 0;
    bool center = false;

    _display.clearDisplay();
    
    switch(m) {
        case mil::Message::NetConfig:
            s1 = F("Config WiFi\nConnect to\n");
            s1 += ConfigPortalName;
            s1 += F("\npress [sel]\nto retry.");
            break;
        case mil::Message::Startup:
            s1 = F("UVMeter");
            s2 = F("v0.1");
            size = 2;
            center = true;
            break;
        case mil::Message::Connecting:
            s1 = F("Connecting...");
            size = 1;
            center = true;
            break;
        case mil::Message::NetFail:
            s1 = F("Network failed,\npress [select] to retry.");
            size = 0;
            break;
        case mil::Message::UpdateFail:
            s1 = F("Time or weather update failed,\npress [select] to retry.");
            size = 0;
            break;
        case mil::Message::AskRestart:
            s1 = F("Restart? (long press for yes)");
            size = 0;
            break;
        case mil::Message::AskResetNetwork:
            s1 = F("Reset network? (long press for yes)");
            size = 0;
            break;
        case mil::Message::VerifyResetNetwork:
            s1 = F("Are you sure? (long press for yes)");
            size = 0;
            break;
        default:
            s1 = F("Unknown string error");
            size = 0;
            break;
    }

    showString(s1.c_str(), size, MessageOffset, center);
    if (s2.c_str()[0] != '\0') {
        showString(s2.c_str(), size, MessageOffset2, center);
    }
    
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
    bool pm = false;

    if (hour == 0) {
        hour = 12;
    } else if (hour >= 12) {
        if (hour > 12) {
            pm = true;
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
    string += pm ? "pm" : "am";
    string += " ";
    
    // Add month and day
    string += ToString(timeinfo.tm_mon + 1 );
    string += "/";
    string += ToString(timeinfo.tm_mday);
       
    showString(string.c_str(), 1, TimeDateOffset, true);
    
    // Show UV header
    showString("uva  uvb", 1, TitleOffset, true, true);

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
    
    cout << "UVA=" << ToString(v1) << ", UVB=" << ToString(v2) << "\n";

    int i2 = int(v2);
    int d2 = int((v2 - i2) * 10);
    string = ToString(i1) + "." + ToString(d1) + " " + ToString(i2) + "." + ToString(d2);
    
    showString(string.c_str(), 2, MainOffset, true);
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

void UVMeter::showString(const char* s, uint8_t size, uint8_t yOffset, bool center, bool invert)
{
    _display.setTextSize(1);

    if (invert) {
        _display.fillRect(0, yOffset - 14, _display.width(), 17, SSD1306_WHITE);
    }
    
    _display.setTextColor(SSD1306_INVERSE);
    
    _display.setCursor(0, yOffset);
    
    switch (size) {
        default:    _display.setFont(&Font_8x8_8pt); break;
        case 1:     _display.setFont(&FreeSans9pt7b); break;
        case 2:     _display.setFont(&FreeSans12pt7b); break;
    }
    
    _display.setCursor(center ? centerXOffset(s) : 0, yOffset);
    _display.print(s);
}

uint16_t
UVMeter::centerXOffset(const char* s) const
{
	int16_t x1, y1;
	uint16_t w, h;
	_display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
 
    if (w > _display.width()) {
        return 0;
    }
    
	return(_display.width() - w) / 2;
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
