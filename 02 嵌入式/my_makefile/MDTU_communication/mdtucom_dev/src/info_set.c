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
#include "fibo_uart.h"
#include "thpool.h"
#include "cJSON.h"
#include "info_set.h"
#include "mqtt.h"
#include "com_utils.h"
#include "com_log.h"
#include "com_network.h"
#include "app_api.h"
#include "def_structs.h"
#include "ini.h"
#include "ntpclient.h"

#define ATC_REQ_RESP_CMD_MAX_LEN     128
#define SHELL_RECEIVE_MAX_LEN     2048

static char atc_cmd_req[ATC_REQ_RESP_CMD_MAX_LEN]    = {0};
static char atc_cmd_resp[ATC_REQ_RESP_CMD_MAX_LEN]  = {0};
static char atc_cmd_tmp[ATC_REQ_RESP_CMD_MAX_LEN]  = {0};

extern int open_serial();
extern void info_publish(char* pub_mes);


void clean_char_arr(){
	memset(atc_cmd_req,0,ATC_REQ_RESP_CMD_MAX_LEN);
	memset(atc_cmd_resp,0,ATC_REQ_RESP_CMD_MAX_LEN);
	memset(atc_cmd_tmp,0,ATC_REQ_RESP_CMD_MAX_LEN);
}

int exec_shell_command(const char *command,char* buffer,int buf_len) {
    int stdout_pipe[2], stderr_pipe[2];
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
       execl("/bin/sh", "/bin/sh", "-c", command, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
        return -1;
    } else {
        //int nbytes=0;
		int remain_len = buf_len;
   		int recv_len = 0;
		
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
	
		while (remain_len>0){
			recv_len = read(stdout_pipe[0], buffer, remain_len);
			if(recv_len<=0){break;}
			if(recv_len>0){
				remain_len-=recv_len;
				buffer+=recv_len;
			}
		}

		while (remain_len>0){
			recv_len = read(stderr_pipe[0], buffer, remain_len);
			if(recv_len<=0){break;}
			if(recv_len>0){
				remain_len-=recv_len;
				buffer+=recv_len;
			}
		}

        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        int status;
        pid_t wpid = waitpid(pid, &status, 0);
        if (wpid == -1) {
            perror("waitpid");
            return -1;
        }
       if (WIFEXITED(status) || (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL)){
			return 0;
		}
        return -1;
    }
}

/*set function*/
#pragma region 0.serial
void parse_serial_json(void *s,char* text){
	fibo_uart_dcb_t *dcb=(fibo_uart_dcb_t*)s;
	/*parse json*/
	cJSON *json,*value,*arr;char* out;
	json=(cJSON *)text;
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
	
}
int exec_set_serial_operation(void *s){
	int ret=0;
	fibo_uart_dcb_t *dcb=(fibo_uart_dcb_t*)s;
	/*open serial*/
	int fd=open_serial();
    if(fd>=0){
		/*set serial*/
		ret = fibo_uart_set_dcb(fd, dcb);
	}else{
		ret=-1;
	}
	if(DebugOpt){
		ret = fibo_uart_get_dcb(fd, dcb);
    	printf("GET DCB ret: %d: baudtate: %d flowctrl: %d databit: %d stopbit: %d parity: %d \n", ret, dcb->baudrate, 
    	dcb->flowctrl, dcb->databit, dcb->stopbit, dcb->parity);
	}
	return ret;
}

void set_serial_info(void* text){
	int ret;
	/*parse json*/
	fibo_uart_dcb_t dcb;
	memset(&dcb, 0x0, sizeof(dcb));
	dcb.flowctrl = E_FC_NONE;
	parse_serial_json(&dcb,text);
	/*execute set*/
	ret=exec_set_serial_operation(&dcb);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "exec_set_operationfailed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		return;
	}
	/*publish*/
	info_publish("set_serial_info OK!\n");
	cJSON_Delete((cJSON*)text);
}
#pragma endregion ok

#pragma region 1.general
void parse_general_json(void *s,char* text){
	/*parse json*/
	global_tcp_config_t* _s=(global_tcp_config_t*)s;
	cJSON *json,*value,*arr;char* out;
	json=(cJSON *)text;

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
	
}

