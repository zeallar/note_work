/*************************************************
Author:zhouBL
Version:
Description:mqtt类方法
Others:
created date:2023/11/7 3:21 下午
modified date:
*************************************************/
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <syslog.h>
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "MQTTLinux.h"
#include "MQTTClient.h"
#include "fibo_oe.h"
#include "thpool.h"
#include "SemaphoreWrapper.h"
#include "com_utils.h"
#include "cJSON.h"
#include "info_get.h"
#include "info_set.h"
#include "mqtt.h"
#include "def_structs.h"
#include "com_log.h"




/*Semaphore variable,Main thread synchronization*/
#define SEMNUM 0
static int32_t sem;
/*mqtt variable*/
Network n;
MQTTClient c1;

volatile int32_t mqtt_connecting;
extern threadpool op_thpool;

volatile int32_t mqtt_connecting=1;
volatile int32_t mqtt_running=1;
static pthread_mutex_t mqtt_handler_mutex;
extern uint32_t task_id;

/*set info variable*/
const int8_t *SET_OP[] = {
	"serial",
	"general",
	"sn",
	"apn",
	"ntp",
	"snmp",
	"link",
	"update",
	"restart",
	"restore",
	NULL
};

/*def struct*/
struct mqtt_subscribe_t{
	int8_t* topic;
	void (*callback)(MessageData* md);
	struct mqtt_subscribe_t* next;
};
struct mqtt_publish_t{
	uint32_t task_id;
	int8_t* topic;
	int8_t* mes;
	struct mqtt_publish_t* next;
};
static struct mqtt_subscribe_t *subscribe_list=NULL;
static struct mqtt_publish_t *publish_list=NULL;


