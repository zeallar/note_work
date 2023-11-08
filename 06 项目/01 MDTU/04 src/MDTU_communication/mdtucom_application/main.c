/*************************************************
Author:zhouBL
Version:
Description:主方法类
Others:
created date:9/27/2023 9:36 上午
modified date:
*************************************************/
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <stdlib.h>

#include "fibo_oe.h"
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "thpool.h"
#include "mqtt.h"
#include "tcp_server.h"
#include "main.h"
#include "com_log.h"
#include "com_network.h"
#include "com_config.h"
#include "thread_usb.h"
#include "def_structs.h"
#include "schedule_tasks.h"

/*mqtt tcp op thread*/
threadpool op_thpool = NULL;/*执行任务线程*/
pthread_t tcp_th;/*tcp线程*/
pthread_t mqtt_th;/*mqtt线程*/
pthread_t usb_th;/*usb线程*/

extern volatile int32_t mqtt_connecting;/*mqtt连接状态标记位*/
/*main variable*/
volatile int32_t app_running=1;/*app运行标记位*/

list_t *task_list=NULL;/*主线程任务队列*/
uint32_t task_id=1;/*任务唯一id*/
int8_t **wargv;
/*
* @Description:用于app更新时清理主线程
* @return-
*/
void main_thread_clean(){
	app_running=0;
	cron_task_unregister_all();
	close_tcp_server();
	mqtt_thread_stop();
	destory_board_network();
}
/*信号处理函数*/
static void signal_handler(int32_t signum)
{
    switch(signum)
    {
        case SIGABRT:
			LOGC(LOG_WARNING, "recv SIGABRT:%s", strerror(errno));
            break;
        case SIGBUS:
			LOGC(LOG_WARNING, "recv SIGBUS:%s", strerror(errno));
            break;
        case SIGFPE:
			LOGC(LOG_WARNING, "recv SIGFPE:%s", strerror(errno));
            break;
        case SIGILL:
			LOGC(LOG_WARNING, "recv SIGILL:%s", strerror(errno));
            break;
        case SIGSEGV:
			LOGC(LOG_WARNING, "recv SIGSEGV,reason:%s", strerror(errno));
			main_thread_clean();
			exit(1);
            break;
        case SIGUSR1:
			LOGC(LOG_WARNING, "recv SIGUSR1,reason:%s", strerror(errno));
            break;
		 case SIGALRM:
			LOGC(LOG_WARNING, "recv SIGALRM,reason:%s", strerror(errno));
            break;
        default:
			LOGC(LOG_WARNING, "recv unknown signal,reason:%s", strerror(errno));
			main_thread_clean();
			exit(1);
            break;
    }
}
/*信号注册*/
void signal_register(){
	signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGBUS, signal_handler);
    signal(SIGFPE, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGSEGV, signal_handler);
    signal(SIGUSR1, signal_handler);
	signal(SIGALRM, signal_handler);
}
/*
* @Description:创建发布任务
* @param1- text:json
* @return- 0:success other:failed
*/
int32_t creat_pub_task(int8_t* text){
	struct thread_info_transmit_t *s=MDTU_COM_MALLOC_STRUCT(thread_info_transmit_t);
	
	s->task_id=task_id++;
	strcpy(s->json_text,text);
	s->flag=0;
	list_node_t *list_node_s = list_node_new(s);
	list_rpush(task_list, list_node_s);
	return mqtt_publish(pub_topic,s);
}
/*发布任务*/
void pub_topic_func(){
	int8_t* pub_mes;
	int32_t ret;
	//int32_t i;
	//for (i=0;i<10;i++){
			/*2.2发布主题:get*/
		
		pub_mes="{\"ac\": \"getinfo\",\"val\": 0}";
		ret=creat_pub_task(pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 1}";
		ret=creat_pub_task(pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 2}";
		ret=creat_pub_task(pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 3}";
		ret=creat_pub_task(pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 4}";
		ret=creat_pub_task(pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 5}";
		ret=creat_pub_task(pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 6}";
		ret=creat_pub_task(pub_mes);
	//}
	
	if(ret){
		
		LOGC(LOG_WARNING, "mqtt_publish failed reason:", strerror(errno));
	}
	
	/*set*/
	
	pub_mes="{\"ac\": \"set\",\"op\": \"serial\",\"name\": \"RS232\",\"val\": {\"dbt\": 8,\"sbt\": 1,\"pbt\": 0,\"bps\": 9600}}";
	ret=creat_pub_task(pub_mes);
	pub_mes="{\"ac\": \"set\",\"op\": \"general\",\"val\": {\"port\": 4059,\"bin\": 0,\"timeout\": 120,\"tcptmr\": 30,\"schedule\": 30}}";
	ret=creat_pub_task(pub_mes);
	pub_mes="{\
			\"ac\": \"set\",\
			\"op\": \"sn\",\
			\"val\": {\
				\"model\": \"DT3401-SM\",\
				\"sn\": \"xxxx\",\
				\"key\": \"238825094282ea6f071328b7b2a1b6aa\",\
				\"hver\": 1\
			}\
			}";
	ret=creat_pub_task(pub_mes);
	pub_mes="{\
			\"ac\": \"set\",\
			\"op\": \"apn\",\
			\"val\": {\
			\"name\": \"1111\",\
			\"user\": \"22222\",\
			\"passwd\": \"3333xxxx\"\
			}\
			}";
	ret=creat_pub_task(pub_mes);
	pub_mes="{\
			\"ac\": \"set\",\
			\"op\": \"ntp\",\
			\"val\": {\
				\"interval\": 60,\
				\"timezone\": 8,\
				\"primary\": \"172.16.225.24\",\
				\"second\": \"172.16.225.24\"\
			}\
			}";
	ret=creat_pub_task(pub_mes);

	pub_mes="{\
		  \"ac\": \"set\",\
		  \"op\": \"update\",\
		  \"val\": {\
		    \"url\": \"http://172.16.225.25/mqtt_test_v2_crc32_test_11_1.bin\",\
		    \"ver\": 2,\
		    \"model\": \"xxxx\"\
		  }\
		}";
	//ret=creat_pub_task(pub_mes);
	
	pub_mes="{ \"ac\": \"set\", \"op\": \"restart\" }";
	//ret=creat_pub_task(pub_mes);
	pub_mes="{ \"ac\": \"set\", \"op\": \"restore\" }";
	ret=creat_pub_task(pub_mes);
	if(ret){
		LOGC(LOG_WARNING, "mqtt_publish failed reason:", strerror(errno));
		}
}
/*
* @Description:主线程任务id对比函数
* @param1- s1:任务1
* @param2- s2:任务2
* @return-
*/
static int32_t task_id_equal(void *s1,void *s2)
{
	int32_t ret;
	struct thread_info_transmit_t *a=(struct thread_info_transmit_t *)s1;
		struct thread_info_transmit_t *b=(struct thread_info_transmit_t *)s2;
	if(a->task_id==b->task_id)
		ret= 1;
	else
    	ret= 0;
	return ret;
}
/*添加cron任务*/
void add_cron_task(){
	int32_t task_id,ret;
	/*每小时同步一次,超时时间为5s。若任务执行超时将取消执行，继续执行下一个任务。*/
	task_id=cron_task_register("0 0 0/1 * * *","ntp_sync_time",ntp_sync_time,NULL,5);
	/*测试:每1分钟同步一次*/
	//task_id=cron_task_register("0 0/1 * * * *","ntp_sync_time",ntp_sync_time,NULL,5);
	/*测试:每十秒同步一次*/
	//task_id=cron_task_register("0/10 * * * * ?","ntp_sync_time",ntp_sync_time,NULL,5);
	
	LOGC(LOG_NOTICE, "cron_task_register task_id:%d",task_id);
	cron_run();

}
int32_t main(int32_t argc, int8_t **argv) {
	int8_t* pub_mes;
	int32_t ret;
	wargv=argv;
	/*根据启动参数不同，打印不同level的log信息，以方便跟踪调试*/
	signal_register();
	/*初始化log*/
	init_log_param(argc, argv);
	//printlogf(LOG_NOTICE,"%s version:%d , started with loglevel %s.\n", argv[0],FIRMWARE_VERSION, LevelAry[LogLevel]);
	LOGC(LOG_NOTICE,"%s version:%d , started with loglevel %s.", argv[0],FIRMWARE_VERSION, LevelAry[LogLevel]);
	/*0.读取配置文件*/
	ret=open_ini();
	/*联网前用,执行at命令【AT+MIPCALL=1】,否则会拨号失败*/
	/*1.开发板联网*/
	ret=init_board_network();
	/*添加cron任务*/
	add_cron_task();
	/*初始化线程交互链表*/
	task_list = list_new();
	task_list->match = task_id_equal;
	/*2.启动mqtt*/
	if(pthread_create(&tcp_th, NULL, mqtt_run, NULL) < 0)
    {
    	LOGC(LOG_ERR, "creating mqtt thread failed! ");
        goto erro1;
    }
	/*info get/set,初始化耗时线程,为避免频繁开关线程，影响程序性能，这里采用线程池实现*/
	op_thpool = thpool_init(1);
	//mqtt_thread_run();
	/*等待mqtt连接成功*/
	while(mqtt_connecting){sleep(1);}
	LOGC(LOG_NOTICE,"mqtt connect successfully!");

	/*3.启动tcp server*/
	if(pthread_create(&tcp_th, NULL, open_tcp_server, NULL) < 0)
    {
    	LOGC(LOG_ERR, "creating tcp thread failed! ");
        goto erro0;
    }
	/*启动usb线程*/
	if(pthread_create(&usb_th, NULL, open_usb_thread, NULL) < 0)
    {
    	LOGC(LOG_ERR, "creating usb thread failed!");
        goto erro0;
    }

	/*2.1订阅主题*/
	ret=mqtt_subscribe(sub_topic);
	if(ret){
		LOGC(LOG_WARNING, "mqtt_subscribe failed reason:", strerror(errno));
	}
	/*发布主题*/
	pub_topic_func();

	
	//pub_mes="{\"ac\": \"getinfo\",\"val\": 0}";
	//ret=creat_pub_task(pub_mes);
	/*2.3取消订阅*/
	/*ret=mqtt_unsubscribe(sub_topic);
	if(ret){
		
		LOGC(LOG_WARNING, "mqtt_unsubscribe failed reason:", strerror(errno));
	
	}*/

	/*主循环*/
	while(app_running){
		sleep(1);
		if(DebugOpt)
			LOGC(LOG_DEBUG,"Main thread #%u working",(int32_t)pthread_self());
	}
	cron_task_unregister_all();
	erro0:
	close_tcp_server();
	erro1:
	mqtt_thread_stop();
	destory_board_network();
	return 0;
}

