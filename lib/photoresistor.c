#include <wiringPi.h>
#include <stdio.h>
#include <wiringPiI2C.h>
#include <pthread.h>
#include <unistd.h>
#include "rpi_common.h"

#define CDS 0
#define LED 1

int threshold = 200;
pthread_mutex_t threshold_mutex = PTHREAD_MUTEX_INITIALIZER;

int cdsControl()
{
    int cds_fd;
    int cnt = 0;
    int prev;
    int a2dVal;
    int current_threshold;
    int a2dChannel = 0;
    int cdsDigitalVal;

    if (rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return 1;
    }

    cds_fd = wiringPiI2CSetupInterface("/dev/i2c-1", 0x48);
    if(cds_fd < 0) {
        perror("wiringPiI2CSetupInterface");
        return 1;
    }

    pinMode(CDS, INPUT);
    pinMode(LED, OUTPUT);

    printf("Raspberry Pi CDS Control\n");

    while(1) {
        /*
            1. PCF8591 ADC 값 읽기
            AIN0 채널 읽기
        */
        wiringPiI2CWrite(cds_fd, 0x40 | a2dChannel);

        prev = wiringPiI2CRead(cds_fd);      // dummy read
        a2dVal = wiringPiI2CRead(cds_fd);    // real read

        pthread_mutex_lock(&threshold_mutex);
        current_threshold = threshold;
        pthread_mutex_unlock(&threshold_mutex);

        /*
            2. 기존 digitalRead 방식도 같이 읽기
            CDS 핀이 센서 모듈의 DO에 연결되어 있어야 의미 있음
        */
        cdsDigitalVal = digitalRead(CDS);

        printf("[%d] prev = %d, a2dVal = %d, threshold = %d, digital = %d, ",
               cnt, prev, a2dVal, current_threshold, cdsDigitalVal);

        /*
            3. ADC 기준 밝기 판단
            네 회로에서 밝을수록 값이 작게 나온다는 전제
        */
        if(a2dVal < current_threshold)
        {
            printf("ADC: Bright, ");
        } 
        else {
            printf("ADC: Dark, ");
        }

        /*
            4. 기존 digitalRead 기준 LED 제어
            네 기존 코드 기준:
            digitalRead(CDS) == LOW 이면 어두움으로 처리
        */
        if(cdsDigitalVal == LOW) {
            gpio_lock();
            digitalWrite(LED, HIGH);
            gpio_unlock();

            printf("DIGITAL: Sun Goes Down -> LED ON\n");
        }
        else {
            gpio_lock();
            digitalWrite(LED, LOW);
            gpio_unlock();

            printf("DIGITAL: Sun Goes Up -> LED OFF\n");
        }

        delay(1000);
        cnt++;
    }

    return 0;
}

int get_cds_value() 
{
    int fd;
    int a2dVal;

    fd = wiringPiI2CSetupInterface("/dev/i2c-1", 0x48);
    if (fd < 0) {
        perror("wiringPiI2CSetupInterface");
        return -1;
    }

    if (rpi_init() == -1) {
        close(fd);
        return -1;
    }

    gpio_lock();

    wiringPiI2CWrite(fd, 0x40);
    wiringPiI2CRead(fd); // dummy read
    a2dVal = wiringPiI2CRead(fd);

    gpio_unlock();

    close(fd);

    return a2dVal;
}

// photoresistor.c 맨 아래에 추가
int get_pr_value() 
{
    if (rpi_init() == -1) {
        return -1;
    }

    gpio_lock();
    pinMode(CDS, INPUT); // 0번 핀을 입력 모드로 설정 (CDS = 0)
    int val = digitalRead(CDS);
    gpio_unlock();

    return val;
}


/*
int main()
{
    cdsControl();
    return 0;
}
*/