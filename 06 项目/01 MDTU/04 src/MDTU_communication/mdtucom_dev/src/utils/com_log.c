/*************************************************
Author:zhouBL
Version:
Description:日志类方法
Others:
created date:2023/11/7 3:11 下午
modified date:
*************************************************/
#include <time.h>
#include <syslog.h>

#include "fibo_oe.h"
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "com_log.h"

/*log variable*/
int8_t *log_path = "/var/log/daemon.log";/*日志路径*/

#define LOG_LEVEL LOG_NOTICE

uint16_t DebugOpt = 0;
uint16_t LogLevel = LOG_LEVEL;

int8_t *LevelAry[] = {
		"EMERG",
		"ALERT",
		"CRIT",
		"ERR",
		"WARNING",
		"NOTICE",
		"INFO",
		"DEBUG",
		"PANIC",
		"ERROR",
		"WARN",
		NULL
	};
/*
* @Description:根据启动命令，执行相应等级的log
* @param1- ac:
* @param2- av:
* @return- none
*/
void init_log_param(int32_t ac, int8_t *av[]){
	
	opterr = 0;
	int32_t i;
	while ((i = getopt(ac,av,"dl:")) != -1) 
	{
	switch (i) {
		case 'l':
			{
				int8_t *ptr;
				int32_t j;
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
/*
* @Description:打印log到控制台。修改文件描述符，也可以打印log到文件
* @param1- func:函数名
* @param2- file:文件名
* @param3- line:行号
* @param4- type:log类型
* @param5- format:格式化
* @param6- ...:参数
* @return- none
*/
void logC(const int8_t *func, const int8_t *file, const int32_t line,
		   const int32_t type, const int8_t *format, ...)
 {
	
	 time_t loacl_time;
	 int8_t time_str[128];
 
	 // 获取本地时间
	 time(&loacl_time);
	 strftime(time_str, sizeof(time_str), "[%Y.%m.%d %X]", localtime(&loacl_time));
	 
	 // 日志内容格式转换
	 va_list ap;
	 va_start(ap, format);
	 int8_t fmt_str[2048];
	 vsnprintf(fmt_str, sizeof(fmt_str), format, ap);
	 va_end(ap);
 
	 // 打开日志文件
	 //FILE *file_fp= fopen(log_path, "a");
	 
	 FILE *file_fp=NULL;//输出到控制台
	 
	 // 写入到日志文件中
	 if (file_fp != NULL)
	 {
	 	if (type <= LogLevel) {
		 fprintf(file_fp, "[%s]%s[%s@%s:%d] %s\n", LevelAry[type], time_str, func, 
				 FILE_NAME(file), line, fmt_str);
		 fclose(file_fp);
	 	}
	 }
	 else
	 {
	 	if (type <= LogLevel) {
				if(type<=4||type==7){
					fprintf(stderr, "%s[%s@%s:%d][%s] %s\n", time_str, func, 
				 FILE_NAME(file), line, LevelAry[type], fmt_str);
				}
				else{
					fprintf(stderr, "%s[%s] %s\n", time_str,  LevelAry[type],fmt_str);
				}
	
	 		}
	 }
 }
		   

