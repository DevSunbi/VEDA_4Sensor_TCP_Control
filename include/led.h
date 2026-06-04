#ifndef LED_H
#define LED_H

int led(char* arg);

typedef int (*led_func_t)(char*);

#endif