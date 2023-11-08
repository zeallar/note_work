/*************************************************
Author:zhouBL
Version:
Description:信息设置类方法
Others:
created date:2023/11/7 3:18 下午
modified date:
*************************************************/
#include "MQTTLinux.h"
#include "MQTTClient.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>   
#include <netinet/in.h>   
#include <arpa/inet.h> 
#include <sys/wait.h>

#include "fibo_oe.h"
#include "fibo_type.h"
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "fibo_uart.h"
#include "thpool.h"
#include "cJSON.h"
#include "info_set.h"
#include "mqtt.h"
#include "com_utils.h"
#include "com_log.h"
#include "com_network.h"
#include "def_structs.h"
#include "ini.h"
#include "http.h"
#include "com_update.h"
#include "md5.h"
#include "schedule_tasks.h"
#include "crontasks.h"

#define ATC_REQ_RESP_CMD_MAX_LEN     128
pthread_t update_th;

typedef struct{
	int8_t model[32];
	int8_t sn[32];
	int8_t key[64];
	uint32_t hver;
}set_sn_t;

typedef struct{
	int8_t name[32];
	int8_t user[32];
	int8_t passwd[64];
}set_apn_t;
void parse_serial_json(void *s,int8_t* text);
int32_t exec_set_serial_operation(void *s);

void parse_general_json(void *s,int8_t* text);
int32_t exec_set_general_operation(void *s);

void parse_sn_json(void *s,int8_t* text);
int32_t exec_set_sn_operation(void *s);

void parse_apn_json(void *s,int8_t* text);
int32_t exec_set_apn_operation(void *s);

void parse_ntp_json(void *s,int8_t* text);
int32_t exec_set_ntp_operation(void *s);

void parse_link_json(void *s,int8_t* text);
int32_t exec_set_link_operation(void *s);

void parse_bin_json(void *s,int8_t* text);
int32_t exec_set_bin_operation(void *s);

void parse_update_json(void *s,int8_t* text);
int32_t exec_set_update_operation(void *s);


static int8_t atc_cmd_req[ATC_REQ_RESP_CMD_MAX_LEN]    = {0};
static int8_t atc_cmd_resp[ATC_REQ_RESP_CMD_MAX_LEN]  = {0};
static int8_t atc_cmd_tmp[ATC_REQ_RESP_CMD_MAX_LEN]  = {0};

void clean_int8_t_arr(){
	memset(atc_cmd_req,0,ATC_REQ_RESP_CMD_MAX_LEN);
	memset(atc_cmd_resp,0,ATC_REQ_RESP_CMD_MAX_LEN);
	memset(atc_cmd_tmp,0,ATC_REQ_RESP_CMD_MAX_LEN);
}