int exec_set_general_operation(void *s){
	int ret=0;
	global_tcp_config_t* _s=(global_tcp_config_t*)s;/**/
	ret = ini_putl("tcp_server", "bin", _s->tcp_server_bin, MDTU_COM_CONFIG_PATH);
	ret = ini_putl("tcp_server", "port", _s->tcp_server_port, MDTU_COM_CONFIG_PATH);
	ret = ini_putl("tcp_server", "timeout",_s->tcp_server_timeout, MDTU_COM_CONFIG_PATH);
	ret = ini_putl("tcp_server", "tcptmr",_s->tcp_server_tcptmr, MDTU_COM_CONFIG_PATH);
	ret = ini_putl("tcp_server", "schedule",_s->tcp_server_schedule, MDTU_COM_CONFIG_PATH);
	return ret;
}
void set_general_info(void* text){
	int ret;
	/*parse json*/
	extern global_tcp_config_t global_tcp_config_s;
	memset(&global_tcp_config_s,0,sizeof(global_tcp_config_t));
	parse_general_json(&global_tcp_config_s,text);
	/*execute set*/
	ret=exec_set_general_operation(&global_tcp_config_s);
	if(!ret){
		int n = errno;
		printlogf(LOG_ERR, "exec_set_general_operation failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		info_publish("set_general_info ERR!\n");
		goto err01;
	}
	/*publish*/
	info_publish("set_general_info OK!\n");
	err01:
	cJSON_Delete((cJSON*)text);
}
#pragma endregion ok whether need to restart tcp server when set after

#pragma region 2.sn
void parse_sn_json(void *s,char* text){
	set_sn_t* _s=(set_sn_t*)s;
	cJSON *json,*value,*arr;char* out;
	json=(cJSON *)text;

	arr=cJSON_GetObjectItem(json,"val");
	value=cJSON_GetObjectItem(arr,"model");
	out=cJSON_Print(value);
	strcpy(_s->model,out);
	
	value=cJSON_GetObjectItem(arr,"sn");
	out=cJSON_Print(value);
	strcpy(_s->sn,out);

	value=cJSON_GetObjectItem(arr,"key");
	out=cJSON_Print(value);
	strcpy(_s->key,out);

	value=cJSON_GetObjectItem(arr,"hver");
	out=cJSON_Print(value);
	_s->hver=atoi(out);

}

int exec_set_sn_operation(void *s){
	int ret;
	/*at命令实现*/
	set_sn_t* _s=(set_sn_t*)s;
	/*sn*/
	clean_char_arr();
	sprintf(atc_cmd_tmp,"AT+GFSN=<%s>",_s->sn);
	strncpy(atc_cmd_req,atc_cmd_tmp,strlen(atc_cmd_tmp));
	Fibo_Send_AT_Cmd(atc_cmd_req, atc_cmd_resp, ATC_REQ_RESP_CMD_MAX_LEN);
	if(strstr(atc_cmd_resp,"OK")){
		ret=0;
	}else{
		ret=-1;
	}
	return ret;
}
void set_sn_info(void* text){
	int ret;
	/*parse json*/
	set_sn_t sn_s;
	parse_sn_json(&sn_s,text);
	/*execute set*/
	ret=exec_set_sn_operation(&sn_s);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "exec_set_sn_operation failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		info_publish("set_sn_info ERR!\n");
		goto err01;
	}
	/*publish*/
	info_publish("set_sn_info OK!\n");
	err01:
	cJSON_Delete((cJSON*)text);

}
#pragma endregion AT command invalid

#pragma region 3.apn
void parse_apn_json(void *s,char* text){

}
int exec_set_apn_operation(void *s){
	return 0;
}
void set_apn_info(void* text){

}
#pragma endregion AT command invalid

