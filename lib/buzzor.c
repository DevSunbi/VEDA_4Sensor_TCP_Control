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

void buzzer(char* arg) {
    printf("Buzzer scale control: %s\n", arg);

    if (rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return;
    }

    gpio_lock();

    if (!tone_initialized) {
        if (softToneCreate(BUZZER) != 0) {
            fprintf(stderr, "softToneCreate failed\n");
            gpio_unlock();
            return;
        }
        tone_initialized = 1;
    }

    if (strcasecmp(arg, "OFF") == 0 || strcmp(arg, "0") == 0) {
        softToneWrite(BUZZER, 0);
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
            softToneWrite(BUZZER, freq);
            delay(500);
            softToneWrite(BUZZER, 0);
        } else {
            // Try to parse as direct frequency integer
            freq = atoi(arg);
            if (freq > 0) {
                printf("Playing direct frequency: %d Hz for 500ms\n", freq);
                softToneWrite(BUZZER, freq);
                delay(500);
                softToneWrite(BUZZER, 0);
            } else {
                fprintf(stderr, "invalid argument: %s\n", arg);
                fprintf(stderr, "Usage: do|re|mi|fa|sol|la|si|do2 OR 1-8 OR frequency_hz OR OFF\n");
            }
        }
    }

    gpio_unlock();
}