int32_t exec_shell_command(void* text) {
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t*)text;
	int32_t o_fd;
	int32_t e_fd;
	/* 复制标准输出描述符 */
	//o_fd = dup(STDOUT_FILENO);
	//e_fd = dup(STDOUT_FILENO);

	int32_t stdout_pipe[2], stderr_pipe[2];
	pid_t pid;
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) {
        perror("pipe");
        return -1;
    }

    pid = fork();
    if (pid == 0) {
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

		
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout");
            return -1;
        }

        if (dup2(stderr_pipe[1], STDERR_FILENO) == -1) {
            perror("dup2 stderr");
            return -1;
        }
		
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
       // execl(shell, shell, command, NULL);
       //printf("command=%s\n",command);
       execl("/bin/sh", "/bin/sh", "-c", s->json_text, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
        return -1;
    } else {
       int8_t buffer[4096];
	   memset(buffer, '\0', sizeof(buffer));
        int32_t nbytes;

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
		
		/* 重定向标准输出到usb */
		//dup2(s->fd_usb,STDOUT_FILENO);
		//dup2(s->fd_usb,STDERR_FILENO);
		
        while ((nbytes = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
            switch (s->flag)
				{
				case 0:{
					memset(s->json_text,0,sizeof(s->json_text));
					strcpy(s->json_text,buffer);
					//sprintf(s->json_text,"\n%.*s",(int32_t)strlen(buffer),(int8_t*)buffer);
					break;
				}
				case 1:
					write(s->sock, buffer, nbytes);
					break;
				case 2:
					write(s->fd_usb, buffer, nbytes);
					break;
				default:
					break;
				}
        }

        while ((nbytes = read(stderr_pipe[0], buffer, sizeof(buffer))) > 0) {
            switch (s->flag)
				{
				case 0:{
					memset(s->json_text,0,sizeof(s->json_text));
					strcpy(s->json_text,buffer);
					break;
				}
				case 1:
					write(s->sock, buffer, nbytes);
					break;
				case 2:
					write(s->fd_usb, buffer, nbytes);
					break;
				default:
					break;
				}
        }
		
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
		
		
        int32_t status;
        pid_t wpid = waitpid(pid, &status, 0);
        if (wpid == -1) {
            perror("waitpid");
            return -1;
        }
       if (WIFEXITED(status) || (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL)){
		/* 重定向恢复标准输出 */
	   /*
		dup2(o_fd,STDOUT_FILENO);
		dup2(e_fd,STDERR_FILENO);
	   	close(o_fd);
		close(e_fd);*/
			return 0;
		}
        return -1;
    }
}


/*set function*/
#pragma region 0.serial
void parse_serial_json(void *s,int8_t* text){
	fibo_uart_dcb_t *dcb=(fibo_uart_dcb_t*)s;
	/*parse json*/
	cJSON *json,*value,*arr;int8_t* out;
	json=cJSON_Parse(text);
	value = cJSON_GetObjectItem(json,"name");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	if(!(strncmp(out,"RS232",5))){
		arr=cJSON_GetObjectItem(json,"val");
		value=cJSON_GetObjectItem(arr,"dbt");
		out=cJSON_Print(value);
		dcb->databit=atoi(out);
		
		value=cJSON_GetObjectItem(arr,"sbt");
		out=cJSON_Print(value);
		dcb->stopbit=atoi(out);

		value=cJSON_GetObjectItem(arr,"pbt");
		out=cJSON_Print(value);
		dcb->parity=atoi(out);

		value=cJSON_GetObjectItem(arr,"bps");
		out=cJSON_Print(value);
		dcb->baudrate=atoi(out);
	}
	cJSON_Delete(json);
	
}
int32_t exec_set_serial_operation(void *s){
	int32_t ret=0;
	fibo_uart_dcb_t *dcb=(fibo_uart_dcb_t*)s;
	/*open serial*/
	int32_t fd=open_serial();
    if(fd>=0){
		/*set serial*/
		ret = fibo_uart_set_dcb(fd, dcb);
	}else{
		ret=-1;
	}
	if(DebugOpt){
		ret = fibo_uart_get_dcb(fd, dcb);
    	LOGC(LOG_NOTICE,"GET DCB ret: %d: baudtate: %d flowctrl: %d databit: %d stopbit: %d parity: %d ", ret, dcb->baudrate, 
    	dcb->flowctrl, dcb->databit, dcb->stopbit, dcb->parity);
	}
	return ret;
}

void set_serial_info(void* text){
	int32_t ret;int8_t* pub_mes;
	struct thread_info_transmit_t *thread_temp_param_s=(struct thread_info_transmit_t*)text;
	/*parse json*/
	fibo_uart_dcb_t dcb;
	memset(&dcb, 0x0, sizeof(dcb));
	dcb.flowctrl = E_FC_NONE;
	parse_serial_json(&dcb,thread_temp_param_s->json_text);
	/*execute set*/
	ret=exec_set_serial_operation(&dcb);
	memset(thread_temp_param_s->json_text,0,strlen(thread_temp_param_s->json_text));
	if(ret){
		LOGC(LOG_ERR, "set_serial_info failed reason:", strerror(errno));
		pub_mes="set_serial_info failed!\n";
		goto end;
	}
	/*publish*/
	pub_mes="set_serial_info OK!\n";
	end:
	strcpy(thread_temp_param_s->json_text,pub_mes);
	/*publish*/
	select_handler_method(thread_temp_param_s);
}
#pragma endregion ok

