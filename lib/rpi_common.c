#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>

#define SW 5
#define CDS 0
#define LED 1

static pthread_mutex_t gpio_mutex = PTHREAD_MUTEX_INITIALIZER;
static int init_result = 0;

// int rpi_init(void)
// {
//     if(init_result) {
//         return 0;
//     }

//     if (wiringPiSetup() == -1) {
//         fprintf(stderr, "wiringPiSetup failed\n");
//         return;
//     }

//     pinMode(SW, INPUT);
//     pinMode(CDS, INPUT);
//     pinMode(LED, OUTPUT);

//     init_result = 1;
//     printf("GPIO initialized successfully\n");
//     return 0;
// }

int rpi_init(void)
{
    if(init_result) {
        return 0;
    }

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "wiringPiSetup failed\n");
        return -1;
    }

    pinMode(SW, INPUT);
    pinMode(CDS, INPUT);
    pinMode(LED, OUTPUT);

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