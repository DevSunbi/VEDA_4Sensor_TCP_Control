#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <softTone.h>
#include <dlfcn.h>
#include "rpi_common.h"

// Set to 1 if using a Common Anode display, 0 for Common Cathode
#define COMMON_ANODE 1
#define SEGMENT_ON  (COMMON_ANODE ? LOW : HIGH)
#define SEGMENT_OFF (COMMON_ANODE ? HIGH : LOW)

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
        digitalWrite(segment_pins[i], segment_data[digit][i] ? SEGMENT_ON : SEGMENT_OFF);
    }
}

static void clear_display() {
    for (int i = 0; i < 8; i++) {
        digitalWrite(segment_pins[i], SEGMENT_OFF);
    }
}

void segment(char* arg) {
    printf("Segment command: %s\n", arg);

    if (rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return;
    }

    gpio_lock();

    if (strcasecmp(arg, "OFF") == 0) {
        set_cancel_countdown(1);
        clear_display();
        pwmWrite(LED_PIN, 0);
        set_led_state(0);
        printf("Segment and LED OFF\n");
    } else if (strcasecmp(arg, "start") == 0) {
        printf("Starting countdown from 9 to 0...\n");
        // Reset LED to OFF at start of countdown
        pwmWrite(LED_PIN, 0);
        set_led_state(0);
        set_cancel_countdown(0);
        
        for (int i = 9; i >= 0; i--) {
            // Release lock briefly to check if a cancel was requested
            gpio_unlock();
            int cancelled = get_cancel_countdown();
            gpio_lock();
            if (cancelled) {
                printf("Countdown cancelled!\n");
                clear_display();
                pwmWrite(LED_PIN, 0);
                set_led_state(0);
                gpio_unlock();
                return;
            }

            display_digit(i);
            printf("Segment: %d\n", i);
            
            // Release lock during delay so other GPIO actions are not blocked for 10s!
            gpio_unlock();
            delay(1000);
            gpio_lock();
        }
        
        // Final check after the last second delay
        gpio_unlock();
        int cancelled = get_cancel_countdown();
        gpio_lock();
        if (cancelled) {
            printf("Countdown cancelled at the end!\n");
            clear_display();
            pwmWrite(LED_PIN, 0);
            set_led_state(0);
            gpio_unlock();
            return;
        }

        // Value became 0, turn on LED
        pwmWrite(LED_PIN, 1024);
        set_led_state(1);
        printf("LED turned ON!\n");

        void* buzz_handle = dlopen("./libbuzzor.so", RTLD_LAZY);
        if(buzz_handle) {
            void (*buzzer_func)(void) = dlsym(buzz_handle, "play_fein_style_alert");
            if(buzzer_func) {
                buzzer_func();
            } else {
                fprintf(stderr, "dlsym play_fein_style_alert failed: %s\n", dlerror());
            }
            dlclose(buzz_handle);
        } else {
            fprintf(stderr, "dlopen ./libbuzzor.so failed: %s\n", dlerror());
        }
    } else {
        // Try parsing direct digit
        int val = atoi(arg);
        if (val >= 0 && val <= 9) {
            display_digit(val);
            if (val == 0) {
                pwmWrite(LED_PIN, 1024);
                set_led_state(1);
                printf("Digit 0, LED ON\n");
            } else {
                pwmWrite(LED_PIN, 0);
                set_led_state(0);
            }
        } else {
            fprintf(stderr, "invalid argument: %s\n", arg);
            fprintf(stderr, "Usage: start|OFF|0-9\n");
        }
    }

    gpio_unlock();
}
