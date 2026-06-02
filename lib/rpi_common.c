#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>
#include "rpi_common.h"

const int segment_pins[8] = {21, 22, 23, 24, 25, 27, 28, 29};

static pthread_mutex_t gpio_mutex = PTHREAD_MUTEX_INITIALIZER;
static int init_result = 0;

int rpi_init(void)
{
    if(init_result) {
        return 0;
    }

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPiSetup failed\n");
        return -1;
    }

    pinMode(SW_PIN, INPUT);
    pinMode(CDS_PIN, INPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    for (int i = 0; i < 8; i++) {
        pinMode(segment_pins[i], OUTPUT);
    }

    init_result = 1;
    printf("GPIO initialized successfully\n");
    return 0;
}

void gpio_lock(void)
{
    pthread_mutex_lock(&gpio_mutex);
}

void gpio_unlock(void)
{
    pthread_mutex_unlock(&gpio_mutex);
}