/*************************************************
Author:zhouBL
Version:
Description:轻量级cron,添加任务超时功能。
Others:
created date:2023/11/7 11:13 上午
modified date:
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <setjmp.h>

#include "ccronexpr.h"
#include "crontasks.h"
#include "com_log.h"
/*task status*/
#define JOB_NONE        0/*运行*/
#define JOB_ARMED       -1/*就绪*/
#define JOB_WAITING     -2/*等待*/
#define CRON_MALLOC_STRUCT(s)   (struct s *) calloc(1, sizeof(struct s))
/*task struct*/
typedef struct cron_task {
        int8_t*            schedule;/*执行时间，cron表达式*/
        int8_t*             job_name;/*任务名称*/   
        uint32_t    clientreg;/*任务唯一注册id*/
        void           *clientarg;/*任务回调函数参数*/
        CronCallback *thecallback;/*回调函数*/
        struct cron_expr*    expr;/*解析后的cron表达式*/
		time_t 		  nextTrigger;
        int32_t		           cl_Pid;/* running pid, 0, or armed (-1), or waiting (-2) */
        time_t 		  cl_NotUntil;/*最后运行时间*/
		unsigned long 	  timeout;
		time_t 	   timeout_record;/*任务执行超时时间*/
        struct cron_task *next;
    }cron_task_t;
/*corn functions*/
void* run_job(void   * param);
void* crond();
void prev_stamp(cron_task_t* task);
int32_t  arm_jobs(void);
void updateNextTrigger(cron_task_t* task);
/*cron variable*/
static uint32_t regnum = 1;/*task唯一id*/
static struct cron_task *the_tasks = NULL;/*任务队列*/
pthread_mutex_t tasks_mutex;/*任务执行互斥量*/
pthread_cond_t	cond;/*任务执行完成条件变量*/
int32_t isFunctionCompleted;/*任务是否完成*/
/*
* @Description:移除cron任务
* @param1- clientreg:任务id
* @return-
*/
void
cron_task_unregister(uint32_t clientreg){
    struct cron_task *sa_ptr, **prevNext = &the_tasks;

    for (sa_ptr = the_tasks;
         sa_ptr != NULL && sa_ptr->clientreg != clientreg;
         sa_ptr = sa_ptr->next) {
        prevNext = &(sa_ptr->next);
        }
    if (sa_ptr != NULL) {
        *prevNext = sa_ptr->next;
        free(sa_ptr);
    }
}
/*
* @Description:移除cron的所有任务
* @return-
*/
void
cron_task_unregister_all(void)
{
  struct cron_task *sa_ptr, *sa_tmp;

  for (sa_ptr = the_tasks; sa_ptr != NULL; sa_ptr = sa_tmp) {
    sa_tmp = sa_ptr->next;
	free(sa_ptr->expr);
    free(sa_ptr);
  }
  the_tasks = NULL;
}  
/*
* @Description:cron任务注册
* @param1- when:cron表达式
* @param2- task_name:任务名称
* @param3- thecallback:回调函数指针
* @param4- clientarg:回调函数参数
* @param5- time_out:超时时间
* @return-
*/
uint32_t
cron_task_register(int8_t* when,int8_t* task_name,
					CronCallback * thecallback, void *clientarg,unsigned long time_out)
{	
	/*任务链表*/
	struct cron_task **s = NULL;
	
	for (s = &(the_tasks); *s != NULL; s = &((*s)->next));

	*s = CRON_MALLOC_STRUCT(cron_task);
	if (*s == NULL) {
		perror("CRON_MALLOC_STRUCT:");
		return -1;
	}
	(*s)->schedule = when;
	(*s)->clientarg = clientarg;
	(*s)->thecallback = thecallback;
	(*s)->clientreg = regnum++;
	(*s)->cl_Pid = JOB_WAITING;
	(*s)->job_name=task_name;
	/*解析cron表达式，得到下次运行时间*/
	cron_expr *expr_s = CRON_MALLOC_STRUCT(cron_expr);
	const int8_t* err = NULL;
	cron_parse_expr((*s)->schedule, expr_s, &err);
	 if (err) {
		perror("error parsing cron expression:");
		return -1;
	}
	(*s)->expr=expr_s;
	(*s)->next = NULL;
	(*s)->cl_Pid=JOB_WAITING;
	(*s)->timeout=time_out;
	/*更新nextTrigger*/
	updateNextTrigger((*s));
	return (*s)->clientreg;
}
/*
* @Description:执行cron任务
* @param1- param:cron_task_t任务结构体指针
* @return-
*/
void* run_job(void   * param)

