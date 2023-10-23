#ifndef NTPCLIENT_H
#define NTPCLIENT_H
//#define ENABLE_DEBUG
/* when present, debug is a true global */
#ifdef ENABLE_DEBUG
extern int debug;
#else
#define debug 0
#endif
extern int ntpTimeSync;
void ntp_operate(char * timezone);

/* global tuning parameter */
extern double min_delay;

/* prototype for function defined in phaselock.c */
int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);

#endif
