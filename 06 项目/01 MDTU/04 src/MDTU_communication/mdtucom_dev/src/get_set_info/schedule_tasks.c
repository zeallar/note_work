/*************************************************
Author:zhouBL
Version:
Description:cron任务类方法
Others:
created date:2023/11/7 3:23 下午
modified date:
*************************************************/
#include "fibo_oe.h"
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "ntpclient.h"
#include "com_log.h"
#include "def_structs.h"
/*
* @Description:ntp时间同步
* @return- none
*/
void ntp_sync_time(uint32_t clientreg,void *clientarg){
	/*ntp sync time*/
	/*更新时区*/
	FILE* fp = fopen(TIMEZONE_PATH, "w");
	if (NULL == fp)
	{
		LOGC(LOG_ERR,"time zone file open failed!");
		return;
	}
	if(global_ntp_config_s.ntp_timezone==8){
		fprintf(fp,"CST-8");/*中国标准时间*/
	}else if(global_ntp_config_s.ntp_timezone==4){
		fprintf(fp,"CST-4");/*古巴标准时间*/
	}
	fclose(fp);
	/*ntp同步时间*/
	ntp_operate(global_ntp_config_s.ntp_primary_ip);
	if(!ntpTimeSync){
		ntp_operate(global_ntp_config_s.ntp_second_ip);
	}
	if(!ntpTimeSync){
			LOGC(LOG_ERR,"ntp sync failed!");
			return;
		}else{
			LOGC(LOG_NOTICE,"ntp sync success!");
		}
	LOGC(LOG_NOTICE,"ntp sync:%d!",ntpTimeSync);
}