{	
	cron_task_t *s=(cron_task_t*)param;
	if(s->cl_Pid==JOB_ARMED)
	{
		s->cl_Pid=JOB_NONE;
		//printf("next triggertime=%s\n",ctime(&(s->nextTrigger)));
		(*((s)->thecallback)) ((s)->clientreg, (s)->clientarg);
		prev_stamp(s);
		updateNextTrigger(s);
		s->cl_Pid=JOB_WAITING;
	}
	// 释放互斥锁
    pthread_mutex_lock(&tasks_mutex);
    isFunctionCompleted = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&tasks_mutex);
}
/*
* @Description:装载任务
* @return-
*/
int32_t  arm_jobs(void){

	short nJobs = 0;
	struct cron_task *sa_ptr_global, *sa_tmp_global;
	for (sa_ptr_global = the_tasks; sa_ptr_global != NULL; sa_ptr_global = sa_tmp_global) {
		volatile time_t current_time = time(NULL);
		if(sa_ptr_global->nextTrigger<=current_time&&sa_ptr_global->cl_Pid!=JOB_ARMED){
			sa_ptr_global->cl_Pid=JOB_ARMED;
			nJobs+=1;
		
			pthread_t thread;
			pthread_mutex_init(&tasks_mutex, NULL);
			pthread_cond_init(&cond, NULL);
			isFunctionCompleted = 0;
			
			// 创建新线程并执行函数
   			if(pthread_create(&thread, NULL, run_job, sa_ptr_global)<0){
				LOGC(LOG_ERR, "creating cron task thread failed! ");
				return -1;
			}
			// 等待函数完成或超时
		    pthread_mutex_lock(&tasks_mutex);
		    if (!isFunctionCompleted) {
		        struct timespec ts;
		        clock_gettime(CLOCK_REALTIME, &ts);
		        ts.tv_sec += sa_ptr_global->timeout;
		        pthread_cond_timedwait(&cond, &tasks_mutex, &ts);
		    }
		    pthread_mutex_unlock(&tasks_mutex);

			 // 检查函数是否已完成
		    if (!isFunctionCompleted) {
		        printf("Function timed out after %d seconds\n", sa_ptr_global->timeout);
		        // 取消线程
		        pthread_cancel(thread);
		    }

			 // 清理
		    pthread_join(thread, NULL);
		 	pthread_mutex_destroy(&tasks_mutex);
   			pthread_cond_destroy(&cond);
			
		}
		sa_tmp_global = sa_ptr_global->next;
	}

	return nJobs;
}
/*
* @Description:cron主循环
* @return-
*/
void* crond(){
	for (;;) {
		sleep(1);
		//printf("wakeup\n");
		arm_jobs();
	}
}

/*
* @Description:启动cron线程
* @return-
*/
void cron_run(){
   int32_t err = 0;
	pthread_t th;
	/*以分离状态启动子线程*/
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if((err = pthread_create(&th, &attr, crond, NULL) != 0)){
        perror("pthread_create error");
    }
}

/*
* @Description:更新任务的下一次触发时间
* @return-
*/
void updateNextTrigger(cron_task_t* task){
	time_t current_time = time(NULL);
    time_t next_run;
	next_run = cron_next(task->expr, current_time);
	task->nextTrigger=next_run;
}
/*
* @Description:记录任务最后一次执行时间
* @return-
*/
void prev_stamp(cron_task_t* task){
	time_t current_time = time(NULL);
	task->cl_NotUntil=cron_prev(task->expr, current_time);
}