int32_t run_sub_pub();
void message_arrived_handler(MessageData* md);
void doit(void* dpt);
/*mqtt回调函数*/
void message_arrived_handler(MessageData* md)
{
	pthread_mutex_lock(&mqtt_handler_mutex); 
	MQTTMessage* message = md->message;
	LOGC(LOG_NOTICE,"%.*s", md->topicName->lenstring.len, md->topicName->lenstring.data);
	LOGC(LOG_NOTICE,"%.*s", (int32_t)message->payloadlen, (int8_t*)message->payload);
	/*parse json*/
	struct thread_info_transmit_t *s=MDTU_COM_MALLOC_STRUCT(thread_info_transmit_t);

	s->task_id=task_id++;
	sprintf(s->json_text,"%.*s",(int32_t)message->payloadlen,(int8_t*)message->payload);
	s->flag=0;
	
	list_node_t *list_node_s = list_node_new(s);
	list_rpush(task_list, list_node_s);
	
	doit(s);
	pthread_mutex_unlock(&mqtt_handler_mutex);
}
/* Parse text to JSON, then render back to text, and to exec function! */
/*falg 0:mqtt,1:tcp server 2:usb linked*/
/*根据接收到的shell或json，执行相应的操作*/
void doit(void* ti)
{
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t *)ti;
	//LOGC(LOG_NOTICE,"doit s->task_id=%d",s->task_id);
	int8_t *out;cJSON *json=NULL,*value=NULL;
	list_node_t *list_node_s= list_find(task_list, &(s->task_id));
	json=cJSON_Parse(s->json_text);
	if (!json) {
		s->data_type=1;
		//LOGC(LOG_NOTICE,"json parse failure before: %s go to shell execute",cJSON_GetErrorPtr());
		if(strstr(s->json_text,"not found")!=NULL||strstr(s->json_text,"can't")!=NULL){
			list_remove(task_list,list_node_s);
			goto end;
		}
		switch(s->flag){
			case 0:
				thpool_add_work(op_thpool, shell_op, (void*)s);
				break;
			case 1:
				thpool_add_work(op_thpool, shell_op, (void*)s);
				break;
			case 2:
				thpool_add_work(op_thpool, shell_op, (void*)s);
				break;
			default:
			;
		}
		goto end;
	}
	else
	{
		s->data_type=0;
		value = cJSON_GetObjectItem(json,"ac");
		out=cJSON_Print(value);
	}
	/*get info*/
	if(strstr(out,"get")){
		value = cJSON_GetObjectItem(json,"val");
		out=cJSON_Print(value);
		if (cJSON_GetObjectItem(json, "code") != NULL){
				goto end;
			}
		int32_t *val=(int32_t*)malloc(sizeof(int32_t));
		*val=atoi(out);
		s->code=*val;
		switch(*val){
			case 0:/*基础信息*/
			{	
				/*获取信息*/
				thpool_add_work(op_thpool, get_base_info, (void*)(s));
				break;
			}
			case 1:/*基站附着信息*/
				thpool_add_work(op_thpool, get_base_station_info,(void*)(s));
				break;
			case 2:/*信号参数*/
				thpool_add_work(op_thpool, get_signal_param,(void*)(s));
				break;
			case 3:/*串口信息*/
				thpool_add_work(op_thpool, get_serial_info,(void*)(s));
				break;
			case 4:/*扩展信息*/
				thpool_add_work(op_thpool, get_extra_info,(void*)(s));
				break;
			case 5:/*APN 配置信息*/
				thpool_add_work(op_thpool, get_apn_config,(void*)(s));
				break;
			case 6:/*ntp 配置信息*/
				thpool_add_work(op_thpool, get_ntp_config,(void*)(s));
				break;
			default:
				list_remove(task_list,list_node_s);
				break;
		}
		//cJSON_Delete(json);
	}/*set info*/
	else if(strstr(out,"set")){
		value = cJSON_GetObjectItem(json,"op");
		out=cJSON_Print(value);
		remove_double_quotation_marks(out);
		int32_t j;
		for(j=0;SET_OP[j];j++){
			if(!strncmp(out,SET_OP[j],strlen(SET_OP[j])))
				break;
		}
		switch(j){
			case 0:/*serial*/
				thpool_add_work(op_thpool, set_serial_info, (void*)s);
				break;
			case 1:/*general*/
				thpool_add_work(op_thpool, set_general_info, (void*)s);
				break;
			case 2:/*sn*/
				thpool_add_work(op_thpool, set_sn_info, (void*)s);
				break;
			case 3:/*apn*/
				/*use at commant can't set success or can't find suitable at command*/
				thpool_add_work(op_thpool, set_apn_info, (void*)s);
				break;
			case 4:/*ntp*/
				thpool_add_work(op_thpool, set_ntp_info, (void*)s);
				break;
			case 5:/*snmp*/
				break;
			case 6:/*link*/
				break;
			case 7:/*update*/
				thpool_add_work(op_thpool, set_update_info, (void*)s);
				break;
			case 8:/*restart*/
				thpool_add_work(op_thpool, set_restart_info, (void*)s);
				break;
			case 9:/*restore*/
				thpool_add_work(op_thpool, set_restore_info, (void*)s);
				break;
			default:
				list_remove(task_list,list_node_s);
				break;
			
		}
	}
	//cJSON_Delete(json);
	end:
	//free(out);
	;
}
/*启动mqtt*/
void* mqtt_run()
{	
	int32_t ret;
	pthread_mutex_init(&mqtt_handler_mutex, NULL);/*mqtt接收处理函数互斥量*/
	
	
	static uint8_t sendbuf[1024];
	static uint8_t readbuf[1024];
	uint32_t  command_timeout_ms = 1000;
	static MQTTPacket_connectData mqtt_data= MQTTPacket_connectData_initializer;
	
    mqtt_data.willFlag = 0;
	mqtt_data.MQTTVersion = 3;
	mqtt_data.clientID.cstring = global_mqtt_sign_info_s.clientid;
	mqtt_data.username.cstring = global_mqtt_sign_info_s.username;
	mqtt_data.password.cstring = global_mqtt_sign_info_s.password;
	mqtt_data.keepAliveInterval = 10;
	mqtt_data.cleansession = 1;
    NetworkInit(&n);
    NetworkConnect(&n, global_mqtt_sign_info_s.host, global_mqtt_sign_info_s.port);
    MQTTClientInit(&c1, &n, command_timeout_ms, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));    
    ret = MQTTConnect(&c1, &mqtt_data);
	if(!ret){
		mqtt_connecting=0;		
	}
	int32_t sem_ret;
	/*semaphore init*/
	sem = semCreate_new(123456, 1);
	semSetValue(sem, SEMNUM, 1);/*set init value*/
    while (mqtt_running)
	{	
		sem_ret = semGetValue(sem, SEMNUM);
		if(DebugOpt)
			LOGC(LOG_NOTICE,"sem=%d\n!",sem_ret);
		run_sub_pub();
		MQTTYield(&c1, 1000);//maintain the mqtt connection
	}
	
}
/*执行订阅和发布任务*/
int32_t run_sub_pub(){
	semWait(sem, SEMNUM);/*p operation*/
	int32_t ret;
	struct mqtt_subscribe_t *sub_ptr, *sub_tmp;
	for (sub_ptr = subscribe_list; sub_ptr != NULL; sub_ptr = sub_tmp) {
		sub_tmp = sub_ptr->next;
		ret = MQTTSubscribe(&c1, sub_ptr->topic,QOS2,sub_ptr->callback);
		if(ret){
			LOGC(LOG_ERR,"MQTTSubscribe  ret %d", ret);
			ret=-1;
		}
		free(sub_ptr);
	  } 
	 subscribe_list = NULL;
	
	struct mqtt_publish_t *pub_ptr, *pub_tmp;
	for (pub_ptr = publish_list; pub_ptr != NULL; pub_ptr = pub_tmp) {
		pub_tmp = pub_ptr->next;
		MQTTMessage message;
	    memset(&message, '\0', sizeof(message));
	    message.payload =pub_ptr->mes;
	    message.payloadlen = strlen(message.payload);
	    message.qos = 1;
	    message.retained = 0;
	    message.dup = 0;
		ret = MQTTPublish(&c1,pub_ptr->topic, &message);
		if(ret){
			LOGC(LOG_ERR,"MQTTPublish  ret %d", ret);
			
			LOGC(LOG_NOTICE,"MQTTPublish  ret %d mes %s", ret,pub_ptr->mes);
		}
		list_node_t *list_node_s = list_find(task_list, &(pub_ptr->task_id));
		list_remove(task_list,list_node_s);
		free(pub_ptr);
		//free(list_node_s->val);
	  }
	 publish_list = NULL;
	semIncrement(sem, SEMNUM, 1);/*v operation*/
	return ret;
	
}
/*订阅*/
int32_t mqtt_subscribe(int8_t* topic){
	semWait(sem, SEMNUM);/*p operation*/
	struct mqtt_subscribe_t **s;
	for(s=&(subscribe_list);(*s)!=NULL;s=&((*s)->next));
	*s=MDTU_COM_MALLOC_STRUCT(mqtt_subscribe_t);
	if (*s == NULL) {
		LOGC(LOG_ERR,"don't have enough memory!");
		return -1;
	}
	(*s)->topic=topic;
	(*s)->callback=message_arrived_handler;
	(*s)->next=NULL;
	semIncrement(sem, SEMNUM, 1);/*v operation*/
	return 0;
}
/*发布*/
int32_t mqtt_publish(int8_t* topic,void* _s){
	int32_t ret=0;
	struct thread_info_transmit_t *tit_s=(struct thread_info_transmit_t *)_s;
	semWait(sem, SEMNUM);/*p operation*/
	struct mqtt_publish_t **s;
	for(s=&(publish_list);(*s)!=NULL;s=&((*s)->next));
	*s=MDTU_COM_MALLOC_STRUCT(mqtt_publish_t);
	if (*s == NULL) {
		LOGC(LOG_ERR,"don't have enough memory!");
		ret=-1;
	}
	(*s)->topic=topic;
	(*s)->mes=tit_s->json_text;
	(*s)->task_id=tit_s->task_id;
	(*s)->next=NULL;
	semIncrement(sem, SEMNUM, 1);/*v operation*/
	return ret;
}
/*取消订阅*/
int32_t mqtt_unsubscribe(int8_t* topic){
	int32_t ret;
	ret = MQTTUnsubscribe(&c1, topic);
	if(ret){
		LOGC(LOG_ERR,"MQTTUnsubscribe  ret %d", ret);
		
	}
	return ret;
}
/*mqtt清理*/
void mqtt_thread_stop(){
	/*close mqtt*/
	thpool_wait(op_thpool);
	thpool_destroy(op_thpool);
	mqtt_running=0;
	pthread_mutex_destroy(&mqtt_handler_mutex);
	MQTTDisconnect(&c1);
    NetworkDisconnect(&n);
	LOGC(LOG_NOTICE,"mqtt thread disconnected!");
}

