#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>
#include "rpi_common.h"

const int segment_pins[8] = {21, 22, 23, 24, 25, 27, 28, 29};

static pthread_mutex_t gpio_mutex = PTHREAD_MUTEX_INITIALIZER;
static int init_result = 0;
static int shared_led_state = 0;

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

    // Set initial pin value based on shared state
    digitalWrite(LED_PIN, shared_led_state ? HIGH : LOW);

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

void set_led_state(int state)
{
    shared_led_state = state;
}

int get_led_state(void)
{
    return shared_led_state;
}

static int cancel_countdown_flag = 0;

void set_cancel_countdown(int val)
{
    cancel_countdown_flag = val;
}

int get_cancel_countdown(void)
{
    return cancel_countdown_flag;
}