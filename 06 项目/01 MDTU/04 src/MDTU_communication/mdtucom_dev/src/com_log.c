#include <time.h>
#include "com_log.h"
#include <syslog.h>
#include "app_api.h"
#include "fibo_oe.h"

/*log variable*/
#define LOG_LEVEL LOG_NOTICE
#define LOG_BUFFER		2048 	/* max size of log line */

uint16_t DebugOpt = 0;
uint16_t LogLevel = LOG_LEVEL;
char buf[LOG_BUFFER]={0};
char buf_temp[LOG_BUFFER]={0};
int8_t *LevelAry[] = {
		"emerg",
		"alert",
		"crit",
		"err",
		"warning",
		"notice",
		"info",
		"debug",
		"panic",
		"error",
		"warn",
		NULL
	};

void init_log_param(int ac, char *av[]){
	
	opterr = 0;
	int i;
	while ((i = getopt(ac,av,"dl:")) != -1) 
	{
	switch (i) {
		case 'l':
			{
				char *ptr;
				int j;
				ptr = optarg;
				for (j = 0; LevelAry[j]; ++j) {
					if (strncmp(ptr, LevelAry[j], strlen(LevelAry[j])) == 0) {
						break;
					}
				}
				switch(j) {
					case 0:
					case 8:
						/* #define	LOG_EMERG	0	[* system is unusable *] */
						LogLevel = LOG_EMERG;
						break;
					case 1:
						/* #define	LOG_ALERT	1	[* action must be taken immediately *] */
						LogLevel = LOG_ALERT;
						break;
					case 2:
						/* #define	LOG_CRIT	2	[* critical conditions *] */
						LogLevel = LOG_CRIT;
						break;
					case 3:
					case 9:
						/* #define	LOG_ERR		3	[* error conditions *] */
						LogLevel = LOG_ERR;
						break;
					case 4:
					case 10:
						/* #define	LOG_WARNING	4	[* warning conditions *] */
						LogLevel = LOG_WARNING;
						break;
					case 5:
						/* #define	LOG_NOTICE	5	[* normal but significant condition *] */
						LogLevel = LOG_NOTICE;
						break;
					case 6:
						/* #define	LOG_INFO	6	[* informational *] */
						LogLevel = LOG_INFO;
						break;
					case 7:
						/* #define	LOG_DEBUG	7	[* debug-level messages *] */
						LogLevel = LOG_DEBUG;
						break;
					default:
						LogLevel = atoi(optarg);
				}
			}
			break;
		case 'd':
			DebugOpt = 1;
			LogLevel = LOG_DEBUG;
			break;
			/* fall through to include f too */
		default:
			/*
			 * check for parse error
			 */
			printf("app [-l level] |-d]\n");
			printf("-l loglevel   log events <= this level (defaults to %s (level %d))\n", LevelAry[LOG_LEVEL], LOG_LEVEL);
			printf("-d            run in debugging mode\n");
			exit(2);
		}
		}
}
void
fdprintf(int fd, const char *ctl, ...)
{
	va_list va;
	memset(buf,0,sizeof(buf));
	va_start(va, ctl);
	vsnprintf(buf, sizeof(buf), ctl, va);
	write(fd, buf, strlen(buf));
	va_end(va);
}



void
vlog(int level, int fd, const char *ctl, va_list va)
{
	memset(buf,0,sizeof(buf));
	memset(buf_temp,0,sizeof(buf_temp));
	time_t t1 = time(NULL);
	if (level <= LogLevel) {
		/*
		 * when -d or -f, we always (and only) log to stderr
		 * fd will be 2 except when 2 is bound to a execing subprocess, then it will be 8
		 * [v]snprintf write at most size including \0; they'll null-terminate, even when they truncate
		 * we don't care here whether it truncates
		 */
		
		char* dt = ctime(&t1);
		dt[strlen(dt)-1]=0;
		sprintf(buf_temp,"[%s] [%s] %s",dt,LevelAry[level],ctl);
		vsnprintf(buf, sizeof(buf), buf_temp, va);
		write(fd, buf, strlen(buf));

	}
}

void
printlogf(int level, const char *ctl, ...)
{
	va_list va;

	va_start(va, ctl);
	vlog(level, 2, ctl, va);
	va_end(va);
}



