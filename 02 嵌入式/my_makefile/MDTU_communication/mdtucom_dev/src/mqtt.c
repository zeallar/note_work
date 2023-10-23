#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <syslog.h>

#include "MQTTLinux.h"
#include "MQTTClient.h"
#include "fibo_oe.h"
#include "thpool.h"
#include "SemaphoreWrapper.h"
#include "com_utils.h"
#include "app_api.h"
#include "cJSON.h"
#include "info_get.h"
#include "info_set.h"
#include "mqtt.h"
#include "def_structs.h"

/*Semaphore variable,Main thread synchronization*/
#define SEMNUM 0
static int sem;
/*mqtt variable*/
Network n;
MQTTClient c1;
volatile int mqtt_connecting;
extern threadpool op_thpool;

volatile int mqtt_connecting=1;
volatile int mqtt_running=1;
static pthread_mutex_t mutex;
/*set info variable*/
const char *SET_OP[] = {
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
	char* topic;
	void (*callback)(MessageData* md);
	struct mqtt_subscribe_t* next;
};
struct mqtt_publish_t{
	char* topic;
	char* mes;
	struct mqtt_publish_t* next;
};
static struct mqtt_subscribe_t *subscribe_list=NULL;
static struct mqtt_publish_t *publish_list=NULL;


int run_sub_pub();
void message_arrived_handler(MessageData* md);
void doit(char *text);

void message_arrived_handler(MessageData* md)
{
	pthread_mutex_lock(&mutex); 
	MQTTMessage* message = md->message;
	printlogf(LOG_NOTICE,"%.*s\n", md->topicName->lenstring.len, md->topicName->lenstring.data);
	printlogf(LOG_NOTICE,"%.*s\n", (int)message->payloadlen, (char*)message->payload);
	/*parse json*/
	char val[(int)message->payloadlen];
	sprintf(val,"%.*s",(int)message->payloadlen,(char*)message->payload);
	doit(val);
	pthread_mutex_unlock(&mutex);
}
/* Parse text to JSON, then render back to text, and to exec function! */
void doit(char *text)
{
	char *out;cJSON *json=NULL,*value=NULL;

	
	json=cJSON_Parse(text);
	if (!json) {
		printlogf(LOG_NOTICE,"json parse failure before: %s\n",cJSON_GetErrorPtr());
		goto end;
	}
	else
	{	
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
		int *val=(int*)malloc(sizeof(int));
		*val=atoi(out);
		switch(*val){
			case 0:/*基础信息*/
			{	
				/*获取信息*/
				thpool_add_work(op_thpool, get_base_info, (void*)(val));
				break;
			}
			case 1:/*基站附着信息*/
				thpool_add_work(op_thpool, get_base_station_info,(void*)(val));
				break;
			case 2:/*信号参数*/
				thpool_add_work(op_thpool, get_signal_param,(void*)(val));
				break;
			case 3:/*串口信息*/
				thpool_add_work(op_thpool, get_serial_info,(void*)(val));
				break;
			case 4:/*扩展信息*/
				thpool_add_work(op_thpool, get_extra_info,(void*)(val));
				break;
			case 5:/*APN 配置信息*/
				thpool_add_work(op_thpool, get_apn_config,(void*)(val));
				break;
			case 6:/*ntp 配置信息*/
				thpool_add_work(op_thpool, get_ntp_config,(void*)(val));
				break;
			default:
				;
		}
		//cJSON_Delete(json);
	}/*set info*/
	else if(strstr(out,"set")){
		value = cJSON_GetObjectItem(json,"op");
		out=cJSON_Print(value);
		remove_double_quotation_marks(out);
		int j;
		for(j=0;SET_OP[j];j++){
			if(!strncmp(out,SET_OP[j],strlen(SET_OP[j])))
				break;
		}
		switch(j){
			case 0:/*serial*/
				thpool_add_work(op_thpool, set_serial_info, (void*)json);
				break;
			case 1:/*general*/
				thpool_add_work(op_thpool, set_general_info, (void*)json);
				break;
			case 2:/*sn*/
				thpool_add_work(op_thpool, set_sn_info, (void*)json);
				break;
			case 3:/*apn*/
				/*use at commant can't set success or can't find suitable at command*/
				break;
			case 4:/*ntp*/
				thpool_add_work(op_thpool, set_ntp_info, (void*)json);
				break;
			case 5:/*snmp*/
				break;
			case 6:/*link*/
				break;
			case 7:/*update*/
				break;
			case 8:/*restart*/
				thpool_add_work(op_thpool, set_restart_info, (void*)json);
				break;
			case 9:/*restore*/
				break;
			default:
				;
			
		}
	}
	//cJSON_Delete(json);
	end:
		//free(out);
		;
}

