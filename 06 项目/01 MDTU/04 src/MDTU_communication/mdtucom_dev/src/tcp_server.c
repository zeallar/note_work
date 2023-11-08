/*************************************************
Author:zhouBL
Version:
Description:tcp server类方法
Others:
created date:2023/11/7 3:24 下午
modified date:
*************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <malloc.h>
#include <fcntl.h>
#include <assert.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "com_log.h"
#include "def_structs.h"
#include "list.h"

#define BUF_SIZE 1024
#define LISTENQ 10
int32_t con_sock;
int32_t lsn_sock;
int32_t tcp_running=1;
/*tcp server variable*/
extern global_tcp_config_t global_tcp_config_s;
int8_t recv_buffer[BUF_SIZE]={0};
int8_t send_buffer[BUF_SIZE]={0};
pthread_mutex_t send_lock;
pthread_mutex_t recv_lock;

extern uint32_t task_id;

struct socket_clients_t{
	int32_t socket_id;
	struct socket_clients_t* next;
};
static struct socket_clients_t* socket_clients_list=NULL;

typedef struct __pthread_arg
{
    pthread_mutex_t *arg_lock;
    int32_t sock;
    int8_t IP[24];
    int32_t port;
}pthread_arg;
extern void doit(void* dpt);
void free_one_socket(int32_t sock);
/*
* @Description:清理指定的socket
* @param1- sock:
* @return- none
*/
void clean_sock(int32_t sock){
	LOGC(LOG_NOTICE,"thread exit,socket is %d!",sock);
    close(sock);
	free_one_socket(sock);
}
/*
* @Description:数据接收函数
* @param1- sock:
* @return- 0:success,othre:failed
*/
int32_t recv_data(int32_t sock)
{
	memset(recv_buffer,0,sizeof(recv_buffer));
    int32_t recv_len = 0;
    pthread_mutex_lock(&recv_lock);
    recv_len = recv(sock, recv_buffer, BUF_SIZE, 0);
    if (recv_len < 0)
    {
    	LOGC(LOG_WARNING,"receive timeout sock=%d",sock);
        pthread_mutex_unlock(&recv_lock);
        return -1;
    }else if(recv_len==0){
		LOGC(LOG_NOTICE,"client disconnected! ");
		return 1;
	}

	struct thread_info_transmit_t *s=MDTU_COM_MALLOC_STRUCT(thread_info_transmit_t);

	s->task_id=task_id++;
	strcpy(s->json_text,recv_buffer);
	s->flag=1;
	s->sock=sock;
	
	list_node_t *list_node_s = list_node_new(s);
	list_rpush(task_list, list_node_s);

	doit(s);

    pthread_mutex_unlock(&recv_lock);
    return 0;
}
/*
* @Description:数据发送函数
* @param1- _s:thread_info_transmit_t
* @return- 0:success,othre:failed
*/
int32_t send_data(void* _s)
{
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t *)_s;
    int32_t recv_len = 0;
    pthread_mutex_lock(&send_lock);
	memset(send_buffer,0,sizeof(send_buffer));
	strncpy(send_buffer,s->json_text,strlen(s->json_text));
    recv_len = send(s->sock, send_buffer, BUF_SIZE, 0);
	list_node_t *list_node_s = list_find(task_list, &(s->task_id));
	list_remove(task_list,list_node_s);
    if (recv_len < 0)
    {
    	LOGC(LOG_WARNING,"send timeout sock=%d",s->sock);
        pthread_mutex_unlock(&send_lock);
        return -1;
    }else if(recv_len==0){
			LOGC(LOG_NOTICE,"client disconnected! ");
			clean_sock(s->sock);
			return 1;
	}
    pthread_mutex_unlock(&send_lock);
    return 0;
}
/*
* @Description:socket 队列中释放指定的socket
* @param1- sock:
* @return- none
*/
void free_one_socket(int32_t sock){
	
	struct socket_clients_t *sa_ptr, **prevNext = &socket_clients_list;
	
	for (sa_ptr = socket_clients_list;
		 sa_ptr != NULL && sock==sa_ptr->socket_id;
		 sa_ptr = sa_ptr->next) {
		prevNext = &(sa_ptr->next);
		}
	if (sa_ptr != NULL) {
		*prevNext = sa_ptr->next;
		free(sa_ptr);
	}
}
/*
* @Description:tcp server 多线程socket连接处理函数
* @param1- arg:pthread_arg
* @return- none
*/
void *handle_tcp_connection(void *arg)
{
    assert(arg);
	int32_t ret=-1;
    pthread_arg connet_thread_arg = *(pthread_arg *)arg;
    int32_t sock = connet_thread_arg.sock;
    pthread_mutex_init(&send_lock,NULL);
    pthread_mutex_init(&recv_lock,NULL);

	struct socket_clients_t **s;
	for (s=&socket_clients_list;(*s);s=&((*s)->next));
	(*s)=MDTU_COM_MALLOC_STRUCT(socket_clients_t);
	assert(NULL != (*s));
	(*s)->socket_id=sock;
	(*s)->next=NULL;
 
	while(tcp_running){
		ret = recv_data(sock);
	    if(ret)
	    {	
	        goto err0;
	    }
	}
err0:
	pthread_mutex_destroy(&send_lock);
	pthread_mutex_destroy(&recv_lock);
	clean_sock(sock);
}


