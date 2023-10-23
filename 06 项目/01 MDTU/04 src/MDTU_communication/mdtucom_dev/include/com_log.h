#ifndef COM_LOG_H
#define COM_LOG_H
#include <syslog.h>
#include "fibo_oe.h"
#include "app_api.h"
/*log variable*/
#define LOG_LEVEL LOG_NOTICE
extern uint16_t LogLevel;
#define LOG_BUFFER		2048 	/* max size of log line */

extern uint16_t LogLevel;
extern uint16_t DebugOpt;
extern int8_t *LevelAry[];
void init_log_param(int ac, char *av[]);
void
fdprintf(int fd, const char *ctl, ...);
void
printlogf(int level, const char *ctl, ...);

#endif /*COM_LOG_H*/
