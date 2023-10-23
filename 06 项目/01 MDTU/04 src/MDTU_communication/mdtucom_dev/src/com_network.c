
#include "fibo_oe.h"
#include "com_log.h"
#include "com_network.h"
#include "app_api.h"

/*board_network variable*/
static fibo_data_call_result_info_t result_info;
#define APN_NAME   "CMNET"
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
					printlogf(LOG_DEBUG, "DATA UP device name:%s,profile:%d,call_id:%d\n\n", data_call_ind->addr_info_list.iface_name,data_call_ind->profile_idx,data_call_ind->call_id);
				//mqtt_thread_run();
				break;
            }
        case E_FIBO_DATA_NET_DOWN_EVENT:
            {
            	if(DebugOpt)
					printlogf(LOG_DEBUG, "DATA UP device name:%s,profile:%d,call_id:%d\n\n", "DATA DOWN profile:%d,call_id:%d,call_state:%d\n",data_call_ind->profile_idx,data_call_ind->call_id, data_call_ind->call_state);
                break;
            }
        default:            
            break;
    }
}

int init_board_network(){
	int16_t ret;
	
	ret = fibo_sdk_init("127.0.0.1");
    if (ret != FIBO_NO_ERR)
    {
    	printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		exit(1);
    }
    ret = fibo_data_client_init(fibo_data_call_evt_cb);
	if(ret){
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
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
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		exit(1);
	}  	
   	memset(&result_info, 0,sizeof(fibo_data_call_result_info_t));
   	ret = fibo_data_call_start(PROFILE_ID, &result_info);
	if(ret){
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		exit(1);
	}  	
	sleep(3);/*waiting network init*/
	return ret;
}

int destory_board_network(){
	/*close network*/
	fibo_data_call_stop(result_info.call_id);
    fibo_data_client_deinit();
    fibo_sdk_deinit();
}


