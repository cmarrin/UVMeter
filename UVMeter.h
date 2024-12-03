/*-------------------------------------------------------------------------
    This source file is a part of UVMeter
    For the latest info, see https://github.com/cmarrin/UVMeter
    Copyright (c) 2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// UVMeter uses a Wemos D1 Mini connected to a Wemos OLED shield and a
// Sparkfun VEML6075 UV sensor to display the UVA and UVB radiation hitting
// the sensor. It also displays time, date and weather because they
// are available from ESPlib.
//
// The Wemos display is an I2C device at address 0x3c. The VEML6075 is
// an I2C device at address 0x10. SDA and SCK at at the standard pins
// of D1 and D2.
//
// The system will be battery powered by a single 18650 battery connected
// through a charger and boost converter to provide 5V.
//
// I will try to leave the system powered up and in deep sleep until a
// button is pressed to wake it up. Deep sleep consumes 20us, which should
// give several years of standby power.
 
#include "mil.h"
#include "Application.h"
#include "Clock.h"

#ifdef ARDUINO
#include "SparkFun_VEML6075_Arduino_Library.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#else
#define SSD1306_SWITCHCAPVCC 0
#define WHITE 0

class Adafruit_SSD1306
{
public:
    Adafruit_SSD1306(uint8_t) { }
    void begin(uint8_t, uint8_t) { }
    void clearDisplay() { }
    void display() { }
    void setTextSize(uint8_t) { }
    void setTextColor(uint8_t) { }
    void setCursor(uint8_t, uint8_t) { }
    void write(char) { }
    void print(const char* s) { cout << "[[ " << s << " ]]\n"; }
    void setFont(void*) { }
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

static constexpr uint8_t MessageLine = 0;
static constexpr uint8_t TimeDateLine = 0;
static constexpr uint8_t TitleLine = 10;
static constexpr uint8_t MainLine = 20;

class UVMeter : public mil::Application
{
public:
	UVMeter()
		: mil::Application(LED_BUILTIN, Hostname, ConfigPortalName)
    {    
        _clock = std::unique_ptr<mil::Clock>(new mil::Clock(this, ZipCode));
    }
	
	virtual void setup() override;

	virtual void loop() override
    {
        Application::loop();
        if (_clock) {
            _clock->loop();
        }
    }   

private:
	virtual void showString(mil::Message m) override;
	virtual void showMain(bool force = false) override;
    virtual void showSecondary() override;
    
    void showString(const char* s, uint8_t line);

    std::unique_ptr<mil::Clock> _clock;
    
    VEML6075 uv;

};
