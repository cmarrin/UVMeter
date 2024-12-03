/*-------------------------------------------------------------------------
    This source file is a part of UVMeter
    For the latest info, see https://github.com/cmarrin/UVMeter
    Copyright (c) 2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Arduino IDE Setup:
//
// Board: LOLIN(WEMOS) D1 R2 & mini

// UVMeter is an ESP8266 (NodeMCU) based UV meter, using the I2C Wemos display shield
// and the Sparkfun VEML6075 UV sensor.
//
// The hardware has a button, used to awake from sleep and reset the network

// Connections to dsp7s04b board:
//
//   1 - X
//   2 - X
//   3 - X
//   4 - X
//   5 - X
//   6 - X
//   7 - Gnd
//   8 - SCL
//   9 - SDA
//  10 - 3.3v
//
// Ports
//
//      D0 - GPIO16 (Do Not Use)
//      D1 - SCL
//      D2 - SDA
//      D3 - N/O Button (This is the Flash switch, as long as it's high on boot you're OK)
//      D4 - On board LED
//      D5 - X
//      D6 - X
//      D7 - X
//      D8 - X

#include "UVMeter.h"

UVMeter uvmeter;

void setup()
{
    Wire.begin();
    Serial.begin(115200);
	uvmeter.setup();
}

void loop()
{
	uvmeter.loop();
}