#pragma region 1.general
void parse_general_json(void *s,int8_t* text){
	/*parse json*/
	global_tcp_config_t* _s=(global_tcp_config_t*)s;
	cJSON *json,*value,*arr;int8_t* out;
	json=cJSON_Parse(text);

	arr=cJSON_GetObjectItem(json,"val");
	value=cJSON_GetObjectItem(arr,"port");
	out=cJSON_Print(value);
	_s->tcp_server_port=atoi(out);
	
	value=cJSON_GetObjectItem(arr,"bin");
	out=cJSON_Print(value);
	_s->tcp_server_bin=atoi(out);

	value=cJSON_GetObjectItem(arr,"timeout");
	out=cJSON_Print(value);
	_s->tcp_server_timeout=atoi(out);

	value=cJSON_GetObjectItem(arr,"tcptmr");
	out=cJSON_Print(value);
	_s->tcp_server_tcptmr=atoi(out);

	value=cJSON_GetObjectItem(arr,"schedule");
	out=cJSON_Print(value);
	_s->tcp_server_schedule=atoi(out);
	cJSON_Delete(json);
}

int32_t exec_set_general_operation(void *s){
	int32_t ret=0;
	global_tcp_config_t* _s=(global_tcp_config_t*)s;/**/
	ret=ini_put_int("tcp_server","bin", _s->tcp_server_bin);
	ret=ini_put_int("tcp_server","port", _s->tcp_server_port);
	ret=ini_put_int("tcp_server","timeout",_s->tcp_server_timeout);
	ret=ini_put_int("tcp_server","tcptmr",_s->tcp_server_tcptmr);
	ret=ini_put_int("tcp_server","schedule",_s->tcp_server_schedule);
	return ret;
}
void set_general_info(void* text){
	int32_t ret;int8_t* pub_mes;
	struct thread_info_transmit_t *thread_temp_param_s=(struct thread_info_transmit_t*)text;
	/*parse json*/
	extern global_tcp_config_t global_tcp_config_s;
	memset(&global_tcp_config_s,0,sizeof(global_tcp_config_t));
	parse_general_json(&global_tcp_config_s,thread_temp_param_s->json_text);
	/*execute set*/
	ret=exec_set_general_operation(&global_tcp_config_s);
	memset(thread_temp_param_s->json_text,0,strlen(thread_temp_param_s->json_text));
	if(ret){
		
		LOGC(LOG_ERR, "exec_set_general_operation failed reason:", strerror(errno));
		
		pub_mes="exec_set_general_operation failed!\n";
		goto end;
	}
	/*publish*/
	pub_mes="exec_set_general_operation OK!\n";
	end:
	strcpy(thread_temp_param_s->json_text,pub_mes);
	//printf("thread_temp_param_s->json_text=%s\n",thread_temp_param_s->json_text);
	select_handler_method((void*)thread_temp_param_s);
	
}
#pragma endregion ok whether need to restart tcp server when set after

#pragma region 2.sn
void parse_sn_json(void *s,int8_t* text){
	set_sn_t* _s=(set_sn_t*)s;
	cJSON *json,*value,*arr;int8_t* out;
	json=cJSON_Parse(text);

	arr=cJSON_GetObjectItem(json,"val");
	value=cJSON_GetObjectItem(arr,"model");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->model,out);
	
	value=cJSON_GetObjectItem(arr,"sn");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->sn,out);

	value=cJSON_GetObjectItem(arr,"key");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->key,out);

	value=cJSON_GetObjectItem(arr,"hver");
	out=cJSON_Print(value);
	_s->hver=atoi(out);
	
	cJSON_Delete(json);
}

