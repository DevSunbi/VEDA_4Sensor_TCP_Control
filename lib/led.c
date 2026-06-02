#include <stdio.h>
#include <wiringPi.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include "rpi_common.h"

void led(char *arg)
{
    printf("Raspberry Pi blink\n");

    // if (wiringPiSetup() == -1) {
    //     fprintf(stderr, "wiringPiSetup failed\n");
    //     return;
    // }

    if (rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return;
    }

    gpio_lock();

    if (strcasecmp(arg, "ON") == 0) {
        pwmWrite(LED_PIN, 1024);
        set_led_state(1);
        printf("LED ON\n");
    } else if(strcasecmp(arg, "MID") == 0) {
        pwmWrite(LED_PIN, 512);
        set_led_state(1);
        printf("LED MID\n");
    } else if (strcasecmp(arg, "OFF") == 0) {
        pwmWrite(LED_PIN, 0);
        set_led_state(0);
        set_cancel_countdown(1);
        printf("LED OFF\n");
    } 
    else {
        fprintf(stderr, "invalid argument: %s\n", arg);
        fprintf(stderr, "Usage: ON|OFF\n");

        int brightness = atoi(arg);
        if(brightness >= 0 && brightness <=1024) {
            pwmWrite(LED_PIN, brightness);
            set_led_state(brightness > 0 ? 1 : 0);
            printf("LED brightness: %d\n", brightness);
        }
    }


    gpio_unlock();
}