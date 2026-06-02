#include <wiringPi.h>
#include <stdio.h>
#include <wiringPiI2C.h>
#include <pthread.h>
#include "rpi_common.h"

#define CDS 0
#define LED 1
int threshold = 180;
pthread_mutex_t threshold_mutex = PTHREAD_MUTEX_INITIALIZER;

int cdsControl()
{
    int fd;
    int cnt = 0;
    int prev;
    int a2dVal;
    int current_threshold;
    int a2dChannel = 0;

    fd = wiringPiI2CSetupInterface("/dev/i2c-1", 0x48);
    if(fd < 0) {
        perror("wiringPiI2CSetupInterface");
        return 1;
    }

    printf("Raspberry Pi CDS Control\n");
    int i;

    // if (wiringPiSetup() == -1) {
    //     fprintf(stderr, "wiringPiSetup failed\n");
    //     return 1;
    // }

    if (rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return 1;
    }

    while(1) {
        wiringPiI2CWrite(fd, 0x40 | a2dChannel);
        
        prev = wiringPiI2CRead(fd);
        a2dVal = wiringPiI2CRead(fd);

        pthread_mutex_lock(&threshold_mutex);
        current_threshold = threshold;
        pthread_mutex_unlock(&threshold_mutex);

        printf("[%d] prev = %d, a2dVal = %d, threshold = %d, ", cnt, prev, a2dVal, current_threshold);

        if(a2dVal < current_threshold)
        {
            printf("Bright!\n");
        } else {
            printf("Dark!\n");
        }

        delay(1000);
        cnt++;
    }

    // photoresistor original code
    // for(;;) {
    //     if(digitalRead(CDS)==LOW) {
    //         gpio_lock();
    //         digitalWrite(LED, HIGH);
    //         delay(100);
    //         digitalWrite(LED, LOW);
    //         printf("Sun Goes Down\n");
    //         gpio_unlock();
    //     }
    //     else {
    //         digitalWrite(LED, LOW);
    //         gpio_lock();
    //         delay(100);
    //         digitalWrite(LED, HIGH);
    //         printf("Sun Goes Up\n");
    //         gpio_unlock();
    //     }
    // }


    return 0;
}

int get_cds_value() {
    int fd = wiringPiI2CSetupInterface("/dev/i2c-1", 0x48);
    if (fd < 0) {
        perror("wiringPiI2CSetupInterface");
        return -1;
    }

    if (rpi_init() == -1) {
        return -1;
    }

    gpio_lock();
    wiringPiI2CWrite(fd, 0x40);
    wiringPiI2CRead(fd); // dummy read
    int a2dVal = wiringPiI2CRead(fd);
    gpio_unlock();

    return a2dVal;
}

// int main()
// {
//     wiringPiSetup();
//     cdsControl();
//     return 0;
// }