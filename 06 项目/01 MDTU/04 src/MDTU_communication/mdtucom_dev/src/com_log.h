#ifndef COM_LOG_H
#define COM_LOG_H
#include <syslog.h>
void init_log_param(int ac, char *av[]);
void
fdprintf(int fd, const char *ctl, ...);
void
printlogf(int level, const char *ctl, ...);

#endif /*COM_LOG_H*/
