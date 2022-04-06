/*
Copyright 2022 Nep Nep

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to 
whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the 
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <math.h>
#include <memory.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#define ERR_ARG -1
#define ERR_SETUP 1
#define ERR_INPUT 2
#define ERR_UINPUT 3

#define DELTA_STICK 64
#define DELTA_TRIGGERS 16

#define ABS_LIMIT_TRIGGERS 128
#define ABS_LIMIT_STICK 512

static double getTimeWithMillis() {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec + floor(time.tv_nsec / 1.0e6) / 1000.0;
}

static void printErr(const char *err) {
    fputs(err, stderr);
}

static bool setupAbs(int fd, unsigned short channel, int min, int max) {
    if (ioctl(fd, UI_SET_ABSBIT, channel)) {
        printErr("Failed to set up joystick bit");
        return true;
    }

    struct uinput_abs_setup str = {
        .code = channel,
        .absinfo = { .minimum = min, .maximum = max } 
    };

    if (ioctl(fd, UI_ABS_SETUP, &str)) {
        printErr("Failed to set up joystick");
        return true;
    }
    return false;
}

static void processThumbStick(int *x, int *y, bool isXPosPressed, bool isXNegPressed, bool isYPosPressed, bool isYNegPressed) {
    if (isXNegPressed) {
        if (*x > -ABS_LIMIT_STICK) {
            *x -= DELTA_STICK;
        }
    } else if (isXPosPressed) {
        if (*x < ABS_LIMIT_STICK) {
            *x += DELTA_STICK;
        }
    }

    if (isYPosPressed) {
        if (*y > -ABS_LIMIT_STICK) {
            *y -= DELTA_STICK;
        }
    } else if (isYNegPressed) {
        if  (*y < ABS_LIMIT_STICK) {
            *y += DELTA_STICK;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printErr("Invalid arguments");
        return ERR_ARG;
    }

    char *dev = argv[1];

    int uinput = open("/dev/uinput", O_WRONLY);

    if (uinput < 0) {
        printErr("Failed to open uinput");
        return ERR_SETUP;
    }

    bool failed;
    failed = ioctl(uinput, UI_SET_EVBIT, EV_KEY);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_A);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_B);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_X);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_Y);

    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_DPAD_LEFT);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_DPAD_UP);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_DPAD_DOWN);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_DPAD_RIGHT);

    
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_THUMBL);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_THUMBR);

    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_TL);
    failed = failed || ioctl(uinput, UI_SET_KEYBIT, BTN_TR);

    failed = failed || ioctl(uinput, UI_SET_EVBIT, EV_ABS);

    failed = failed || setupAbs(uinput, ABS_X, -ABS_LIMIT_STICK, ABS_LIMIT_STICK);
    failed = failed || setupAbs(uinput, ABS_Y, -ABS_LIMIT_STICK, ABS_LIMIT_STICK);
    failed = failed || setupAbs(uinput, ABS_RX, -ABS_LIMIT_STICK, ABS_LIMIT_STICK);
    failed = failed || setupAbs(uinput, ABS_RY, -ABS_LIMIT_STICK, ABS_LIMIT_STICK);

    failed = failed || setupAbs(uinput, ABS_HAT2Y, -ABS_LIMIT_TRIGGERS, ABS_LIMIT_TRIGGERS);
    failed = failed || setupAbs(uinput, ABS_HAT2X, -ABS_LIMIT_TRIGGERS, ABS_LIMIT_TRIGGERS);

    struct uinput_setup setupStr = {
        .name = "ControllerEmu Joystick",
        .id = {
            .bustype = BUS_USB,
            .vendor = 0x1,
            .product = 0x1,
            .version = 2
        }
    };

    failed = failed || ioctl(uinput, UI_DEV_SETUP, &setupStr);
    failed = failed || ioctl(uinput, UI_DEV_CREATE, &setupStr);

    if (failed) {
        close(uinput);
        printErr("Failed to initialize");
        return ERR_SETUP;
    }

    int devFD = open(dev, O_RDONLY);

    if (devFD < 0) {
        printErr("Failed to open input device");
        ioctl(uinput, UI_DEV_DESTROY);
        close(uinput);
        return ERR_INPUT;
    }

    bool isAPressed = false;
    bool isBPressed = false;
    bool isXPressed = false;
    bool isYPressed = false;

    bool isLeftPressed = false;
    bool isUpPressed = false;
    bool isDownPressed = false;
    bool isRightPressed = false;

    bool isIPressed = false;
    bool isJPressed = false;
    bool isKPressed = false;
    bool isLPressed = false;

    bool isQPressed = false;
    bool isEPressed = false;

    bool isGPressed = false;
    bool isHPressed = false;

    bool isTPressed = false;
    bool isUPressed = false;

    bool isNP8Pressed = false;
    bool isNP4Pressed = false;
    bool isNP5Pressed = false;
    bool isNP6Pressed = false;

    double lastTime = getTimeWithMillis();

    int x = 0;    
    int y = 0;

    int rX = 0;
    int rY = 0;

    int leftTrigger = 0;
    int rightTrigger = 0;
    while (true) {
        struct input_event event;
        if (read(devFD, &event, sizeof(event)) < 0) {
            printErr("Failed to read event from input");
            ioctl(uinput, UI_DEV_DESTROY);
            close(uinput);
            close(devFD);
            return ERR_INPUT;
        }
        int isPressed = event.value;
        unsigned short key = event.code;
        switch (key) {
            case KEY_A:
                isAPressed = isPressed;
                break;
            case KEY_B:
                isBPressed = isPressed;
                break;
            case KEY_X:
                isXPressed = isPressed;
                break;
            case KEY_Y:
                isYPressed = isPressed;
                break;
            case KEY_DELETE:
                isLeftPressed = isPressed;
                break;
            case KEY_HOME:
                isUpPressed = isPressed;
                break;
            case KEY_END:
                isDownPressed = isPressed;
                break;
            case KEY_PAGEDOWN:
                isRightPressed = isPressed;
                break;
            case KEY_I:
                isIPressed = isPressed;
                break;
            case KEY_J:
                isJPressed = isPressed;
                break;
            case KEY_K:
                isKPressed = isPressed;
                break;
            case KEY_L:
                isLPressed = isPressed;
                break;
            case KEY_Q:
                isQPressed = isPressed;
                break;
            case KEY_E:
                isEPressed = isPressed;
                break;
            case KEY_G:
                isGPressed = isPressed;
                break;
            case KEY_H:
                isHPressed = isPressed;
                break;
            case KEY_T:
                isTPressed = isPressed;
                break;
            case KEY_U:
                isUPressed = isPressed;
                break;
            case KEY_KP8:
                isNP8Pressed = isPressed;
                break;
            case KEY_KP4:
                isNP4Pressed = isPressed;
                break;
            case KEY_KP5:
                isNP5Pressed = isPressed;
                break;
            case KEY_KP6:
                isNP6Pressed = isPressed;
        }
        double newTime = getTimeWithMillis();
        if (newTime - lastTime >= 0.125) {
            lastTime = newTime;
            
            processThumbStick(&x, &y, isLPressed, isJPressed, isIPressed, isKPressed);
            processThumbStick(&rX, &rY, isNP6Pressed, isNP4Pressed, isNP8Pressed, isNP5Pressed);

            if (isQPressed) {
                if (leftTrigger < ABS_LIMIT_TRIGGERS) {
                    leftTrigger += DELTA_TRIGGERS;
                }
            }

            if (isEPressed) {
                if (rightTrigger < ABS_LIMIT_TRIGGERS) {
                    rightTrigger += DELTA_TRIGGERS;
                }
            }
        }

        
        if (!(isJPressed || isLPressed)) {
            x = 0;
        }

        if (!(isIPressed || isKPressed)) {
            y = 0;
        }

        if (!(isNP4Pressed || isNP6Pressed)) {
            rX = 0;
        }

        if (!(isNP8Pressed || isNP5Pressed)) {
            rY = 0;
        }

        if (!isQPressed) {
            leftTrigger = 0;
        }

        if (!isEPressed) {
            rightTrigger = 0;
        }

        struct input_event outEvents[19];
        memset(&outEvents, 0, sizeof(outEvents));
        outEvents[0].code = BTN_A;
        outEvents[0].type = EV_KEY;
        outEvents[0].value = isAPressed;

        outEvents[1].code = BTN_B;
        outEvents[1].type = EV_KEY;
        outEvents[1].value = isBPressed;

        outEvents[2].code = BTN_X;
        outEvents[2].type = EV_KEY;
        outEvents[2].value = isXPressed;

        outEvents[3].code = BTN_Y;
        outEvents[3].type = EV_KEY;
        outEvents[3].value = isYPressed;

        outEvents[4].code = ABS_X;
        outEvents[4].type = EV_ABS;
        outEvents[4].value = x;

        outEvents[5].code = ABS_Y;
        outEvents[5].type = EV_ABS;
        outEvents[5].value = y;

        outEvents[6].code = BTN_DPAD_LEFT;
        outEvents[6].type = EV_KEY;
        outEvents[6].value = isLeftPressed;

        outEvents[7].code = BTN_DPAD_UP;
        outEvents[7].type = EV_KEY;
        outEvents[7].value = isUpPressed;

        outEvents[8].code = BTN_DPAD_DOWN;
        outEvents[8].type = EV_KEY;
        outEvents[8].value = isDownPressed;

        outEvents[9].code = BTN_DPAD_RIGHT;
        outEvents[9].type = EV_KEY;
        outEvents[9].value = isRightPressed;

        outEvents[10].code = ABS_HAT2Y;
        outEvents[10].type = EV_ABS;
        outEvents[10].value = leftTrigger;

        outEvents[11].code = ABS_HAT2X;
        outEvents[11].type = EV_ABS;
        outEvents[11].value = rightTrigger;

        outEvents[12].code = BTN_THUMBL;
        outEvents[12].type = EV_KEY;
        outEvents[12].value = isGPressed;

        outEvents[13].code = BTN_THUMBR;
        outEvents[13].type = EV_KEY;
        outEvents[13].value = isHPressed;

        outEvents[14].code = BTN_TL;
        outEvents[14].type = EV_KEY;
        outEvents[14].value = isTPressed;

        outEvents[15].code = BTN_TR;
        outEvents[15].type = EV_KEY;
        outEvents[15].value = isUPressed;

        outEvents[16].code = ABS_RX;
        outEvents[16].type = EV_ABS;
        outEvents[16].value = rX;

        outEvents[17].code = ABS_RY;
        outEvents[17].type = EV_ABS;
        outEvents[17].value = rY;

        outEvents[18].code = SYN_REPORT;
        outEvents[18].type = EV_SYN;
        outEvents[18].value = 0;

        if (write(uinput, &outEvents, sizeof(outEvents)) <= 0) {
            printErr("Failed to write to uinput");
            ioctl(uinput, UI_DEV_DESTROY);
            close(uinput);
            close(devFD);
            return ERR_UINPUT;
        }
    }
}