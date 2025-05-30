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
// and the Sparkfun AS7331 UV sensor to display the UVA and UVB radiation hitting
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
//      PIN  3 (GPIO3)      - Wake button (N/O button to ground with 10K resister to VCC)
//      LED_BUILTIN (GPIO8) - On board LED
//
// The system will be battery powered by a single 18650 battery connected
// through a charger and boost converter to provide 5V.
//
// After TimeToSleep seconds has elapsed ESP will turn off the display and 
// go into deep sleep. This consumes 200us, which should give over a year of 
// standby power with the 18650 battery.
//
// Hardware hookup:
//
//      C3 (5v)         - Charger (V0+)
//      C3 (G)          - Charger (V0-) - 6075 (Gnd) - Disp (Gnd) - Switch (A)
//      C3 (3.3)        - 6075 (3.3v)   - Disp (Vcc) - 10kΩ (A)
//      C3 (3)          - Switch (B)    - 10kΩ (B)
//      C3 (8)          - 6075 (SDA)    - Disp (SDA)
//      C3 (9)          - 6075 (SCL)    - Disp (SCL)
//      Charger (B+)    - Battery (+)
//      Charger (B-)    - Battery (-)

#include "mil.h"
#include "Application.h"
#include "Clock.h"
#include "ButtonManager.h"

#define DEBUG_UV

#ifdef ARDUINO
#include <Wire.h>
#include <SparkFun_AS7331.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#else
#define SSD1306_SWITCHCAPVCC 0
#define SSD1306_WHITE 0
#define SSD1306_INVERSE 0
#define SSD1306_DISPLAYOFF 0
#define ESP_GPIO_WAKEUP_GPIO_LOW 0
#define MEAS_MODE_CMD 0
#define kSTkErrOk 0

static inline void esp_deep_sleep_enable_gpio_wakeup(uint8_t, uint8_t) { }
static inline void esp_deep_sleep_start() { }

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
    void ssd1306_command(uint8_t) { }
};

class SfeAS7331ArdI2C
{
public:
    bool begin() { return true; }
    float getUVA() { return 1.532; }
    float getUVB() { return 2.145; }
    bool prepareMeasurement(uint8_t) { return true; }
    uint8_t setStartState(bool) { return 0; }
    uint16_t getConversionTimeMillis() { return 0; }
    uint8_t readAllUV() { return 0; }
};

#endif

static constexpr const char* ConfigPortalName = "MT UVMeter";
static constexpr const char* Hostname = "uvsensor";

static constexpr const char* ZipCode = "93405";
static constexpr uint8_t SelectButton = 3;

static constexpr uint8_t MessageOffset = 20;    // For network messages, etc.
static constexpr uint8_t MessageOffset2 = 40;   // Second line for messages
static constexpr uint8_t TimeDateOffset = 10;
static constexpr uint8_t WeatherOffset = 20;
static constexpr uint8_t UVHeaderOffset = 37;
static constexpr uint8_t UVValuesOffset = 60;

static constexpr uint32_t TimeToSleep = 5 * 60; // In seconds
static constexpr uint8_t WakeButton = 3;

static constexpr uint32_t UVSampleRate = 1000; // In ms

class UVMeter : public mil::Application
{
public:
    enum class FontSize { Compact, Small, Medium, Large };
    
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

    virtual void preUserAnswer() override;
    
    void gotoSleep();
    
    void showString(const char* s, FontSize, uint8_t yOffset, bool center, bool invert = false);

    void handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event);
    
    uint16_t centerXOffset(const char* s) const;
    
    void updateUVValues();
    float uva() const { return _uva; }
    float uvb() const { return _uvb; }

    std::unique_ptr<mil::Clock> _clock;
    
	mil::ButtonManager _buttonManager;
    SfeAS7331ArdI2C _uv;
    float _uva = 0;
    float _uvb = 0;
    
	Ticker _sleepTimer;
	Ticker _uvSampleTimer;
    
    bool _needDisplay = false;
    bool _uvWorking = false;

};
