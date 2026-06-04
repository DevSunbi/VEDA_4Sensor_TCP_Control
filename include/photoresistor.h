#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

int cdsControl(void);
int get_cds_value(void);
int get_pr_value(void);

typedef int (* get_cds_value_t)(void);
typedef int (* get_pr_value_t)(void);

#endif
