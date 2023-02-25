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
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef NO_CONFIG
#include <ini.h>
#include <limits.h>
#include <stdlib.h>
#endif

#define ERR_SETUP 1
#define ERR_INPUT 2
#define ERR_UINPUT 3
#define ERR_NO_HOME 4
#define ERR_INTERNAL 5
#define ERR_ARG 6

#define DELTA_STICK 64
#define DELTA_TRIGGERS 16

#define ABS_LIMIT_TRIGGERS 128
#define ABS_LIMIT_STICK 512

#define CONFIG_AMOUNT 22

#define CONFIG_A "A"
#define CONFIG_B "B"
#define CONFIG_X "X"                    
#define CONFIG_Y "Y"                    
#define CONFIG_DPAD_LEFT "DPAD_LEFT"
#define CONFIG_DPAD_UP "DPAD_UP"
#define CONFIG_DPAD_DOWN "DPAD_DOWN"
#define CONFIG_DPAD_RIGHT "DPAD_RIGHT"
#define CONFIG_LEFT_THUMB_LEFT "LEFT_THUMB_LEFT"
#define CONFIG_LEFT_THUMB_UP "LEFT_THUMB_UP"
#define CONFIG_LEFT_THUMB_DOWN "LEFT_THUMB_DOWN"
#define CONFIG_LEFT_THUMB_RIGHT "LEFT_THUMB_RIGHT"
#define CONFIG_LEFT_THUMB_CLICK "LEFT_THUMB_CLICK"
#define CONFIG_LEFT_TRIGGER "LEFT_TRIGGER"
#define CONFIG_RIGHT_TRIGGER "RIGHT_TRIGGER"
#define CONFIG_RIGHT_THUMB_LEFT "RIGHT_THUMB_LEFT"
#define CONFIG_RIGHT_THUMB_UP "RIGHT_THUMB_UP"
#define CONFIG_RIGHT_THUMB_DOWN "RIGHT_THUMB_DOWN"
#define CONFIG_RIGHT_THUMB_RIGHT "RIGHT_THUMB_RIGHT"
#define CONFIG_RIGHT_THUMB_CLICK "RIGHT_THUMB_CLICK"
#define CONFIG_LEFT_BUMPER "LEFT_BUMPER"
#define CONFIG_RIGHT_BUMPER "RIGHT_BUMPER"


static double getTimeWithMillis() {
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec + floor(time.tv_nsec / 1.0e6) / 1000.0;
}

