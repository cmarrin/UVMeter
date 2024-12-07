/*-------------------------------------------------------------------------
    This source file is a part of UVMeter
    For the latest info, see https://github.com/cmarrin/UVMeter
    Copyright (c) 2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "UVMeter.h"

#include <sys/ioctl.h>

static int getc()
{
    int bytes = 0;
    if (ioctl(0, FIONREAD, &bytes) == -1) {
        return -1;
    }
    if (bytes == 0) {
        return 0;
    }
    return getchar();
}

int main(int argc, const char * argv[])
{
    system("stty raw");

    UVMeter uvmeter;
    
    uvmeter.setup();
    
    while (true) {
        uvmeter.loop();

        int c = getc();
        if (c == '1') {
            cout << " Got Click\n";
            uvmeter.sendInput(mil::Input::Click, false);
        } else if (c == '2') {
            cout << " Got Long Press\n";
            uvmeter.sendInput(mil::Input::LongPress, false);
        }
    }
    return 0;
}
