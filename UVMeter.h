/*-------------------------------------------------------------------------
    This source file is a part of UVMeter
    For the latest info, see https://github.com/cmarrin/UVMeter
    Copyright (c) 2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Board: ESP32 C3 Super Mini

// UVMeter is an ESP32 C3 Super Mini based UV meter, using an I2C OLED display
// and the Sparkfun VEML6075 UV sensor to display the UVA and UVB radiation hitting
// the sensor. It also displays time, date and weather because they
// are available from ESPlib.
//
// The hardware has a button, used to awake from sleep and reset the network.
//
// The OLED display is an I2C device at address 0x3c. The VEML6075 is
// an I2C device at address 0x10.
//
// Connections for the C3 Super Mini are as follows:
//
//      Pin  8 (GPIO8)      - SDA
//      Pin  9 (GPIO9)      - SCL
//      PIN  3 (GPIO3)      - Wake button (N/C button to ground with 10K resister to VCC)
//      LED_BUILTIN (GPIO8) - On board LED
//
// The system will be battery powered by a single 18650 battery connected
// through a charger and boost converter to provide 5V.
//
// After TimeToSleep seconds has elapsed ESP will turn off the display and 
// go into deep sleep. This consumes 200us, which should give over a year of 
// standby power with the 18650 battery.

#include "mil.h"
#include "Application.h"
#include "Clock.h"
#include "ButtonManager.h"

#ifdef ARDUINO
#include "SparkFun_VEML6075_Arduino_Library.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#else
#define SSD1306_SWITCHCAPVCC 0
#define SSD1306_WHITE 0
#define SSD1306_INVERSE 0

class Adafruit_SSD1306
{
public:
    Adafruit_SSD1306(uint8_t, uint8_t, void*, uint8_t) { }
    void begin(uint8_t, uint8_t) { }
    void clearDisplay() { }
    void display() { }
    void setTextSize(uint8_t) { }
    void setTextColor(uint16_t) { }
    void setTextColor(uint16_t, uint16_t) { }
    void setCursor(uint8_t, uint8_t) { }
    void write(char) { }
    void print(const char* s) { cout << "[[ " << s << " ]]\n"; }
    void setFont(void*) { }
    void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1,
                       int16_t *y1, uint16_t *w, uint16_t *h) { }
    int16_t width() { return 0; }
    void fillRect (int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { }
};

class VEML6075
{
public:
    bool begin() { return true; }
    float uva() { return 1.532; }
    float uvb() { return 2.145; }
};

#endif

static constexpr const char* ConfigPortalName = "MT UVMeter";
static constexpr const char* Hostname = "uvsensor";

static constexpr const char* ZipCode = "93405";
static constexpr uint8_t SelectButton = 0;

static constexpr uint8_t MessageOffset = 20;
static constexpr uint8_t MessageOffset2 = 40;
static constexpr uint8_t TimeDateOffset = 15;
static constexpr uint8_t TitleOffset = 37;
static constexpr uint8_t MainOffset = 60;

static constexpr uint8_t TimeToSleep = 30; // In seconds
static constexpr uint8_t WakeButton = 3;

class UVMeter : public mil::Application
{
public:
	UVMeter()
		: mil::Application(LED_BUILTIN, Hostname, ConfigPortalName)
		, _buttonManager([this](const mil::Button& b, mil::ButtonManager::Event e) { handleButtonEvent(b, e); })
    {    
        _clock = std::unique_ptr<mil::Clock>(new mil::Clock(this, ZipCode));
    }
	
	virtual void setup() override;
	virtual void loop() override;

private:
	virtual void showString(mil::Message m) override;
	virtual void showMain(bool force = false) override;
    virtual void showSecondary() override;
    
    void showString(const char* s, uint8_t size, uint8_t yOffset, bool center, bool invert = false);

    void handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event);
    
    uint16_t centerXOffset(const char* s) const;

    std::unique_ptr<mil::Clock> _clock;
    
	mil::ButtonManager _buttonManager;
    VEML6075 uv;
	Ticker _sleepTimer;
    
    bool _needDisplay = false;

};
