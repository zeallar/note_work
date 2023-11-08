/*************************************************
Author:zhouBL
Version:
Description:配置文件类方法
Others:
created date:2023/11/7 3:10 下午
modified date:
*************************************************/
#include<fcntl.h>
#include<unistd.h>

#include "ini.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "fibo_type.h"

#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "com_log.h"
#include "mqtt.h"
#include "def_structs.h"





#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))

/*global variable*/
global_mqtt_sign_info_t global_mqtt_sign_info_s={0};
int8_t pub_topic[64];
int8_t sub_topic[64];
/*tcp server variable*/
global_tcp_config_t global_tcp_config_s={0};
/*ntp_config variable*/
global_ntp_config_t global_ntp_config_s={0};
/*ota*/
global_update_config_t global_update_config_s={0};

int32_t init_ini();
int32_t load_config();
/*
* @Description:打开配置文件
* @return- none
*/
int32_t open_ini(){
 	int32_t ret;
	FILE *fp = fopen(MDTU_COM_CONFIG_PATH, "r");
	if (fp == NULL) {
		init_ini();
	}
	ret=load_config();
	return ret;
}	
/*
* @Description:添加或者修改str值
* @param1- section:分组
* @param2- key:
* @param3- val:
* @return- 0:success,othre:failed
*/
int32_t ini_put_str(const int8_t *section,const int8_t *key,const int8_t* val){
	return ini_puts(section,key, val, MDTU_COM_CONFIG_PATH)?0:1;
}
/*
* @Description:添加或者修改int值
* @param1- section:分组
* @param2- key:
* @param3- val:
* @return- 0:success,othre:failed
*/
int32_t ini_put_int(const int8_t *section,const int8_t *key,const int32_t val){
	return ini_putl(section,key, val, MDTU_COM_CONFIG_PATH)?0:1;
}
/*
* @Description:创建默认配置文件
* @return- 0:success,othre:failed
*/
int32_t init_ini(){
	int32_t fd,ret;
	fd = open(MDTU_COM_CONFIG_PATH, O_RDWR|O_CREAT, 0777);
	if(fd<0){
		ret=-1;
	}
	ret=ini_put_str("mqtt_sub_pub", "pub_first", "topic1");
	ret=ini_put_str("mqtt_sub_pub", "sub_first", "topic1");

	ret=ini_put_str("mqtt_sign_info", "clientid", "123456");
	ret=ini_put_str("mqtt_sign_info", "username", "08818bb8");
	ret=ini_put_str("mqtt_sign_info", "password", "50ba03ecd0c86bea");
	ret=ini_put_str("mqtt_sign_info", "host", "172.16.225.24");
	ret=ini_put_int("mqtt_sign_info", "port", 1883);

	ret=ini_put_int("tcp_server", "bin", 0);
	ret=ini_put_int("tcp_server", "port", 4059);
	ret=ini_put_int("tcp_server", "timeout", 120);
	ret=ini_put_int("tcp_server", "tcptmr", 0);
	ret=ini_put_int("tcp_server", "schedule", 0);

	ret=ini_put_str("ntp_config", "primary_ip", "172.16.225.24");
	ret=ini_put_str("ntp_config", "primary_ip", "172.16.225.24");
	ret=ini_put_int("ntp_config", "interval", 60);
	ret=ini_put_int("ntp_config", "timezone", 8);

	ret=ini_put_str("ota_update", "url", "http://172.16.225.25/mqtt_test_v2_crc32_test_11_1.bin");
	ret=ini_put_str("ota_update", "model", "L716");
	ret=ini_put_int("ota_update", "ver", 1);

	ret=ini_put_str("base_station_info", "lteBands", "B2/B3");

	ret=ini_put_str("apn", "name", "xxxx");
	ret=ini_put_str("apn", "user", "xxxx");
	ret=ini_put_str("apn", "passwd", "xxxx");
	return ret;
	
}	
/*
* @Description:获取str类型值
* @param1- section:分组
* @param2- key:
* @param3- location:存储key对应的val
* @return- 0:success,othre:failed
*/
int32_t get_date_str(const int8_t *section,const int8_t *key,int8_t* location){
	long n;int8_t str[100]={0};
	n = ini_gets(section, key, "dummy", str, sizearray(str), MDTU_COM_CONFIG_PATH);
	if(strncmp(str,"dummy",5)==0){
		return -1;
	}
	strcpy(location,str);
	return 0;
}
/*
* @Description:获取int类型值
* @param1- section:分组
* @param2- key:
* @param3- location:存储key对应的val
* @return- 0:success,othre:failed
*/
int32_t get_date_int(const int8_t *section,const int8_t *key,int32_t* location){
	long n;
	n = ini_getl(section, key, -1, MDTU_COM_CONFIG_PATH);
	if(n==-1)return -1;
	(*location)=n;
	return 0;
}

/*
* @Description:加载配置文件
* @return- none
*/
int32_t load_config(){
	int32_t ret;
	 /* mqtt_sub_pub*/
  	ret=get_date_str("mqtt_sub_pub","pub_first",pub_topic);
	//printf("before pub_topic=%s\n",pub_topic);
	//n = ini_puts("mqtt_sub_pub", "pub_first", "topic2", MDTU_COM_CONFIG_PATH);

	ret=get_date_str("mqtt_sub_pub","sub_first",sub_topic);

	/*mqtt_sign_info*/
	ret=get_date_str("mqtt_sign_info","clientid",global_mqtt_sign_info_s.clientid);

	ret=get_date_str("mqtt_sign_info","username",global_mqtt_sign_info_s.username);

	ret=get_date_str("mqtt_sign_info","password",global_mqtt_sign_info_s.password);

	ret=get_date_str("mqtt_sign_info","host",global_mqtt_sign_info_s.host);

	ret=get_date_int("mqtt_sign_info","port",&(global_mqtt_sign_info_s.port));
	
	/*tcp_server*/
	ret=get_date_int("tcp_server","bin",&(global_tcp_config_s.tcp_server_bin));

	ret=get_date_int("tcp_server","port",&(global_tcp_config_s.tcp_server_port));
	
	ret=get_date_int("tcp_server","timeout",&(global_tcp_config_s.tcp_server_timeout));
	
	ret=get_date_int("tcp_server","tcptmr",&(global_tcp_config_s.tcp_server_tcptmr));
	
	ret=get_date_int("tcp_server","schedule",&(global_tcp_config_s.tcp_server_schedule));

	
	/*ntp_config*/
	ret=get_date_str("ntp_config","primary_ip",global_ntp_config_s.ntp_primary_ip);
	
	ret=get_date_str("ntp_config","second_ip",global_ntp_config_s.ntp_second_ip);
	
	ret=get_date_int("ntp_config","interval",&(global_ntp_config_s.ntp_interval));

	ret=get_date_int("ntp_config","timezone",&(global_ntp_config_s.ntp_timezone));
	
	/*url_config*/
	ret=get_date_str("ota_update","url",global_update_config_s.url);
	
	ret=get_date_str("ota_update","model",global_update_config_s.model);
	
	ret=get_date_int("ota_update","ver",&(global_update_config_s.ver));
	return ret;
}



