#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <softPwm.h>
#include "rpi_common.h"

int led(char *arg)
{
    printf("Raspberry Pi blink\n");

    // if (wiringPiSetup() == -1) {
    //     fprintf(stderr, "wiringPiSetup failed\n");
    //     return;
    // }

    if (rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return -1;
    }

    int result = 0;
    gpio_lock();

    if (strcasecmp(arg, "ON") == 0) {
        softPwmWrite(LED_PIN, 100);
        set_led_state(1);
        printf("LED ON\n");
    } else if(strcasecmp(arg, "MID") == 0) {
        softPwmWrite(LED_PIN, 50);
        set_led_state(1);
        printf("LED MID\n");
    } else if (strcasecmp(arg, "OFF") == 0) {
        softPwmWrite(LED_PIN, 0);
        set_led_state(0);
        set_cancel_countdown(1);
        printf("LED OFF\n");
    } 
    else {
        char *endptr;
        long brightness = strtol(arg, &endptr, 10);
        if (*endptr == '\0' && endptr != arg && brightness >= 0 && brightness <= 1024) {
            softPwmWrite(LED_PIN, (int)(brightness * 100 / 1024));
            set_led_state(brightness > 0 ? 1 : 0);
            printf("LED brightness: %ld (mapped to softPwm %ld)\n", brightness, brightness * 100 / 1024);
        } else {
            fprintf(stderr, "invalid argument: %s\n", arg);
            fprintf(stderr, "Usage: ON|MID|OFF|brightness(0-1024)\n");
            result = -1;
        }
    }


    gpio_unlock();
    return result;
}