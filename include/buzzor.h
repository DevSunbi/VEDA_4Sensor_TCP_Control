#ifndef BUZZOR_H
#define BUZZOR_H

void buzzer(char* arg);
void play_fein_style_alert(void);

typedef void (*buzzer_func_t)(char*);
typedef void (*play_fein_style_alert_t)(void);

#endif