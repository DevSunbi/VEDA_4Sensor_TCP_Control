#ifndef RPI_COMMON_H
#define RPI_COMMON_H

#define LED_PIN    1  // wPi 1 / BCM 18
#define BUZZER_PIN 2  // wPi 2 / BCM 27
#define CDS_PIN    3  // wPi 3 / BCM 22
#define SW_PIN     5  // wPi 5 / BCM 24
#define PR_PIN     0  // wPi 0 / BCM 17

extern const int segment_pins[8];

int rpi_init(void);
void gpio_lock(void);
void gpio_unlock(void);

void set_led_state(int state);
int get_led_state(void);

void set_cancel_countdown(int val);
int get_cancel_countdown(void);

#endif