#include <stdio.h>
#include <wiringPi.h>
#include <softTone.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include "rpi_common.h"

#define BUZZER 2  // BCM GPIO27 is wiringPi pin 2

typedef struct {
    char *note;
    int frequency;
} NoteMap;

static NoteMap notes[] = {
    {"do", 262},
    {"re", 294},
    {"mi", 330},
    {"fa", 349},
    {"sol", 392},
    {"la", 440},
    {"si", 494},
    {"do2", 523},
    {"1", 262},
    {"2", 294},
    {"3", 330},
    {"4", 349},
    {"5", 392},
    {"6", 440},
    {"7", 494},
    {"8", 523},
    {NULL, 0}
};

static int tone_initialized = 0;

void play_fein_style_alert(void);

int buzzer(char* arg) {
    printf("Buzzer scale control: %s\n", arg);

    if (rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return -1;
    }

    int result = 0;
    gpio_lock();

    if (!tone_initialized) {
        if (softToneCreate(BUZZER_PIN) != 0) {
            fprintf(stderr, "softToneCreate failed\n");
            gpio_unlock();
            return -1;
        }
        tone_initialized = 1;
    }

    if(strcasecmp(arg, "fein") == 0 || strcasecmp(arg, "alert") == 0) {
        printf("Playing Fein Alert Melody\n");
        gpio_unlock();
        play_fein_style_alert();
        gpio_lock();
    } else if (strcasecmp(arg, "OFF") == 0 || strcmp(arg, "0") == 0) {
        softToneWrite(BUZZER_PIN, 0);
        printf("Buzzer OFF\n");
    } else {
        int freq = 0;
        for (int i = 0; notes[i].note != NULL; i++) {
            if (strcasecmp(arg, notes[i].note) == 0) {
                freq = notes[i].frequency;
                break;
            }
        }

        if (freq > 0) {
            printf("Playing note %s at %d Hz for 500ms\n", arg, freq);
            softToneWrite(BUZZER_PIN, freq);
            delay(500);
            softToneWrite(BUZZER_PIN, 0);
        } else {
            // Try to parse as direct frequency integer
            freq = atoi(arg);
            if (freq > 0) {
                printf("Playing direct frequency: %d Hz for 500ms\n", freq);
                softToneWrite(BUZZER_PIN, freq);
                delay(500);
                softToneWrite(BUZZER_PIN, 0);
            } else {
                fprintf(stderr, "invalid argument: %s\n", arg);
                fprintf(stderr, "Usage: do|re|mi|fa|sol|la|si|do2 OR 1-8 OR frequency_hz OR OFF\n");
                result = -1;
            }
        }
    }

    gpio_unlock();
    return result;
}

void play_fein_style_alert(void)
{
    int melody[] = {
        440, 0,
        440, 0,
        660, 660, 660, 660,
        880
    };

    int duration[] = {
        180, 80,
        180, 80,
        80, 80, 80, 80,
        400
    };

    int len = sizeof(melody) / sizeof(melody[0]);

    if (!tone_initialized) {
        if (softToneCreate(BUZZER_PIN) != 0) {
            fprintf(stderr, "softToneCreate failed\n");
            return;
        }
        tone_initialized = 1;
    }

    for (int i = 0; i < len; i++) {
        softToneWrite(BUZZER_PIN, melody[i]);
        delay(duration[i]);
        softToneWrite(BUZZER_PIN, 0);
        delay(40);
    }
}