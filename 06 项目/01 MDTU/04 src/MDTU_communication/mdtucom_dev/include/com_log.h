#ifndef COM_LOG_H
#define COM_LOG_H
#include <syslog.h>

void logC(const int8_t *func, const int8_t *file, const int32_t line,
		   const int32_t type, const int8_t *format, ...);
/*
* @Description:去除文件路径
* @param1- x:路径
* @return- 文件名
*/
#define FILE_NAME(x) (strrchr(x,'/')?strrchr(x,'/')+1:x)
/*
* @Description:基本log
* @param1- type:log级别
* @param2- format:格式化
* @param3- format:参数
* @return- none
*/
#define LOGC(type, format, ...) logC(__func__, __FILE__, __LINE__, type, format, ##__VA_ARGS__)
/*
* @Description:校验log
* @param1- A:0:打印log并跳转,1:无操作
* @return- none
*/
#define error_unless(A)\
		if (!(A)) { \
			fprintf(stderr, "\n[%s@%s:%d] FAILED\n", FILE_NAME(__FILE__),__func__, __LINE__);\
			goto error;\
			}
/*
* @Description:测试log
* @param1- M:格式化
* @param2- ...:参数
* @return-
*/
#define log_test(M,...) \
	fprintf(stderr, "\n[%s@%s:%d] "M"\n", FILE_NAME(__FILE__),__func__, __LINE__,##__VA_ARGS__)


void init_log_param(int32_t ac, int8_t *av[]);

#endif /*COM_LOG_H*/