void* mqtt_run()
{	
	int ret;
	pthread_mutex_init(&mutex, NULL);/*mqtt接收处理函数互斥量*/

	static unsigned char sendbuf[1024];
	static unsigned char readbuf[1024];
	unsigned int  command_timeout_ms = 1000;
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
	int sem_ret;
	/*semaphore init*/
	sem = semCreate_new(123456, 1);
	semSetValue(sem, SEMNUM, 1);/*set init value*/
    while (mqtt_running)
	{	
		sem_ret = semGetValue(sem, SEMNUM);
		if(DebugOpt)
			printlogf(LOG_NOTICE,"sem=%d\n!\n",sem_ret);
		run_sub_pub();
		MQTTYield(&c1, 1000);//maintain the mqtt connection
	}
	
}
int run_sub_pub(){
	semWait(sem, SEMNUM);/*p operation*/
	int ret;
	struct mqtt_subscribe_t *sub_ptr, *sub_tmp;
	for (sub_ptr = subscribe_list; sub_ptr != NULL; sub_ptr = sub_tmp) {
		sub_tmp = sub_ptr->next;
		ret = MQTTSubscribe(&c1, sub_ptr->topic,QOS2,sub_ptr->callback);
		if(ret){
			printlogf(LOG_ERR,"MQTTSubscribe  ret %d\n", ret);
			printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
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
			printlogf(LOG_ERR,"MQTTPublish  ret %d\n", ret);
			printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		}
		//printlogf(LOG_NOTICE,"MQTTPublish  ret %d mes %s\n", ret,pub_ptr->mes);
		free(pub_ptr);
	  }
	 publish_list = NULL;
	semIncrement(sem, SEMNUM, 1);/*v operation*/
	return ret;
	
}

int mqtt_subscribe(char* topic){
	semWait(sem, SEMNUM);/*p operation*/
	struct mqtt_subscribe_t **s;
	for(s=&(subscribe_list);(*s)!=NULL;s=&((*s)->next));
	*s=MDTU_COM_MALLOC_STRUCT(mqtt_subscribe_t);
	if (*s == NULL) {
		printlogf(LOG_ERR,"don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	(*s)->topic=topic;
	(*s)->callback=message_arrived_handler;
	(*s)->next=NULL;
	semIncrement(sem, SEMNUM, 1);/*v operation*/
	return 0;
}

int mqtt_publish(char* topic,char* mes){

	semWait(sem, SEMNUM);/*p operation*/
	struct mqtt_publish_t **s;
	for(s=&(publish_list);(*s)!=NULL;s=&((*s)->next));
	*s=MDTU_COM_MALLOC_STRUCT(mqtt_publish_t);
	if (*s == NULL) {
		printlogf(LOG_ERR,"don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	(*s)->topic=topic;
	(*s)->mes=mes;
	(*s)->next=NULL;
	semIncrement(sem, SEMNUM, 1);/*v operation*/
	return 0;
}

int mqtt_unsubscribe(char* topic){
	int ret;
	ret = MQTTUnsubscribe(&c1, topic);
	if(ret){
		printlogf(LOG_ERR,"MQTTUnsubscribe  ret %d\n", ret);
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	return ret;
}

void mqtt_thread_stop(){
	/*close mqtt*/
	thpool_wait(op_thpool);
	thpool_destroy(op_thpool);
	mqtt_running=0;
	pthread_mutex_destroy(&mutex);
	MQTTDisconnect(&c1);
    NetworkDisconnect(&n);
	printlogf(LOG_NOTICE,"mqtt thread disconnected!\n");
}

