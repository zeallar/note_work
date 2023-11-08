/*************************************************
Author:zhouBL
Version:
Description:网络类方法
Others:
created date:2023/11/7 3:12 下午
modified date:
*************************************************/
#include "fibo_oe.h"
#include "com_log.h"
#include "com_network.h"
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t


/*board_network variable*/
static fibo_data_call_result_info_t result_info;
#define APN_NAME   "CMNET"
/*
* @Description:执行at命令
* @param1- cmd:
* @return- 0:success,othre:failed
*/
int32_t exec_at_command(int8_t *cmd){
	int32_t at_ret;int8_t atc_cmd_resp[512]={0};
	at_ret=Fibo_Send_AT_Cmd(cmd, atc_cmd_resp, sizeof(atc_cmd_resp));
	LOGC(LOG_NOTICE,"atc_cmd_resp=%s,ret:%d ",atc_cmd_resp,at_ret);
	return at_ret;
}

/*
* @Description:fibo_data_client_init回调函数
* @return- none
*/
void fibo_data_call_evt_cb(e_fibo_data_net_event_msg_id_t ind_flag,
    void                  *ind_msg_buf,
    uint32_t              ind_msg_len)
{
    fibo_data_call_indication_t *data_call_ind = (void *)ind_msg_buf;
	
    switch(ind_flag)
    {
        case E_FIBO_DATA_NET_UP_EVENT:
            {
                sleep(2);  //wait for modem do netcard cfg
                if(DebugOpt)
					LOGC(LOG_DEBUG, "DATA UP device name:%s,profile:%d,call_id:%d", data_call_ind->addr_info_list.iface_name,data_call_ind->profile_idx,data_call_ind->call_id);
				//mqtt_thread_run();
				break;
            }
        case E_FIBO_DATA_NET_DOWN_EVENT:
            {
            	if(DebugOpt)
					LOGC(LOG_DEBUG, "DATA UP device name:%s,profile:%d,call_id:%d", "DATA DOWN profile:%d,call_id:%d,call_state:%d\n",data_call_ind->profile_idx,data_call_ind->call_id, data_call_ind->call_state);
                break;
            }
        default:            
            break;
    }
    
}
/*
* @Description:初始化网络
* @return- 0:success 1:failed
*/
int32_t init_board_network(){
	int16_t ret;
	ret = fibo_sdk_init("127.0.0.1");
    if (ret != FIBO_NO_ERR)
    {
    	LOGC(LOG_ERR, "err");
		exit(1);
    }
	ret=exec_at_command("AT+MIPCALL=1");
    ret = fibo_data_client_init(fibo_data_call_evt_cb);
	if(ret){
		LOGC(LOG_ERR, "err");
		exit(1);
	}
	fibo_data_call_config_info_t apn;
	memset(&apn, 0, sizeof(fibo_data_call_config_info_t));
	apn.profile_idx = PROFILE_ID;
	apn.profile_valid = 1;
	apn.ip_family = E_FIBO_DATA_IP_FAMILY_IPV4;
	apn.ip_family_valid = 1;
	strncpy(apn.apn_name, APN_NAME, strlen(APN_NAME));
	apn.apn_name_valid = 1;
	ret = fibo_data_apn_set(&apn);
	if(ret){
		LOGC(LOG_ERR, "err");
		exit(1);
	}  	
   	memset(&result_info, 0,sizeof(fibo_data_call_result_info_t));
   	ret = fibo_data_call_start(PROFILE_ID, &result_info);
	if(ret){
		LOGC(LOG_ERR, "err");
		exit(1);
	}  	
	sleep(3);/*waiting network init*/
	return ret;
}
/*
* @Description:清理网络
* @return- none
*/
void destory_board_network(){
	/*close network*/
	exec_at_command("AT+MIPCALL=0");
	fibo_data_call_stop(result_info.call_id);
    fibo_data_client_deinit();
    fibo_sdk_deinit();
}



