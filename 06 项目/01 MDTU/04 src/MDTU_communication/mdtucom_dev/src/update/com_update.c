/*************************************************
Author:zhouBL
Version:
Description:app升级类方法
Others:
created date:2023/11/7 3:12 下午
modified date:
*************************************************/
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "fibo_type.h"

#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "com_log.h"
#include "com_update.h"
extern void main_thread_clean();
int8_t* tmpdir=TEMP_IMAGE_SAVE;
int8_t* exec_path=CUR_APP_PATH;
/*
* @Description:重启app(已弃用)
* @return-
*/
/*
int32_t restart_app(){
	int32_t pid,ret=0;
	pid=fork();
	switch (pid) {
		case -1: 	//error happend
			LOGC(LOG_ERR, "fork err:%s", strerror(errno));
			return -1;
		case 0: 	//child 
			{
			ret = execv(exec_path, wargv);
			break;
		}
		default:	//parent 
			{
			wait(NULL);
			if(ret){
				LOGC(LOG_NOTICE, "restart failed ,try again after 5 seconds.");
				sleep(5);
				ret = execv(exec_path, wargv);
			}
			return ret;
			}
	}
	return ret;
}
*/

/*
* @Description:赋权、移动、重启app
* @return- none
*/
void* update_operation(){
	int32_t ret;
	struct stat current_st;
	ret = stat(exec_path, &current_st);
	ret = chmod(tmpdir, current_st.st_mode | S_IXUSR);
	ret = rename(tmpdir, exec_path);
	LOGC(LOG_NOTICE, "update_operation rename ret:%d", ret);
	if(ret){
		LOGC(LOG_ERR, "update_operation failed,reason:%s", strerror(errno));
		return ;
	}
	main_thread_clean();
	/*restart*/
	int32_t re_flag;
	do{
		re_flag=execv(exec_path, wargv);
		if(re_flag){
			sleep(5);
		}
	}while(re_flag);
}