#pragma region 4.ntp
void parse_ntp_json(void *s,char* text){
	global_ntp_config_t* _s=(global_ntp_config_t*)s;
	cJSON *json,*value,*arr;char* out;
	json=(cJSON *)text;

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
	
}
int exec_set_ntp_operation(void *s){
	int ret=0;
	global_ntp_config_t* _s=(global_ntp_config_t*)s;/**/
	ret = ini_puts("ntp_config", "primary_ip", NULL, MDTU_COM_CONFIG_PATH);
	ret = ini_puts("ntp_config", "primary_ip", _s->ntp_primary_ip, MDTU_COM_CONFIG_PATH);
	ret = ini_puts("ntp_config", "second_ip", _s->ntp_second_ip, MDTU_COM_CONFIG_PATH);
	ret = ini_putl("ntp_config", "interval",_s->ntp_interval, MDTU_COM_CONFIG_PATH);
	ret = ini_putl("ntp_config", "timezone",_s->ntp_timezone, MDTU_COM_CONFIG_PATH);
	/*modefy timezone file location /etc/TZ*/
	
	return ret;
	
}
void set_ntp_info(void* text){
	int ret;
	/*parse json*/
	memset(&global_ntp_config_s,0,sizeof(global_ntp_config_t));
	parse_ntp_json(&global_ntp_config_s,text);
	/*execute set*/
	ret=exec_set_ntp_operation(&global_ntp_config_s);
	/*ntp sync time*/
	ntp_operate(global_ntp_config_s.ntp_primary_ip);
	if(!ntpTimeSync){
		ntp_operate(global_ntp_config_s.ntp_second_ip);
	}
	if(!ntpTimeSync){
			printlogf(LOG_NOTICE,"ntp sync failed!\n");
		}else{
			printlogf(LOG_NOTICE,"ntp sync success!\n");
		}
	printlogf(LOG_NOTICE,"ntp sync:%d!\n",ntpTimeSync);
		
	
	if(!ret){
		int n = errno;
		printlogf(LOG_ERR, "exec_set_ntp_operation failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		info_publish("set_ntp_info ERR!\n");
		goto err01;
	}
	/*publish*/
	info_publish("set_ntp_info OK!\n");
	err01:
	cJSON_Delete((cJSON*)text);
}
#pragma endregion ok

#pragma region 5.link
void parse_link_json(void *s,char* text){

}
int exec_set_link_operation(void *s){
	return 0;
}
void set_link_info(void* text){

}
#pragma endregion to do shell experiment

#pragma region 6.bin
void parse_bin_json(void *s,char* text){

}
int exec_set_bin_operation(void *s){
return 0;
}
void set_bin_info(void* text){

}
#pragma endregion to do 

#pragma region 7.update
void parse_update_json(void *s,char* text){
	global_update_config_t* _s=(global_update_config_t*)s;
	cJSON *json,*value,*arr;char* out;
	json=(cJSON *)text;

	arr=cJSON_GetObjectItem(json,"val");
	value=cJSON_GetObjectItem(arr,"url");
	out=cJSON_Print(value);
	strcpy(_s->url,out);
	
	value=cJSON_GetObjectItem(arr,"model");
	out=cJSON_Print(value);
	strcpy(_s->model,out);

	value=cJSON_GetObjectItem(arr,"ver");
	out=cJSON_Print(value);
	_s->ver=atoi(out);
}
int exec_set_update_operation(void *s){
	int ret;
	global_update_config_t* _s=(global_update_config_t*)s;
	ret = fibo_fota_FotaUpgrade(_s->url, "updata image path", "", "");
	return 0;
}
void set_update_info(void* text){
	int ret;
	/*parse json*/
	global_update_config_t global_update_config_s;
	memset(&global_update_config_s,0,sizeof(global_update_config_t));
	parse_update_json(&global_update_config_s,text);
	/*execute set*/
	ret=exec_set_update_operation(&global_update_config_s);
	if(!ret){
		int n = errno;
		printlogf(LOG_ERR, "exec_set_update_operation failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		info_publish("set_update_info ERR!\n");
		goto err01;
	}
	/*publish*/
	info_publish("set_update_info OK!\n");
	err01:
	cJSON_Delete((cJSON*)text);
}
#pragma endregion to do 

#pragma region 8.restart

void set_restart_info(void* text){
	int ret;
	/*execute set*/
	char buffer[SHELL_RECEIVE_MAX_LEN]={0};
	/*reboot*/
	//ret=exec_shell_command("/sbin/reboot",NULL,buffer,SHELL_RECEIVE_MAX_LEN);
	/*ls test*/
	//ret=exec_shell_command("/bin/ls",NULL,buffer,SHELL_RECEIVE_MAX_LEN);

	ret=exec_shell_command("ls",buffer,SHELL_RECEIVE_MAX_LEN);
	printlogf(LOG_NOTICE,"shell return content:\n%s\n",buffer);
	if(ret){
		int n = errno;
		printlogf(LOG_ERR, "set_restart_info failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		info_publish("set_restart_info ERR!\n");
		goto err01;
	}
	/*publish*/
	info_publish("set_restart_info OK!\n");
	err01:
	cJSON_Delete((cJSON*)text);
}

#pragma endregion ok

#pragma region 9.restore

void set_restore_info(void* text){
	int ret;
	/*execute set*/
	if(!ret){
		int n = errno;
		printlogf(LOG_ERR, "exec_set_general_operation failed reason:\n", strerror(n));
		printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
		info_publish("set_restore_info ERR!\n");
		goto err01;
	}
	/*publish*/
	info_publish("set_restore_info OK!\n");
	err01:
	cJSON_Delete((cJSON*)text);

}
#pragma endregion where is the command






