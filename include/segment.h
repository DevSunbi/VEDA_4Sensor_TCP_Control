#ifndef SEGMENT_H
#define SEGMENT_H

void segment(char *str);
void segment_init(void);

typedef void (*segment_func_t)(char*);

#endif