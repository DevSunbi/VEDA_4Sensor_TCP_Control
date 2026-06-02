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
        digitalWrite(LED_PIN, HIGH);
        set_led_state(1);
        printf("LED ON\n");
    } else if (strcasecmp(arg, "OFF") == 0) {
        digitalWrite(LED_PIN, LOW);
        set_led_state(0);
        set_cancel_countdown(1);
        printf("LED OFF\n");
    } else {
        fprintf(stderr, "invalid argument: %s\n", arg);
        fprintf(stderr, "Usage: ON|OFF\n");
    }

    gpio_unlock();
}