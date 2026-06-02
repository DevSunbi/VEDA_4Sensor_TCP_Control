#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include "rpi_common.h"

#define LED 1

static const int segment_pins[8] = {21, 22, 23, 24, 26, 27, 28, 29};
// a, b, c, d, e, f, g, dp

// font definition for common cathode 7-segment (A, B, C, D, E, F, G, DP)
static const int segment_data[10][8] = {
    {1, 1, 1, 1, 1, 1, 0, 0}, // 0
    {0, 1, 1, 0, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1, 0}, // 2
    {1, 1, 1, 1, 0, 0, 1, 0}, // 3
    {0, 1, 1, 0, 0, 1, 1, 0}, // 4
    {1, 0, 1, 1, 0, 1, 1, 0}, // 5
    {1, 0, 1, 1, 1, 1, 1, 0}, // 6
    {1, 1, 1, 0, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1, 0}, // 8
    {1, 1, 1, 1, 0, 1, 1, 0}  // 9
};

static void display_digit(int digit) {
    if (digit < 0 || digit > 9) return;
    for (int i = 0; i < 8; i++) {
        digitalWrite(segment_pins[i], segment_data[digit][i]);
    }
}

static void clear_display() {
    for (int i = 0; i < 8; i++) {
        digitalWrite(segment_pins[i], LOW);
    }
}

void segment(char* arg) {
    printf("Segment command: %s\n", arg);

    if (rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return;
    }

    gpio_lock();

    // Initialize pins as OUTPUT
    for (int i = 0; i < 8; i++) {
        pinMode(segment_pins[i], OUTPUT);
    }
    pinMode(LED, OUTPUT);

    if (strcasecmp(arg, "OFF") == 0) {
        clear_display();
        digitalWrite(LED, LOW);
        printf("Segment and LED OFF\n");
    } else if (strcasecmp(arg, "start") == 0) {
        printf("Starting countdown from 9 to 0...\n");
        for (int i = 9; i >= 0; i--) {
            display_digit(i);
            printf("Segment: %d\n", i);
            
            // Release lock during delay so other GPIO actions are not blocked for 10s!
            gpio_unlock();
            delay(1000);
            gpio_lock();
        }
        
        // Value became 0, turn on LED
        digitalWrite(LED, HIGH);
        printf("LED turned ON!\n");
    } else {
        // Try parsing direct digit
        int val = atoi(arg);
        if (val >= 0 && val <= 9) {
            display_digit(val);
            if (val == 0) {
                digitalWrite(LED, HIGH);
                printf("Digit 0, LED ON\n");
            } else {
                digitalWrite(LED, LOW);
            }
        } else {
            fprintf(stderr, "invalid argument: %s\n", arg);
            fprintf(stderr, "Usage: start|OFF|0-9\n");
        }
    }

    gpio_unlock();
}