static void printErr(const char *err) {
    fprintf(stderr, "%s\n", err);
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

typedef struct Config {
    char **keys;
    unsigned short *values;
} Config;

#ifndef NO_CONFIG
static int iniIndex = 0;

static unsigned long min(unsigned long x, unsigned long y) {
    if (x < y) {
        return x;
    }
    return y;
}

static int iniGen(void *user, const char *section, const char *name, const char *value) {
    if (!strcmp(section, "keys")) {
        Config *config = (Config*) user;
        char *key = strdup(name);
        config->keys[iniIndex] = key;
        config->values[iniIndex] = min(strtoul(value, NULL, 10), USHRT_MAX);
        iniIndex++;
    }
    return 1;
}

// This isn't necessary strictly speaking but oh well
static void cleanupConfig(Config* config) {
    for (int i = 0; i < CONFIG_AMOUNT; i++) {
        free(config->keys[i]);
    }
    free(config->keys);
    free(config->values);
}
#endif

static unsigned short getValue(Config *config, const char *key) {
    for (int i = 0; i < CONFIG_AMOUNT; i++) {
        if (!strcmp(config->keys[i], key)) {
            return config->values[i];
        }
    }
    return 0;
}

static void defaultConfig(Config *config) {
    static char* keys[] = {
        CONFIG_A,
        CONFIG_B,
        CONFIG_X,                    
        CONFIG_Y,                  
        CONFIG_DPAD_LEFT,
        CONFIG_DPAD_UP,
        CONFIG_DPAD_DOWN,
        CONFIG_DPAD_RIGHT,
        CONFIG_LEFT_THUMB_LEFT,
        CONFIG_LEFT_THUMB_UP,
        CONFIG_LEFT_THUMB_DOWN,
        CONFIG_LEFT_THUMB_RIGHT,
        CONFIG_LEFT_THUMB_CLICK,
        CONFIG_LEFT_TRIGGER,
        CONFIG_RIGHT_TRIGGER,
        CONFIG_RIGHT_THUMB_LEFT,
        CONFIG_RIGHT_THUMB_UP,
        CONFIG_RIGHT_THUMB_DOWN,
        CONFIG_RIGHT_THUMB_RIGHT,
        CONFIG_RIGHT_THUMB_CLICK,
        CONFIG_LEFT_BUMPER,
        CONFIG_RIGHT_BUMPER
    };
    static unsigned short values[] = {
        KEY_A,
        KEY_B,
        KEY_X,
        KEY_Y,
        KEY_DELETE,
        KEY_HOME,
        KEY_END,
        KEY_PAGEDOWN,
        KEY_I,
        KEY_J,
        KEY_K,
        KEY_L,
        KEY_Q,
        KEY_E,
        KEY_KP4,
        KEY_KP8,
        KEY_KP5,
        KEY_KP6,
        KEY_G,
        KEY_H,
        KEY_T,
        KEY_U
    };
    
    config->keys = keys;
    config->values = values;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printErr("Invalid arguments");
        return ERR_ARG;
    }

    Config config;
    #ifndef NO_CONFIG
    char *configPath = getenv("CONTROLLEREMU_CONFIG");

    if (configPath == NULL && argc == 3 && !strcmp(argv[2], "--find-config")) {
        char *configDir = getenv("XDG_CONFIG_HOME");
        if (configDir != NULL) {
            configPath = strcat(configDir, "/controller-emu.cfg");
        }
        if (configPath == NULL) {
            char *home = getenv("HOME");
            if (home != NULL) {
                configPath = strcat(home, "/.config/controller-emu.cfg");
            }
        }
        if (configPath == NULL) {
            printErr("Your $HOME is not set, go figure that out");
            return ERR_NO_HOME;
        }
    }

    const bool mallocConfig = configPath != NULL;
    #define maybeCleanup() if (mallocConfig) cleanupConfig(&config)
    if (mallocConfig) {
        config.keys = malloc(sizeof(char*) * CONFIG_AMOUNT);
        config.values = malloc(sizeof(unsigned short*) * CONFIG_AMOUNT);
        if (ini_parse(configPath, iniGen, &config)) {
            cleanupConfig(&config);
            return ERR_INTERNAL;
        }
    } else {
        defaultConfig(&config);
    }
    #else
    defaultConfig(&config);
    #endif

    char *dev = argv[1];

    int uinput = open("/dev/uinput", O_WRONLY);

    if (uinput < 0) {
        #ifndef NO_CONFIG
        maybeCleanup();
        #endif
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
        #ifndef NO_CONFIG
        maybeCleanup();
        #endif
        printErr("Failed to initialize");
        return ERR_SETUP;
    }

    int devFD = open(dev, O_RDONLY);

    if (devFD < 0) {
        printErr("Failed to open input device");
        #ifndef NO_CONFIG
        maybeCleanup();
        #endif
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
            #ifndef NO_CONFIG
            maybeCleanup();
            #endif
            ioctl(uinput, UI_DEV_DESTROY);
            close(uinput);
            close(devFD);
            return ERR_INPUT;
        }
        int isPressed = event.value;
        unsigned short key = event.code;

        if (key == getValue(&config, CONFIG_A)) {
            isAPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_B)) {
            isBPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_X)) {
            isXPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_Y)) {
            isYPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_DPAD_LEFT)) {
            isLeftPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_DPAD_UP)) {
            isUpPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_DPAD_DOWN)) {
            isDownPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_DPAD_RIGHT)) {
            isRightPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_LEFT_THUMB_UP)) {
            isIPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_LEFT_THUMB_LEFT)) {
            isJPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_LEFT_THUMB_DOWN)) {
            isKPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_LEFT_THUMB_RIGHT)) {
            isLPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_LEFT_TRIGGER)) {
            isQPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_RIGHT_TRIGGER)) {
            isEPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_LEFT_THUMB_CLICK)) {
            isGPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_RIGHT_THUMB_CLICK)) {
            isHPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_LEFT_BUMPER)) {
            isTPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_RIGHT_BUMPER)) {
            isUPressed = isPressed;
        } else if (key == getValue(&config, CONFIG_RIGHT_THUMB_UP)) {
            isNP8Pressed = isPressed;
        } else if (key == getValue(&config, CONFIG_RIGHT_THUMB_LEFT)) {
            isNP4Pressed = isPressed;
        } else if (key == getValue(&config, CONFIG_RIGHT_THUMB_DOWN)) {
            isNP5Pressed = isPressed;
        } else if (key == getValue(&config, CONFIG_RIGHT_THUMB_RIGHT)) {
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
            #ifndef NO_CONFIG
            maybeCleanup();
            #endif
            printErr("Failed to write to uinput");
            ioctl(uinput, UI_DEV_DESTROY);
            close(uinput);
            close(devFD);
            return ERR_UINPUT;
        }
    }
}
