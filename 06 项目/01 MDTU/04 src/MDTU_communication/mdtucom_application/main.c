/*************************************************
Author:zhouBL
Version:
Description:mqtt lib
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
#include "app_api.h"
#include "thpool.h"
#include "mqtt.h"
#include "tcp_server.h"
#include "main.h"
#include "com_log.h"
#include "com_network.h"
#include "com_config.h"
/*mqtt tcp op thread*/
threadpool op_thpool = NULL;
pthread_t tcp_th;
pthread_t mqtt_th;
extern volatile int mqtt_connecting;
/*main variable*/
volatile int app_running=1;



static void signal_handler(int signum)
{
    switch(signum)
    {
        case SIGABRT:
            FIBO_LOG(FIBO_LOG_WARN,"[arch_test] recv SIGABRT\n");
            break;
        case SIGBUS:
            FIBO_LOG(FIBO_LOG_WARN,"[arch_test] recv SIGBUS\n");
            break;
        case SIGFPE:
            FIBO_LOG(FIBO_LOG_WARN,"[arch_test] recv SIGFPE\n");
            break;
        case SIGILL:
            FIBO_LOG(FIBO_LOG_WARN,"[arch_test] recv SIGILL\n");
            break;
        case SIGSEGV:
            FIBO_LOG(FIBO_LOG_WARN,"[arch_test] recv SIGSEGV\n");
			exit(1);
            break;
        case SIGUSR1:
            FIBO_LOG(FIBO_LOG_WARN,"[arch_test] recv SIGUSR1, umdp_server exception.\n");
            break;
        default:
            FIBO_LOG(FIBO_LOG_WARN,"[arch_test] recv unknown signal\n");
            break;
    }
	app_running=0;
}

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
}
void pub_topic_func(){
	char* pub_mes;
	int ret;
	//int i;
	//for (i=0;i<10;i++){
			/*2.2发布主题:get*/
	
		pub_mes="{\"ac\": \"getinfo\",\"val\": 0}";
		ret=mqtt_publish(pub_topic,pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 1}";
		ret=mqtt_publish(pub_topic,pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 2}";
		ret=mqtt_publish(pub_topic,pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 3}";
		ret=mqtt_publish(pub_topic,pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 4}";
		ret=mqtt_publish(pub_topic,pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 5}";
		ret=mqtt_publish(pub_topic,pub_mes);
		pub_mes="{\"ac\": \"getinfo\",\"val\": 6}";
		ret=mqtt_publish(pub_topic,pub_mes);
	//}
	
	if(ret){
		int n = errno;
		printlogf(LOG_WARNING, "mqtt_publish failed reason:\n", strerror(n));
		printlogf(LOG_WARNING, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	
	/*set*/
	
	pub_mes="{\"ac\": \"set\",\"op\": \"serial\",\"name\": \"RS232\",\"val\": {\"dbt\": 8,\"sbt\": 1,\"pbt\": 0,\"bps\": 9600}}";
	ret=mqtt_publish(pub_topic,pub_mes);
	pub_mes="{\"ac\": \"set\",\"op\": \"general\",\"val\": {\"port\": 4059,\"bin\": 0,\"timeout\": 120,\"tcptmr\": 30,\"schedule\": 30}}";
	ret=mqtt_publish(pub_topic,pub_mes);
	pub_mes="{\
			\"ac\": \"set\",\
			\"op\": \"sn\",\
			\"val\": {\
				\"model\": \"DT3400-SM\",\
				\"sn\": \"xxxx\",\
				\"key\": \"b8e774f281c81e931b1f3a3a56ee5f42\",\
				\"hver\": 1\
			}\
			}";
	ret=mqtt_publish(pub_topic,pub_mes);
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
	ret=mqtt_publish(pub_topic,pub_mes);
	pub_mes="{ \"ac\": \"set\", \"op\": \"restart\" }";
	ret=mqtt_publish(pub_topic,pub_mes);
	if(ret){
		int n = errno;
		printlogf(LOG_WARNING, "mqtt_publish failed reason:\n", strerror(n));
		printlogf(LOG_WARNING, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
}
int main(int argc, char *argv[]) {
	char* pub_mes;
	int ret;
	signal_register();
	/*根据启动参数不同，打印不同level的log信息，以方便跟踪调试*/
	init_log_param(argc, argv);
	printlogf(LOG_NOTICE,"%s version:%d , started with loglevel %s.\n", argv[0],FIRMWARE_VERSION, LevelAry[LogLevel]);
	/*0.读取配置文件*/
	load_config();
	/*1.开发板联网*/
	ret=init_board_network();
	/*info get/set,初始化耗时线程,为避免频繁开关线程，影响程序性能，这里采用线程池实现*/
	op_thpool = thpool_init(1);
	/*2.启动mqtt*/
	if(pthread_create(&tcp_th, NULL, mqtt_run, NULL) < 0)
    {
    	printlogf(LOG_ERR, "creating mqtt thread failed! %s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
        goto erro1;
    }
	//mqtt_thread_run();
	/*等待mqtt连接成功*/
	while(mqtt_connecting){sleep(1);}
	printlogf(LOG_NOTICE,"mqtt connect successfully!\n");
	/*3.启动tcp server*/
	if(pthread_create(&tcp_th, NULL, open_tcp_server, NULL) < 0)
    {
    	printlogf(LOG_ERR, "creating tcp thread failed! %s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
        goto erro0;
    }
	/*2.1订阅主题*/
	ret=mqtt_subscribe(sub_topic);
	if(ret){
		int n = errno;
		printlogf(LOG_WARNING, "mqtt_subscribe failed reason:\n", strerror(n));
		printlogf(LOG_WARNING, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	/*发布主题*/
	pub_topic_func();
	//pub_mes="{\"ac\": \"getinfo\",\"val\": 0}";
	//ret=mqtt_publish(pub_topic,pub_mes);
	/*2.3取消订阅*/
	/*ret=mqtt_unsubscribe(sub_topic);
	if(ret){
		int n = errno;
		printlogf(LOG_WARNING, "mqtt_unsubscribe failed reason:\n", strerror(n));
		printlogf(LOG_WARNING, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}*/

	/*主循环*/
	while(app_running){
		sleep(1);
		if(DebugOpt)
			printlogf(LOG_DEBUG,"Main thread #%u working\n",(int)pthread_self());
	}
	erro0:
	close_tcp_server();
	erro1:
	mqtt_thread_stop();
	destory_board_network();
	return 0;
}
