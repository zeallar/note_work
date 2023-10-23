#include "MQTTLinux.h"
#include "MQTTClient.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>   
#include <netinet/in.h>   
#include <arpa/inet.h> 

#include "fibo_oe.h"
#include "fibo_type.h"
#include "fibo_uart.h"
#include "thpool.h"
#include "cJSON.h"
#include "info_get.h"
#include "mqtt.h"
#include "com_utils.h"
#include "com_log.h"
#include "com_network.h"
#include "app_api.h"
#include "def_structs.h"
#define ADDR_BUF_SIZE        512
#define FIBO_UART1_DEV      "/dev/ttyS1"
/*macro define*/
/*0.基础信息*/
#define HW_VERSION_SIZE 16
#define SDK_VERSION_SIZE 16
#define CCID_SIZE 256
#define IMSI_SIZE 128
#define IMEI_SIZE 128
#define SN_SIZE 64
#define MODEL_SIZE 128
/*1.基站附着信息*/
#define OPERATOR_SIZE 64
/*define get struct*/
/*0.基础信息*/
struct base_info_t{
	char fver[SDK_VERSION_SIZE];
	char hver[HW_VERSION_SIZE];
	char ccid[CCID_SIZE];
	char imsi[IMSI_SIZE];
	char imei[IMEI_SIZE];
	char sn[SN_SIZE];
	char model[MODEL_SIZE];
};
/*1.基站附着信息*/
struct base_station_info_t{
	char *ip;
	char *apn;
	char operator[OPERATOR_SIZE];
	char *lteBands;
	char *lteStatus;
	int txPower;
	int pCID;
	int cid;
	int mcc;
	int mnc;
};
/*2.信号参数*/
struct signal_param_t{
	int ecio;
	int rsrp;
	int rsrq;
	int rssi;
	int sinr;
	int snr;
};
/*3.串口信息*/
struct serial_info_t{
	char *name;
	int bps;
	int dbt;
	int pbt;
	int sbt;
};
/*4.扩展信息*/
struct extra_info_t{
	int bin;
	int port;
	int timeout;
	int tcptmr;
	int schedule;
};
/*5.APN 配置信息*/
struct apn_config_t{
	char* name;
	char* user;
	char* passwd;
};
/*6.ntp 配置信息*/
struct ntp_config_t{
	char *primary_ip;
	char *second_ip;
	int interval;
	int timezone;
};


/*common methods*/
int open_serial();
void info_publish(char* pub_mes);
/*get info*/
int base_info_read(struct base_info_t* s);
char* base_info_convert_json_object(int val,struct base_info_t* s);

int base_station_info_read(struct base_station_info_t* s);
char* base_station_info_convert_json_object(int val,struct base_station_info_t* s);

int signal_param_read(struct signal_param_t* s);
char* signal_param_convert_json_object(int val,struct signal_param_t* s);

int serial_info_read(struct serial_info_t* s);
char* serial_info_convert_json_object(int val,struct serial_info_t* s);

int extra_info_read(struct extra_info_t* s);
char* extra_info_convert_json_object(int val,struct extra_info_t* s);

int apn_config_read(struct apn_config_t* s);
char* apn_config_convert_json_object(int val,struct apn_config_t* s);

int ntp_config_read(struct ntp_config_t* s);
char* ntp_config_convert_json_object(int val,struct ntp_config_t* s);



