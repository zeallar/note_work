
#include<fcntl.h>
#include<unistd.h>

#include "ini.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "com_log.h"
#include "app_api.h"
#include "mqtt.h"
#include "def_structs.h"

#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))

/*global variable*/
global_mqtt_sign_info_t global_mqtt_sign_info_s={0};
char pub_topic[64];
char sub_topic[64];
/*tcp server variable*/
global_tcp_config_t global_tcp_config_s={0};
/*ntp_config variable*/
global_ntp_config_t global_ntp_config_s={0};
void init_ini();
void load_config();

void check_value(void* text,int type,int n){
	if(type){
		if(DebugOpt){
		printlogf(LOG_DEBUG, "%s len:%d\n", (char*)text,n);
		}
	}else{
		if(DebugOpt){
		printlogf(LOG_DEBUG, "%d\n", (int*)text);
		}
	}
}
void open_ini(){
	FILE *fp = fopen(MDTU_COM_CONFIG_PATH, "r");
	if (fp == NULL) {
		init_ini();
	}
	load_config();
}
void init_ini(){
	long n;
	
	int fd;
	fd = open(MDTU_COM_CONFIG_PATH, O_RDWR|O_CREAT, 0777);
	n = ini_puts("mqtt_sub_pub", "pub_first", "topic1", MDTU_COM_CONFIG_PATH);
	n = ini_puts("mqtt_sub_pub", "sub_first", "topic1", MDTU_COM_CONFIG_PATH);
	
	n = ini_puts("mqtt_sign_info", "clientid", "123456", MDTU_COM_CONFIG_PATH);
	n = ini_puts("mqtt_sign_info", "username", "08818bb8", MDTU_COM_CONFIG_PATH);
	n = ini_puts("mqtt_sign_info", "password", "50ba03ecd0c86bea", MDTU_COM_CONFIG_PATH);
	n = ini_puts("mqtt_sign_info", "host", "172.16.225.24", MDTU_COM_CONFIG_PATH);
	n = ini_puts("mqtt_sign_info", "port", "1883", MDTU_COM_CONFIG_PATH);

	n = ini_putl("tcp_server", "bin", 0, MDTU_COM_CONFIG_PATH);
	n = ini_putl("tcp_server", "port", 4059, MDTU_COM_CONFIG_PATH);
	n = ini_putl("tcp_server", "timeout", 120, MDTU_COM_CONFIG_PATH);
	n = ini_putl("tcp_server", "tcptmr", 0, MDTU_COM_CONFIG_PATH);
	n = ini_putl("tcp_server", "schedule", 0, MDTU_COM_CONFIG_PATH);

	n = ini_puts("ntp_config", "primary_ip", "172.16.13.32", MDTU_COM_CONFIG_PATH);
	n = ini_puts("ntp_config", "second_ip", "172.16.13.33", MDTU_COM_CONFIG_PATH);
	n = ini_putl("tcp_server", "interval", 60, MDTU_COM_CONFIG_PATH);
	n = ini_putl("tcp_server", "timezone", 8, MDTU_COM_CONFIG_PATH);
}	
void load_config(){
	char str[100];
	long n;
	  /* mqtt_sub_pub*/
  	n = ini_gets("mqtt_sub_pub", "pub_first", "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	strcpy(pub_topic,str);
	pub_topic[n+1]='\0';
	check_value((void*)pub_topic,1,n);
	//printf("before pub_topic=%s\n",pub_topic);
	//n = ini_puts("mqtt_sub_pub", "pub_first", "topic2", MDTU_COM_CONFIG_PATH);
	
	n = ini_gets("mqtt_sub_pub", "sub_first", "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	strcpy(sub_topic,str);
	sub_topic[n+1]='\0';
	check_value((void*)sub_topic,1,n);
	
	/*mqtt_sign_info*/
	n = ini_gets("mqtt_sign_info", "clientid", "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	strcpy(global_mqtt_sign_info_s.clientid,str);
	global_mqtt_sign_info_s.clientid[n+1]='\0';
	check_value((void*)global_mqtt_sign_info_s.clientid,1,n);
	
	n = ini_gets("mqtt_sign_info", "username", "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	strcpy(global_mqtt_sign_info_s.username,str);
	global_mqtt_sign_info_s.username[n+1]='\0';
	check_value((void*)global_mqtt_sign_info_s.username,1,n);
	
	n = ini_gets("mqtt_sign_info", "password", "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	strcpy(global_mqtt_sign_info_s.password,str);
	global_mqtt_sign_info_s.password[n+1]='\0';
	check_value((void*)global_mqtt_sign_info_s.password,1,n);

	n = ini_gets("mqtt_sign_info", "host", "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	strcpy(global_mqtt_sign_info_s.host,str);
	global_mqtt_sign_info_s.host[n+1]='\0';
	check_value((void*)global_mqtt_sign_info_s.host,1,n);

	n = ini_getl("mqtt_sign_info", "port", -1, MDTU_COM_CONFIG_PATH);
	global_mqtt_sign_info_s.port=n;
	check_value((void*)global_mqtt_sign_info_s.port,0,n);

	/*tcp_server*/
	n = ini_getl("tcp_server", "bin", -1, MDTU_COM_CONFIG_PATH);
	global_tcp_config_s.tcp_server_bin=n;
	check_value((void*)global_tcp_config_s.tcp_server_bin,0,n);
	
	n = ini_getl("tcp_server", "port", -1, MDTU_COM_CONFIG_PATH);
	global_tcp_config_s.tcp_server_port=n;
	check_value((void*)global_tcp_config_s.tcp_server_port,0,n);
	
	n = ini_getl("tcp_server", "timeout", -1, MDTU_COM_CONFIG_PATH);
	global_tcp_config_s.tcp_server_timeout=n;
	check_value((void*)global_tcp_config_s.tcp_server_timeout,0,n);
	
	n = ini_getl("tcp_server", "tcptmr", -1, MDTU_COM_CONFIG_PATH);
	global_tcp_config_s.tcp_server_tcptmr=n;
	check_value((void*)global_tcp_config_s.tcp_server_tcptmr,0,n);
	
	n = ini_getl("tcp_server", "schedule", -1, MDTU_COM_CONFIG_PATH);
	global_tcp_config_s.tcp_server_schedule=n;
	check_value((void*)global_tcp_config_s.tcp_server_schedule,0,n);
	
	/*ntp_config*/
	n = ini_gets("ntp_config", "primary_ip", "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	strcpy(global_ntp_config_s.ntp_primary_ip,str);
	global_ntp_config_s.ntp_primary_ip[n+1]='\0';
	check_value((void*)global_ntp_config_s.ntp_primary_ip,1,n);

	n = ini_gets("ntp_config", "second_ip", "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	strcpy(global_ntp_config_s.ntp_second_ip,str);
	global_ntp_config_s.ntp_second_ip[n+1]='\0';
	check_value((void*)global_ntp_config_s.ntp_second_ip,1,n);

	n = ini_getl("ntp_config", "interval", -1, MDTU_COM_CONFIG_PATH);
	global_ntp_config_s.ntp_interval=n;
	check_value((void*)global_ntp_config_s.ntp_interval,0,n);

	n = ini_getl("ntp_config", "timezone", -1, MDTU_COM_CONFIG_PATH);
	global_ntp_config_s.ntp_timezone=n;
	check_value((void*)global_ntp_config_s.ntp_timezone,0,n);
}



