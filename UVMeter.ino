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
// Board: ESP32 C3 Super Mini
// See circuit details in UVMeter.h

#include "UVMeter.h"

UVMeter uvmeter;

void setup()
{
    Wire.begin();
    Serial.begin(115200);
    cout << "UVMeter v0.1\n";
	uvmeter.setup();
}

void loop()
{
	uvmeter.loop();
}