static int h_sim= 0;
static dm_client_handle_type h_dm = 0;
static nw_client_handle_type h_nw = 0;
static char *service_type[] = {"service none", "service limited", "service avaliable"};
/*
* @Description:打开串口
* @return-串口操作符
*/
int open_serial(){
	int fd = -1;
	fibo_uart_dcb_t uart_dcb;
	memset(&uart_dcb, 0x0, sizeof(uart_dcb));
	uart_dcb.flowctrl = E_FC_NONE;
	uart_dcb.baudrate = E_B_9600;
	fd = fibo_uart_open(FIBO_UART1_DEV, &uart_dcb);
	if (fd < 0)
    {
    	info_publish("fibo_uart_open failure!\n");
		int n = errno;
		printlogf(LOG_ERR, "fibo_uart_open failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
    }
	return fd;
}

void info_publish(char* pub_mes){
	mqtt_publish(pub_topic,pub_mes);
	if(DebugOpt){
		printlogf(LOG_NOTICE,"%s\n",pub_mes);
		printlogf(LOG_NOTICE, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
}

/*get function*/
#pragma region 0.base_info

char* base_info_convert_json_object(int val,struct base_info_t* base_info_s){
	char *pub_mes;cJSON *root,*info;
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "ac", cJSON_CreateString("getinfo"));
	cJSON_AddItemToObject(root, "val", cJSON_CreateNumber(val));
	cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
	
	cJSON_AddItemToObject(root, "info", info=cJSON_CreateObject());
	cJSON_AddStringToObject(info,"fver",base_info_s->fver);
	cJSON_AddStringToObject(info,"hver",base_info_s->hver);
	cJSON_AddStringToObject(info,"ccid",base_info_s->ccid);
	cJSON_AddStringToObject (info,"imsi",base_info_s->imsi);
	cJSON_AddStringToObject(info,"imei",base_info_s->imei);
	cJSON_AddStringToObject(info,"sn",base_info_s->sn);
	cJSON_AddStringToObject(info,"model",base_info_s->model);
	pub_mes=cJSON_Print(root);
	cJSON_Delete(root);
	return pub_mes;
}
int base_info_read(struct base_info_t* base_info_s){
	int ret;
	/*init sim*/
	ret = fibo_sim_client_init(&h_sim);
	/*ccid*/
	ret = fibo_sim_get_iccid(h_sim, E_FIBO_SIM_SLOT_ID_1, base_info_s->ccid, CCID_SIZE);
	/*imsi*/
	fibo_sim_app_id_info_t    t_info;
    t_info.e_slot_id    = E_FIBO_SIM_SLOT_ID_1;
    t_info.e_app        = E_FIBO_SIM_APP_TYPE_3GPP;
    ret =  fibo_sim_get_imsi(h_sim, &t_info, base_info_s->imsi, IMSI_SIZE);
	/*deinit sim*/
	ret = fibo_sim_client_deinit(h_sim);
	h_sim = 0;
	ret = fibo_dm_client_init(&h_dm);
	ret = fibo_dm_get_hw_version(h_dm, base_info_s->hver, HW_VERSION_SIZE);
	ret = fibo_dm_get_sdk_version(h_dm, base_info_s->fver, SDK_VERSION_SIZE);
	ret = fibo_dm_get_imei(h_dm, base_info_s->imei, IMEI_SIZE);
	ret = fibo_dm_get_sn(h_dm, base_info_s->sn, SN_SIZE);
	ret = fibo_dm_get_model_id(h_dm, base_info_s->model, MODEL_SIZE);
	ret = fibo_dm_client_deinit(h_dm);
	return ret;
}
void get_base_info(void*       	val){
	/*read info*/
	int ret;
	int a=*((int*)val);
	free(val);
		printlogf(LOG_DEBUG,"1.get_base_info val=%d \n",a);
	struct base_info_t *base_info_s=MDTU_COM_MALLOC_STRUCT(base_info_t);
	if (base_info_s == NULL) {
		printlogf(LOG_ERR,"0.don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	ret=base_info_read(base_info_s);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "base_info_read failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	/*convert into json*/
	char *pub_mes=base_info_convert_json_object(a,base_info_s);
	/*publish*/
	info_publish(pub_mes);
	free(base_info_s);
}
#pragma endregion ok

#pragma region 1.base_station_info
int base_station_info_read(struct base_station_info_t* base_station_info_s){
	/*1.apn*/
	int ret;
	fibo_data_call_config_info_t apn;
	uint32_t  profile_idx = PROFILE_ID;
	memset(&apn, 0,sizeof(fibo_data_call_config_info_t));
	ret = fibo_data_apn_get(profile_idx,  &apn);
	if(ret == FIBO_NO_ERR)
    {	 if(apn.apn_name_valid ==1)
        {
			base_station_info_s->apn=apn.apn_name;
		}else{
			base_station_info_s->apn=" ";
		}
    }
	/*2.ip*/
	/*"fibo_data_get_call_info"*/
	uint32_t  call_id = 1;
    uint32_t idx = 0;
    fibo_data_call_state_s call_info;
    char tmpBuf[ADDR_BUF_SIZE] = {0};
    fibo_data_call_addr_info_t *addr;
    char *ptr = NULL;
    int is_ipv6 = 0;
	memset(&call_info, 0,sizeof(fibo_data_call_state_s));
	ret = fibo_data_get_call_info(call_id, &call_info);
	if(ret == FIBO_NO_ERR)
    {
        for(idx = 0; idx < call_info.addr_info_list.addr_info_num; idx ++)
        {
            addr = &(call_info.addr_info_list.addr_info[idx]);

            ptr = addr2str(&addr->iface_addr_s, tmpBuf, sizeof(tmpBuf), &is_ipv6);
            if(ptr)
            {	
            	base_station_info_s->ip=ptr;
				if(DebugOpt)
					printlogf(LOG_NOTICE,"ipaddr : %s\n",ptr);
            }
            else
            {	
            	int n = errno;
				printlogf(LOG_ERR, "fibo_data_get_call_info failed reason:\n", strerror(n));
				printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
            	ret=-1;
            }
        }
    }
	/*3.lteStatus、operator、mcc、mnc*/
	ret = fibo_nw_client_init(&h_nw);
	fibo_nw_operator_name_info_t  t_info;
    ret = fibo_nw_get_operator_name(h_nw, &t_info);
	sprintf(base_station_info_s->operator,"%s(%s)",t_info.long_eons,t_info.short_eons);
	base_station_info_s->mcc=atoi(t_info.mcc);
	base_station_info_s->mnc=atoi(t_info.mnc);
	/*4.lteStatus*/
	fibo_nw_reg_status_info_cl_t         reg_status_info;
	memset(&reg_status_info, 0, sizeof(fibo_nw_reg_status_info_cl_t));
    ret = fibo_nw_get_reg_status(h_nw, &reg_status_info);
	base_station_info_s->lteStatus=service_type[reg_status_info.voice_registration.service_type];
	/*5.cid、pCID、txPower、lteBands ？*/
	base_station_info_s->lteBands="B2/B3";
	base_station_info_s->txPower=0;
	base_station_info_s->pCID=0;
	base_station_info_s->cid=0;
	/*close*/
	ret = fibo_nw_client_deinit(h_nw);
	if(DebugOpt){
		printf("ip=%s\n",base_station_info_s->ip);
		printf("apn=%s\n",base_station_info_s->apn);
		printf("operator=%s\n",base_station_info_s->operator);
		printf("lteBands=%s\n",base_station_info_s->lteBands);
		printf("lteStatus=%s\n",base_station_info_s->lteStatus);
		printf("txPower=%d\n",base_station_info_s->txPower);
		printf("cid=%d\n",base_station_info_s->cid);
		printf("mcc=%d\n",base_station_info_s->mcc);
		printf("mnc=%d\n",base_station_info_s->mnc);
	}
	
	return ret;
}
char* base_station_info_convert_json_object(int val,struct base_station_info_t* s){
	char *pub_mes;cJSON *root,*info;
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "ac", cJSON_CreateString("getinfo"));
	cJSON_AddItemToObject(root, "val", cJSON_CreateNumber(val));
	cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
	
	cJSON_AddItemToObject(root, "info", info=cJSON_CreateObject());
	cJSON_AddStringToObject(info,"ip",s->ip);
	cJSON_AddStringToObject(info,"apn",s->apn);
	cJSON_AddStringToObject(info,"operator",s->operator);
	cJSON_AddStringToObject (info,"lteBands",s->lteBands);
	cJSON_AddStringToObject(info,"lteStatus",s->lteStatus);
	cJSON_AddNumberToObject(info,"txPower",s->txPower);
	cJSON_AddNumberToObject(info,"pCID",s->pCID);
	cJSON_AddNumberToObject(info,"cid",s->cid);
	cJSON_AddNumberToObject(info,"mcc",s->mcc);
	cJSON_AddNumberToObject(info,"mnc",s->mnc);
	pub_mes=cJSON_Print(root);
	cJSON_Delete(root);
	return pub_mes;
}

void get_base_station_info(void* val){
	/*read info*/
	int ret;
	int a=*((int*)val);
	free(val);
	if(DebugOpt)
		printlogf(LOG_DEBUG,"1.get_base_station_info val=%d \n",a);
	struct base_station_info_t *base_station_info_s=MDTU_COM_MALLOC_STRUCT(base_station_info_t);
	if ((base_station_info_s) == NULL) {
		printlogf(LOG_ERR,"don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	ret=base_station_info_read(base_station_info_s);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "base_station_info_read failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	/*convert into json*/
	char *pub_mes=base_station_info_convert_json_object(a,base_station_info_s);
	/*publish*/
	info_publish(pub_mes);
	free(base_station_info_s);
}
#pragma endregion some params can not read 

#pragma region 2.signal_param


int signal_param_read(struct signal_param_t* s){
	int ret;
	s->ecio=0;s->rsrp=0;s->rsrq=0;
	s->rssi=0;s->sinr=0;s->snr=0;
	ret = fibo_nw_client_init(&h_nw);
	fibo_nw_signal_strength_info_t    t_info;
    memset(&t_info, 0, sizeof(fibo_nw_signal_strength_info_t));
    ret = fibo_nw_get_signal_strength(h_nw, &t_info);
	
	if(t_info.gsm_sig_info_valid)
    {
       s->rssi= t_info.gsm_sig_info.rssi;
    } 
	if(t_info.wcdma_sig_info_valid)
    {
       s->rssi= t_info.wcdma_sig_info.rssi;
	   s->ecio= t_info.wcdma_sig_info.ecio;
    } 
	if(t_info.tdscdma_sig_info_valid)
    {
       s->rssi= t_info.tdscdma_sig_info.rssi;
	   s->ecio= t_info.tdscdma_sig_info.ecio;
	   s->sinr= t_info.tdscdma_sig_info.sinr;
    } 
	if(t_info.lte_sig_info_valid)
    {
    	s->rssi= t_info.lte_sig_info.rssi;
		s->rsrq= t_info.lte_sig_info.rsrq;
		s->rsrp= t_info.lte_sig_info.rsrp;
		s->snr= t_info.lte_sig_info.snr;
    } 
    if(t_info.cdma_sig_info_valid)
    {
       s->rssi= t_info.cdma_sig_info.rssi;
	   s->ecio= t_info.cdma_sig_info.ecio;
    } 
	if(t_info.hdr_sig_info_valid)
    {
       s->rssi= t_info.hdr_sig_info.rssi;
	   s->ecio= t_info.hdr_sig_info.ecio;
	   s->sinr= t_info.hdr_sig_info.sinr;
    } 
	if(t_info.nr5g_sig_info_valid)
    {
    	s->rssi= t_info.nr5g_sig_info.rssi;
		s->rsrq= t_info.nr5g_sig_info.rsrq;
		s->rsrp= t_info.nr5g_sig_info.rsrp;
		s->snr= t_info.nr5g_sig_info.snr;
    } 
	
	ret = fibo_nw_client_deinit(h_nw);
	return ret;
}
char* signal_param_convert_json_object(int val,struct signal_param_t* s){
	char *pub_mes;cJSON *root,*info;
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "ac", cJSON_CreateString("getinfo"));
	cJSON_AddItemToObject(root, "val", cJSON_CreateNumber(val));
	cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
	
	cJSON_AddItemToObject(root, "info", info=cJSON_CreateObject());
	cJSON_AddNumberToObject(info,"ecio",s->ecio);
	cJSON_AddNumberToObject(info,"rsrp",s->rsrp);
	cJSON_AddNumberToObject(info,"rsrq",s->rsrq);
	cJSON_AddNumberToObject (info,"rssi",s->rssi);
	cJSON_AddNumberToObject(info,"sinr",s->sinr);
	cJSON_AddNumberToObject(info,"snr",s->snr);
	pub_mes=cJSON_Print(root);
	cJSON_Delete(root);
	return pub_mes;
}
void get_signal_param(void* val){
	/*read info*/
	int ret;
	int a=*((int*)val);
	free(val);
	if(DebugOpt)
		printlogf(LOG_DEBUG,"2.get_signal_param val=%d \n",a);
	struct signal_param_t *s=MDTU_COM_MALLOC_STRUCT(signal_param_t);
	if (s == NULL) {
		printlogf(LOG_ERR,"don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	ret=signal_param_read(s);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "signal_param_read failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	/*convert into json*/
	char *pub_mes=signal_param_convert_json_object(a,s);
	/*publish*/
	info_publish(pub_mes);
	free(s);
}

#pragma endregion ok

#pragma region 3.serial_info
int serial_info_read(struct serial_info_t* s){
	int ret;
	s->name="RS232";
	fibo_uart_dcb_t dcb;
	memset(&dcb, 0x0, sizeof(dcb));
	dcb.flowctrl = E_FC_NONE;
	/*open serial*/
	int fd=open_serial();
    if (fd >= 0)
 	{
		/*get serial info*/
		ret = fibo_uart_get_dcb(fd, &dcb);
		s->bps=dcb.baudrate;
		s->dbt=dcb.databit;
		s->pbt=dcb.parity;
		s->sbt=dcb.stopbit;
	}
	return ret;
}
char* serial_info_convert_json_object(int val,struct serial_info_t* s){
	char *pub_mes;cJSON *root,*param,*info_obj,*info_arr;
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "ac", cJSON_CreateString("getinfo"));
	cJSON_AddItemToObject(root, "val", cJSON_CreateNumber(val));
	cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));

	param=cJSON_CreateObject();
	cJSON_AddNumberToObject(param, "bps", s->bps);
	cJSON_AddNumberToObject(param, "dbt", s->dbt);
	cJSON_AddNumberToObject(param, "pbt", s->pbt);
	cJSON_AddNumberToObject(param, "sbt", s->sbt);
	
	info_obj=cJSON_CreateObject();
	cJSON_AddItemToObject(info_obj, "name", cJSON_CreateString(s->name));
	cJSON_AddItemToObject(info_obj, "param",param);
	
	info_arr=cJSON_CreateArray();
	cJSON_AddItemToArray(info_arr, info_obj);

	cJSON_AddItemToObject(root, "info", info_arr);
	
	pub_mes=cJSON_Print(root);
	cJSON_Delete(root);
	return pub_mes;
}

void get_serial_info(void* val){
	/*read info*/
	int ret;
	int a=*((int*)val);
	free(val);
	if(DebugOpt)
		printlogf(LOG_DEBUG,"3.get_serial_info val=%d \n",a);
	struct serial_info_t *s=MDTU_COM_MALLOC_STRUCT(serial_info_t);
	if (s == NULL) {
		printlogf(LOG_ERR,"don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	ret=serial_info_read(s);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "serial_info_read failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	/*convert into json*/
	char *pub_mes=serial_info_convert_json_object(a,s);
	/*publish*/
	info_publish(pub_mes);
	free(s);
}

#pragma endregion ok

#pragma region 4.extra_info
int extra_info_read(struct extra_info_t* s){

	s->bin=global_tcp_config_s.tcp_server_bin;
	s->port=global_tcp_config_s.tcp_server_port;
	s->timeout=global_tcp_config_s.tcp_server_timeout;
	s->tcptmr=global_tcp_config_s.tcp_server_tcptmr;
	s->schedule=global_tcp_config_s.tcp_server_schedule;
	return 0;
}
char* extra_info_convert_json_object(int val,struct extra_info_t* s){
	char *pub_mes;cJSON *root,*info;
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "ac", cJSON_CreateString("getinfo"));
	cJSON_AddItemToObject(root, "val", cJSON_CreateNumber(val));
	cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
	
	cJSON_AddItemToObject(root, "info", info=cJSON_CreateObject());
	cJSON_AddNumberToObject(info,"bin",s->bin);
	cJSON_AddNumberToObject(info,"port",s->port);
	cJSON_AddNumberToObject(info,"timeout",s->timeout);
	cJSON_AddNumberToObject (info,"tcptmr",s->tcptmr);
	cJSON_AddNumberToObject(info,"schedule",s->schedule);
	pub_mes=cJSON_Print(root);
	cJSON_Delete(root);
	return pub_mes;
}

void get_extra_info(void* val){
	/*read info*/
	int ret;
	int a=*((int*)val);
	free(val);
	if(DebugOpt)
		printlogf(LOG_DEBUG,"4.get_extra_info val=%d \n",a);
	struct extra_info_t *s=MDTU_COM_MALLOC_STRUCT(extra_info_t);
	if (s == NULL) {
		printlogf(LOG_ERR,"don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	ret=extra_info_read(s);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "extra_info_read failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	/*convert into json*/
	char *pub_mes=extra_info_convert_json_object(a,s);
	/*publish*/
	info_publish(pub_mes);
	free(s);
}

#pragma endregion ok

#pragma region 5.apn_config
int apn_config_read(struct apn_config_t* s){
	int ret;
	fibo_data_call_config_info_t apn;
	uint32_t  profile_idx = PROFILE_ID;
	memset(&apn, 0,sizeof(fibo_data_call_config_info_t));
	ret = fibo_data_apn_get(profile_idx,  &apn);
	if(ret == FIBO_NO_ERR)
    {	 if(apn.apn_name_valid ==1)
        {
			s->name=apn.apn_name;
		}else{
			s->name=" ";
		}
		 if(apn.user_name_valid ==1)
        {
        	s->user=apn.user_name;
			if(DebugOpt){
				printlogf(LOG_DEBUG,"fibo_data_apn_get usr name:%s\n", apn.user_name);
			}
        }else{
			s->user=" ";
		}
        if(apn.password_valid ==1)
        {
        	s->passwd=apn.password;
			if(DebugOpt){
				printlogf(LOG_DEBUG,"fibo_data_apn_get passsword:%s\n", apn.password);
			}
        }else{
			s->passwd=" ";
		}
    }
	return ret;
}
char* apn_config_convert_json_object(int val,struct apn_config_t* s){
	char *pub_mes;cJSON *root,*info;
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "ac", cJSON_CreateString("getinfo"));
	cJSON_AddItemToObject(root, "val", cJSON_CreateNumber(val));
	cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
	
	cJSON_AddItemToObject(root, "info", info=cJSON_CreateObject());
	cJSON_AddStringToObject(info,"name",s->name);
	cJSON_AddStringToObject(info,"user",s->user);
	cJSON_AddStringToObject(info,"passwd",s->passwd);
	pub_mes=cJSON_Print(root);
	cJSON_Delete(root);
	return pub_mes;
}

void get_apn_config(void* val){
	/*read info*/
	int ret;
	int a=*((int*)val);
	free(val);
	if(DebugOpt)
		printlogf(LOG_DEBUG,"5.get_apn_config val=%d \n",a);
	struct apn_config_t *s=MDTU_COM_MALLOC_STRUCT(apn_config_t);
	if (s == NULL) {
		printlogf(LOG_ERR,"don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	ret=apn_config_read(s);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "apn_config_read failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	/*convert into json*/
	char *pub_mes=apn_config_convert_json_object(a,s);
	/*publish*/
	info_publish(pub_mes);
	free(s);
}

#pragma endregion ok

#pragma region 6.ntp_config
int ntp_config_read(struct ntp_config_t* s){
	s->primary_ip=global_ntp_config_s.ntp_primary_ip;
	s->second_ip=global_ntp_config_s.ntp_second_ip;
	s->interval=global_ntp_config_s.ntp_interval;
	s->timezone=global_ntp_config_s.ntp_timezone;
	return 0;
}
char* ntp_config_convert_json_object(int val,struct ntp_config_t* s){
	char *pub_mes;cJSON *root,*info;
	root=cJSON_CreateObject();
	cJSON_AddItemToObject(root, "ac", cJSON_CreateString("getinfo"));
	cJSON_AddItemToObject(root, "val", cJSON_CreateNumber(val));
	cJSON_AddItemToObject(root, "code", cJSON_CreateNumber(0));
	
	cJSON_AddItemToObject(root, "info", info=cJSON_CreateObject());
	cJSON_AddStringToObject(info,"primary",s->primary_ip);
	cJSON_AddStringToObject(info,"second",s->second_ip);
	cJSON_AddNumberToObject(info,"interval",s->interval);
	cJSON_AddNumberToObject(info,"timezone",s->timezone);
	pub_mes=cJSON_Print(root);
	cJSON_Delete(root);
	return pub_mes;
}

void get_ntp_config(void* val){
	/*read info*/
	int ret;
	int a=*((int*)val);
	free(val);
	struct ntp_config_t *s=MDTU_COM_MALLOC_STRUCT(ntp_config_t);
	if (s == NULL) {
		printlogf(LOG_ERR,"don't have enough memory!\n");
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	ret=ntp_config_read(s);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "ntp_config_read failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
	}
	/*convert into json*/
	char *pub_mes=ntp_config_convert_json_object(a,s);
	/*publish*/
	info_publish(pub_mes);
	free(s);
}

#pragma endregion what this?





