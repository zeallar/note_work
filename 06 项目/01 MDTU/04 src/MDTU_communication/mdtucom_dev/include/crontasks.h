#ifndef CRONTASKS_H
#define CRONTASKS_H
/*
* @Description:定义回调函数
* @param1- 参数:回调函数注册id
* @param2- 参数:回调函数参数
* @return- none
*/
typedef void    (CronCallback) (uint32_t clientreg,
                                         void *clientarg);
/*
* @Description:任务注册
* @param1- when:何时发生
* @param2- task_name:任务名称
* @param3- thecallback:回调函数
* @param4- clientarg:回调函数参数
* @param5- timeout:超时时间
* @return-注册成功返回注册id，注册失败返回-1
*/
uint32_t 
cron_task_register(int8_t* when,int8_t* task_name,
                    CronCallback * thecallback, void *clientarg
                    ,unsigned long timeout);
/*
* @Description:取消指定任务注册
* @param1- clientreg:任务id
* @return- none
*/
void cron_task_unregister(uint32_t clientreg);
/*
* @Description:取消全部注册任务
* @return-
*/
void cron_task_unregister_all(void);
/*
* @Description:cron线程开始执行
* @return- none
*/
void cron_run();


#endif 
