/*************************************************
Author:zhouBL
Version:
Description:usb类方法
Others:
created date:2023/11/7 3:36 下午
modified date:
*************************************************/
#include "fibo_oe.h"
#include "com_log.h"
#include "list.h"
#include "def_structs.h"
#include <unistd.h>

#define DEV_USB  "/dev/ttyGS1"
#define SEND_RECEV_MAX_SIZE 1024

extern void doit(void* dpt);
extern uint32_t task_id;


int32_t fd_usb =-1;
pthread_t thread_id;
/*
* @Description:数据接收方法
* @return- none
*/
void* thread_usb_recev_fun()
{
	int8_t rec_data[SEND_RECEV_MAX_SIZE];
    int32_t rec_len = 0;
    int32_t ret;
    fd_set readfds;
    int32_t max_fd = 0;
    prctl(PR_SET_NAME, "thread_usb_recev_fun", 0, 0, 0);
    while(1) {
        FD_ZERO(&readfds);
        FD_SET(fd_usb,&readfds);
        memset(rec_data,0,sizeof(rec_data));
        ret = select(fd_usb + 1, &readfds, NULL, NULL, NULL);
	
        rec_len = read(fd_usb,rec_data,sizeof(rec_data));
        if(rec_len > 0) {
			LOGC(LOG_NOTICE,"usb data receive test: (%d)%s", rec_len,rec_data); 

			struct thread_info_transmit_t *s=MDTU_COM_MALLOC_STRUCT(thread_info_transmit_t);

			s->task_id=task_id++;
			strcpy(s->json_text,rec_data);
			s->flag=2;
			s->fd_usb=fd_usb;
			
			/*
			dup2(fd_usb,0); // STDIN
		 	dup2(fd_usb,1); // STDOUT
		    dup2(fd_usb,2); // STDERR
			*/
			list_node_t *list_node_s = list_node_new(s);
			list_rpush(task_list, list_node_s);
			
			doit(s);
        }
    }
}
/*
* @Description:数据发送方法
* @param1- _s:thread_info_transmit_t
* @return- none
*/
void usb_send_fun(void* _s){
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t *)_s;
	write(fd_usb,s->json_text,strlen(s->json_text));
	list_node_t *list_node_s = list_find(task_list, &(s->task_id));
	list_remove(task_list,list_node_s);
}
/*
* @Description:打开usb线程
* @return- none
*/
void* open_usb_thread(){
	int32_t ret,i=0;int32_t a=1;
	struct epoll_event event;
	fd_usb = open(DEV_USB,O_RDWR|O_NONBLOCK);
    if(fd_usb >= 0 ) {
	LOGC(LOG_NOTICE,"start usb thread!");
    ret = pthread_create(&thread_id, NULL, thread_usb_recev_fun, NULL);
    if (ret) {
		LOGC(LOG_ERR,"Error, create thread failed(ret = %d).",ret);
        close(fd_usb);
        return;
    	}
    }else {
    	LOGC(LOG_ERR,"deviceopen %s fail,errno: %d",DEV_USB,errno);
        return;
    }

    return;
}

