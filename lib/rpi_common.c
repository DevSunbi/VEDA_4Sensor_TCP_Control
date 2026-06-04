#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>
#include <softPwm.h>
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
    pinMode(BUZZER_PIN, OUTPUT);

    if (softPwmCreate(LED_PIN, shared_led_state ? 100 : 0, 100) != 0) {
        fprintf(stderr, "softPwmCreate failed\n");
        return -1;
    }

    for (int i = 0; i < 8; i++) {
        pinMode(segment_pins[i], OUTPUT);
    }

    init_result = 1;
    printf("GPIO initialized successfully (using softPwm)\n");
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

static pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_led_state(int state)
{
    pthread_mutex_lock(&state_mutex);
    shared_led_state = state;
    pthread_mutex_unlock(&state_mutex);
}

int get_led_state(void)
{
    int state;
    pthread_mutex_lock(&state_mutex);
    state = shared_led_state;
    pthread_mutex_unlock(&state_mutex);
    return state;
}

static int cancel_countdown_flag = 0;

void set_cancel_countdown(int val)
{
    pthread_mutex_lock(&state_mutex);
    cancel_countdown_flag = val;
    pthread_mutex_unlock(&state_mutex);
}

int get_cancel_countdown(void)
{
    int val;
    pthread_mutex_lock(&state_mutex);
    val = cancel_countdown_flag;
    pthread_mutex_unlock(&state_mutex);
    return val;
}