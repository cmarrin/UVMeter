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

    setHaveUserQuestions(true, false);
    
    _buttonManager.addButton(mil::Button(SelectButton, SelectButton, false, mil::Button::PinMode::Float));

    _uvWorking = false;
    
    if (!_uv.begin()) {
        cout << "******** Failed to communicate with UV sensor, check wiring?\n";
    } else {
        if (!_uv.prepareMeasurement(MEAS_MODE_CMD)) {
            cout << "******** Failed to set UV measurement mode\n";
        } else {
            _uvWorking = true;
            updateUVValues();
            if (_uvWorking) {
                cout << "UV sensor connected. UVA=" << ToString(_uva) << ", UVB=" << ToString(_uvb) << "\n";
            }
        }
    }
    
    // Setup sleep timer. When it fires, device will go to deep sleep
	_sleepTimer.once_ms(TimeToSleep * 1000, [this]() {
        gotoSleep();
    });

    // Setup UV sensor to take a reading every second
//    _uvSampleTimer.attach_ms(UVSampleRate, [this]() {
//        updateUVValues();
//    });
}

void
UVMeter::gotoSleep()
{
    cout << "***** GOING TO SLEEP...\n";
    _display.ssd1306_command(SSD1306_DISPLAYOFF);
    esp_deep_sleep_enable_gpio_wakeup(1 << WakeButton, ESP_GPIO_WAKEUP_GPIO_LOW);
    esp_deep_sleep_start();
}

void
UVMeter::preUserAnswer()
{
    _display.clearDisplay();
    _display.display();
    delay(3000);
    gotoSleep();
}

void
UVMeter::loop()
{
    updateUVValues();

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
    FontSize size = FontSize::Compact;
    bool center = false;

    _display.clearDisplay();
    
    switch(m) {
        case mil::Message::AskPreUserQuestion:
            s1 = F("Turn Off?\n(long press for yes)");
            center = true;
            break;
        case mil::Message::NetConfig:
            s1 = F("Config WiFi\nConnect to\n");
            s1 += ConfigPortalName;
            s1 += F("\npress [sel]\nto retry.");
            break;
        case mil::Message::Startup:
            s1 = F("UVMeter");
            s2 = F("v0.1");
            size = FontSize::Medium;
            center = true;
            break;
        case mil::Message::Connecting:
            s1 = F("Connecting...");
            size = FontSize::Small;
            center = true;
            break;
        case mil::Message::NetFail:
            s1 = F("Network failed,\npress [select] to retry.");
            break;
        case mil::Message::UpdateFail:
            s1 = F("Time or weather update failed,\npress [select] to retry.");
            break;
        case mil::Message::AskRestart:
            s1 = F("Restart?\n(long press for yes)");
            break;
        case mil::Message::AskResetNetwork:
            s1 = F("Reset network?\n(long press for yes)");
            break;
        case mil::Message::VerifyResetNetwork:
            s1 = F("Are you sure?\n(long press for yes)");
            break;
        default:
            s1 = F("Unknown string error");
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
UVMeter::updateUVValues()
{
    if (_uvWorking) {
        if (kSTkErrOk != _uv.setStartState(true)) {
            cout << "Error starting UV read\n";
            _uvWorking = false;
            return;
        }

        delay(2 + _uv.getConversionTimeMillis());
        
         if (kSTkErrOk != _uv.readAllUV()) {
             cout << "Error reading UV sensor\n";
             _uvWorking = false;
             return;
         }

        _uva = _uv.getUVA();
        if (_uva < 0) {
            _uva = 0;
        }
        
        _uvb = _uv.getUVB();
        if (_uvb < 0) {
            _uvb = 0;
        }
    }
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
        pm = true;
        if (hour > 12) {
            hour -= 12;
        }
    }
    string = ToString(hour);
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
       
    showString(string.c_str(), FontSize::Compact, TimeDateOffset, true);
    
    // Show weather
    string = "cur:";
    string += ToString(_clock->currentTemp());
    string += " hi:";
    string += ToString(_clock->highTemp());
    string += " lo:";
    string += ToString(_clock->lowTemp());
    string += " ";
    string += _clock->weatherConditions();

    showString(string.c_str(), FontSize::Compact, WeatherOffset, true);
    
    // Show UV header
    showString("uva  uvb", FontSize::Small, UVHeaderOffset, true, true);

    if (_uvWorking) {
        // Now show the UV values. Print with 1 decimal digit
        int iuva = int(_uva);
        int duva = int((_uva - iuva) * 10);
        int iuvb = int(_uvb);
        int duvb = int((_uvb - iuvb) * 10);

        string = ToString(iuva) + "." + ToString(duva) + " " + ToString(iuvb) + "." + ToString(duvb);
    } else {
        string = "---- ----";
    }
    
    showString(string.c_str(), FontSize::Medium, UVValuesOffset, true);
    if (isInCallback()) {
        _needDisplay = true;
    } else {
        _display.display();
    }
}

void
UVMeter::showSecondary()
{
    cout << "***** Show Secondary\n";
}

void UVMeter::showString(const char* s, FontSize size, uint8_t yOffset, bool center, bool invert)
{
cout << "***** showString: \"" << s << "\", size=" << uint8_t(size) << ", offset=" << yOffset << "\n";

    _display.setTextSize(1);

    if (invert) {
        _display.fillRect(0, yOffset - 14, _display.width(), 17, SSD1306_WHITE);
    }
    
    _display.setTextColor(SSD1306_INVERSE);
    
    _display.setCursor(0, yOffset);
    
    switch (size) {
        case FontSize::Compact:   _display.setFont(&Font_Compact_5pt); break;
        case FontSize::Small:     _display.setFont(&FreeSans9pt7b); break;
        case FontSize::Medium:    _display.setFont(&FreeSans12pt7b); break;
        case FontSize::Large:     _display.setFont(&Font_8x8_8pt); break;
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