int32_t exec_set_sn_operation(void *s){
	int32_t ret;
	set_sn_t* _s=(set_sn_t*)s;
	/*md5校验imei值是否一致*/
	/*获取imei*/
	int8_t imei_val[128]={0};
	dm_client_handle_type f_dm = 0;
	ret = fibo_dm_client_init(&f_dm);
	ret = fibo_dm_get_imei(f_dm, imei_val, sizeof(imei_val));
	ret = fibo_dm_client_deinit(f_dm);
	error_unless(ret==0);
	/*md5加密imei*/
	int32_t len=strlen(imei_val);
	int32_t i;
	uint8_t result[16];
	int8_t buf[64]={'\0'};
	int8_t tmp[3]={'\0'};
	 for (i = 0; i < 1000000; i++) {
        md5((uint8_t*)imei_val, len, result);
    }
 	 // display result
 	memset(imei_val,0,strlen(imei_val));
    for (i = 0; i < 16; i++){
		sprintf(tmp,"%2.2x", result[i]);
		strcat(buf,tmp);
	}
 	/*对比key*/
	log_test("buf:%s,%d,key:%s,%d", buf,strlen(buf), _s->key,strlen(_s->key));
	if(strncmp(buf,_s->key,32)==0){
		/*更新sn*/
		ini_put_str("sn","model",_s->model);
		ini_put_str("sn","sn",_s->sn);
		ini_put_int("sn","hver",_s->hver);
		return 0;
	}
	else{
		return -1;
	}
	error:
	return ret;
}
void set_sn_info(void* text){
	int32_t ret;int8_t* pub_mes;
	struct thread_info_transmit_t *thread_temp_param_s=(struct thread_info_transmit_t*)text;
	/*parse json*/
	set_sn_t sn_s;
	parse_sn_json(&sn_s,thread_temp_param_s->json_text);
	/*execute set*/
	ret=exec_set_sn_operation(&sn_s);
	memset(thread_temp_param_s->json_text,0,strlen(thread_temp_param_s->json_text));
	if(ret){
		
		LOGC(LOG_ERR, "exec_set_sn_operation failed reason:", strerror(errno));
		
		pub_mes="exec_set_sn_operation failed!\n";
		goto end;
	}
	/*publish*/
	pub_mes="exec_set_sn_operation OK!\n";
	end:
	strcpy(thread_temp_param_s->json_text,pub_mes);
	/*publish*/
	select_handler_method((void*)thread_temp_param_s);
}
#pragma endregion AT command invalid

#pragma region 3.apn
void parse_apn_json(void *s,int8_t* text){
	set_sn_t* _s=(set_sn_t*)s;
	cJSON *json,*value,*arr;int8_t* out;
	json=cJSON_Parse(text);

	arr=cJSON_GetObjectItem(json,"val");
	value=cJSON_GetObjectItem(arr,"name");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->model,out);
	
	value=cJSON_GetObjectItem(arr,"user");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->sn,out);

	value=cJSON_GetObjectItem(arr,"passwd");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->key,out);
	cJSON_Delete(json);
}
int32_t exec_set_apn_operation(void *s){
	int32_t ret ;set_apn_t* _s=(set_apn_t*)s;
	ret=ini_put_str("apn","name",_s->name);
	ret=ini_put_str("apn","user",_s->user);
	ret=ini_put_str("apn","passwd",_s->passwd);
}
void set_apn_info(void* text){
	int32_t ret;int8_t* pub_mes;
	struct thread_info_transmit_t *thread_temp_param_s=(struct thread_info_transmit_t*)text;
	/*parse json*/
	set_apn_t apn_s;
	parse_apn_json(&apn_s,thread_temp_param_s->json_text);
	/*execute set*/
	ret=exec_set_apn_operation(&apn_s);
	memset(thread_temp_param_s->json_text,0,strlen(thread_temp_param_s->json_text));
	if(ret){
		
		LOGC(LOG_ERR, "set_apn_info failed reason:", strerror(errno));
		
		pub_mes="set_apn_info failed!\n";
		goto end;
	}
	/*publish*/
	pub_mes="set_apn_info OK!\n";
	end:
	strcpy(thread_temp_param_s->json_text,pub_mes);
	/*publish*/
	select_handler_method((void*)thread_temp_param_s);
}
#pragma endregion AT command invalid

