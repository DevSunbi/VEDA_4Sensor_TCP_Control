#ifndef SEGMENT_H
#define SEGMENT_H

int segment(char *str);
void segment_init(void);

typedef int (*segment_func_t)(char*);

#endif