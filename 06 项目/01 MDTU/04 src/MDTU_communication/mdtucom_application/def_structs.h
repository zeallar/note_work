#ifndef DEF_STRUCTS_H
#define DEF_STRUCTS_H
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t
#include "list.h"

typedef struct 
{
	int8_t clientid[64];
	int8_t username[64];
	int8_t password[64];
	int8_t host[24];
	int32_t port;
}global_mqtt_sign_info_t;
/*mqtt account information*/
extern global_mqtt_sign_info_t global_mqtt_sign_info_s;

typedef struct{
	uint32_t tcp_server_bin;
	uint32_t tcp_server_port;
	uint32_t tcp_server_timeout;
	uint32_t tcp_server_tcptmr;
	uint32_t tcp_server_schedule;
}global_tcp_config_t;

/*tcp variable*/
extern global_tcp_config_t global_tcp_config_s;

typedef struct{
	int8_t ntp_primary_ip[24];
	int8_t ntp_second_ip[24];
	uint32_t ntp_interval;
	uint32_t ntp_timezone;
}global_ntp_config_t;

/*ntp_config variable*/
extern global_ntp_config_t global_ntp_config_s;

typedef struct{
	int8_t url[256];
	int8_t model[24];
	uint32_t ver;
}global_update_config_t;
extern global_update_config_t global_update_config_s;


/*Interaction information between threads*/
struct thread_info_transmit_t{
	uint32_t task_id;//task id
	int8_t json_text[4096];
	int32_t flag;/*falg 0:mqtt,1:tcp server 2:usb linked*/
	int32_t data_type;/*0:json,1:shell*/
	int32_t code;
	int32_t sock;
	int32_t fd_usb;
	int8_t *argv[];
};
extern list_t *task_list;

/**/
#endif