#pragma region 4.ntp
void parse_ntp_json(void *s,int8_t* text){
	global_ntp_config_t* _s=(global_ntp_config_t*)s;
	cJSON *json,*value,*arr;int8_t* out;
	json=cJSON_Parse(text);

	arr=cJSON_GetObjectItem(json,"val");
	value=cJSON_GetObjectItem(arr,"primary");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->ntp_primary_ip,out);
	
	value=cJSON_GetObjectItem(arr,"second");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->ntp_second_ip,out);

	value=cJSON_GetObjectItem(arr,"interval");
	out=cJSON_Print(value);
	_s->ntp_interval=atoi(out);

	value=cJSON_GetObjectItem(arr,"timezone");
	out=cJSON_Print(value);
	_s->ntp_timezone=atoi(out);
	cJSON_Delete(json);
}
int32_t exec_set_ntp_operation(void *s){
	int32_t ret=0;
	global_ntp_config_t* _s=(global_ntp_config_t*)s;/**/
	
	ret=ini_put_str("ntp_config","primary_ip",_s->ntp_primary_ip);
	ret=ini_put_str("ntp_config","second_ip",_s->ntp_second_ip);
	ret=ini_put_int("ntp_config","interval",_s->ntp_interval);
	ret=ini_put_int("ntp_config","timezone",_s->ntp_timezone);
	
	/*modefy timezone file location /etc/TZ*/
}

void set_ntp_info(void* text){
	int32_t ret;int8_t* pub_mes;
	struct thread_info_transmit_t *thread_temp_param_s=(struct thread_info_transmit_t*)text;
	/*parse json*/
	memset(&global_ntp_config_s,0,sizeof(global_ntp_config_t));
	parse_ntp_json(&global_ntp_config_s,thread_temp_param_s->json_text);
	/*execute set*/
	ret=exec_set_ntp_operation(&global_ntp_config_s);
	memset(thread_temp_param_s->json_text,0,strlen(thread_temp_param_s->json_text));
	ntp_sync_time(0,NULL);
	if(ret){
		
		LOGC(LOG_ERR, "exec_set_ntp_operation failed reason:", strerror(errno));
		
		pub_mes="exec_set_ntp_operation failed!\n";
		goto end;
	}
	/*publish*/
	pub_mes="exec_set_ntp_operation OK!\n";
	end:
	strcpy(thread_temp_param_s->json_text,pub_mes);
	select_handler_method((void*)thread_temp_param_s);
}
#pragma endregion ok

#pragma region 5.link
void parse_link_json(void *s,int8_t* text){

}
int32_t exec_set_link_operation(void *s){
	return 0;
}
void set_link_info(void* text){

}
#pragma endregion to do shell experiment

#pragma region 6.bin
void parse_bin_json(void *s,int8_t* text){

}
int32_t exec_set_bin_operation(void *s){
return 0;
}
void set_bin_info(void* text){

}
#pragma endregion to do 

