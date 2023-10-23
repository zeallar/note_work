#ifndef DEF_STRUCTS_H
#define DEF_STRUCTS_H
typedef struct 
{
	char clientid[64];
	char username[64];
	char password[64];
	char host[24];
	int port;
}global_mqtt_sign_info_t;
/*mqtt account information*/
extern global_mqtt_sign_info_t global_mqtt_sign_info_s;

typedef struct{
	unsigned int tcp_server_bin;
	unsigned int tcp_server_port;
	unsigned int tcp_server_timeout;
	unsigned int tcp_server_tcptmr;
	unsigned int tcp_server_schedule;
}global_tcp_config_t;

/*tcp variable*/
extern global_tcp_config_t global_tcp_config_s;

typedef struct{
	char ntp_primary_ip[24];
	char ntp_second_ip[24];
	unsigned int ntp_interval;
	unsigned int ntp_timezone;
}global_ntp_config_t;

/*ntp_config variable*/
extern global_ntp_config_t global_ntp_config_s;

typedef struct{
	char url[128];
	char model[24];
	unsigned int ver;
}global_update_config_t;
extern global_update_config_t global_update_config_s;


#endif