/*
* @Description:关闭tcp server
* @return- 0:success,othre:failed
*/
int32_t close_tcp_server(){
	struct socket_clients_t *socket_ptr, *socket_tmp;
	for (socket_ptr = socket_clients_list; socket_ptr != NULL; socket_ptr = socket_tmp) {
		socket_tmp = socket_ptr->next;
		free(socket_ptr);
	}
	close(lsn_sock);
	tcp_running=0;
	return 0;
}
/*
* @Description:打开tcp server
* @return- none
*/
void* open_tcp_server(){
	lsn_sock = -1;
    con_sock = -1;
    struct sockaddr_in client_addr, server_addr;
    int32_t addrlen = sizeof(server_addr);
	struct timeval timeout = {60, 0};
    int32_t ret = -1;

    pthread_t ntid;
    pthread_arg connt_thread_arg;
    int8_t *ip = NULL;
	

    lsn_sock = socket(AF_INET,SOCK_STREAM,0);
    if(lsn_sock < 0)
    {
    	LOGC(LOG_ERR,"creating socket fialed!");
        goto err0;
    }
    memset(&server_addr,0,addrlen);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(global_tcp_config_s.tcp_server_port);
	LOGC(LOG_NOTICE,"binding");
	int32_t mw_optval = 1;
 	setsockopt(lsn_sock, SOL_SOCKET, SO_REUSEADDR, (int8_t *)&mw_optval,sizeof(mw_optval));
    if(bind(lsn_sock, (struct sockaddr *)&server_addr, addrlen) < 0)
    {
    	LOGC(LOG_ERR,"binding socked fialed.");
        goto err0;
    }
	LOGC(LOG_NOTICE,"listening");
    if(listen(lsn_sock, LISTENQ) < 0)
    {
    	LOGC(LOG_ERR,"listening socked failed.");
        goto err0;
    }
    while(1)
    {
        memset(&client_addr,0,addrlen);
		LOGC(LOG_NOTICE,"tcp server waiting to link of clients");
	
        con_sock = accept(lsn_sock, (struct sockaddr *)&client_addr, &addrlen);
        if(con_sock < 0)
        {
        	LOGC(LOG_NOTICE,"tcp server accept has been interrupted!");
            goto err0;
        }
		
		
		timeout.tv_sec = global_tcp_config_s.tcp_server_timeout;
        if(setsockopt(con_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) == -1)
        {	
        	LOGC(LOG_ERR,"set setsockopt send time out error!");
    		break;
        }
		 if(setsockopt(con_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) == -1)
        {
        	LOGC(LOG_ERR,"set setsockopt recv time out error!");
            break;
        }
        connt_thread_arg.sock = con_sock;
        ip = inet_ntoa(client_addr.sin_addr);
        memcpy(connt_thread_arg.IP, ip, strlen(ip));
        connt_thread_arg.IP[strlen(ip)] = '\0';
        connt_thread_arg.port = (int32_t)client_addr.sin_port;
		//if(DebugOpt)
			LOGC(LOG_NOTICE,"\n the linked coming from %s:%d.", connt_thread_arg.IP, connt_thread_arg.port);
        if(pthread_create(&ntid, NULL, handle_tcp_connection, (void *)(&connt_thread_arg)) < 0)
        {
        	LOGC(LOG_ERR,"creating thread failed!");
            break;
        }
    }
err0:
	LOGC(LOG_NOTICE,"tcp_server exit!");
	close(con_sock);
	close(lsn_sock);
    return;
}