#pragma region 7.update
void parse_update_json(void *s,int8_t* text){
	global_update_config_t* _s=(global_update_config_t*)s;
	cJSON *json,*value,*arr;int8_t* out;
	json=cJSON_Parse(text);

	arr=cJSON_GetObjectItem(json,"val");
	value=cJSON_GetObjectItem(arr,"url");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->url,out);
	
	value=cJSON_GetObjectItem(arr,"model");
	out=cJSON_Print(value);
	remove_double_quotation_marks(out);
	strcpy(_s->model,out);

	value=cJSON_GetObjectItem(arr,"ver");
	out=cJSON_Print(value);
	_s->ver=atoi(out);
	cJSON_Delete(json);
}
int32_t exec_set_update_operation(void *_s){
	int32_t ret=0;
	global_update_config_t* s=(global_update_config_t*)_s;
	ret=http_download(s->url,TEMP_IMAGE_SAVE);
	LOGC(LOG_NOTICE,"http_download ret=%d.",ret);
	if(ret){
		 LOGC(LOG_ERR,"http_download failed! please try again later.");
		 return ret;
	}
	ret=check_file_crc32();
	if(ret){
		 LOGC(LOG_ERR,"update file crc32 check failed! please try again later.");
		 return ret;
	}
	/*启动更新线程，更新完成后重启启动app*/
	LOGC(LOG_NOTICE,"to update app.");
	usleep(10);/*防止文件读写冲突*/
	/*异步线程处理升级，等待主线程任务结束后再开始升级*/
	if(pthread_create(&update_th, NULL, update_operation, NULL) < 0)
    {
    	LOGC(LOG_ERR, "creating tcp thread failed!");
        return -1;
    }
	return ret;
}
void set_update_info(void* text){
	int32_t ret;int8_t* pub_mes;
	struct thread_info_transmit_t *thread_temp_param_s=(struct thread_info_transmit_t*)text;
	/*parse json*/
	global_update_config_t global_update_config_s;
	memset(&global_update_config_s,0,sizeof(global_update_config_t));
	parse_update_json(&global_update_config_s,thread_temp_param_s->json_text);
	/*execute set*/
	ret=exec_set_update_operation(&global_update_config_s);
	memset(thread_temp_param_s->json_text,0,strlen(thread_temp_param_s->json_text));
	if(ret){
		
		LOGC(LOG_ERR, "exec_set_update_operation failed reason:", strerror(errno));
		
		pub_mes="exec_set_update_operation failed!\n";
		goto end;
	}
	/*publish*/
	pub_mes="exec_set_update_operation OK!\n";
	end:
	strcpy(thread_temp_param_s->json_text,pub_mes);
	select_handler_method((void*)thread_temp_param_s);
}
#pragma endregion to do 

#pragma region 8.restart

void set_restart_info(void* text){
	int32_t ret;int8_t* pub_mes;
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t*)text;
	ret = fibo_reboot_dev();
	if(ret){
		LOGC(LOG_ERR, "set_restart_info failed reason:", strerror(errno));
	}
	/*publish*/
	/*调用库函数实现*/
	list_node_t *list_node_s= list_find(task_list, &(s->task_id));
	list_remove(task_list,list_node_s);
}

#pragma endregion ok

#pragma region 9.restore

void set_restore_info(void* text){
	int32_t ret;int8_t* pub_mes;
	struct thread_info_transmit_t *thread_temp_param_s=(struct thread_info_transmit_t*)text;
	/*execute set*/
	/*删除配置文件*/
	ret=remove(MDTU_COM_CONFIG_PATH);
	/*恢复默认配置*/
	init_ini();
	memset(thread_temp_param_s->json_text,0,strlen(thread_temp_param_s->json_text));
	if(ret){
		LOGC(LOG_ERR, "set_restore_info failed reason:", strerror(errno));
		pub_mes="set_restore_info failed!\n";
		goto end;
	}
	/*publish*/
	pub_mes="set_restore_info OK!\n";
	end:
	strcpy(thread_temp_param_s->json_text,pub_mes);
	select_handler_method((void*)thread_temp_param_s);
}
#pragma endregion where is the command

#pragma region 10.usb linkage shell operation



void shell_op(void* text){
	int32_t ret;
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t*)text;
	ret=exec_shell_command(s);
	//LOGC(LOG_NOTICE,"shell return content:\n%s",s->json_text);
	if(ret){
		LOGC(LOG_ERR, "shell_op failed reason:", strerror(errno));
	}
	/*publish*/
	select_handler_method(s);

}
#pragma endregion




