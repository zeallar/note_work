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
#include "com_log.h"
#include "def_structs.h"
#include "app_api.h"
#define BUF_SIZE 1024
#define LISTENQ 10
int con_sock;
int lsn_sock;
int tcp_running=1;
/*tcp server variable*/
extern global_tcp_config_t global_tcp_config_s;

struct socket_clients_t{
	int socket_id;
	struct socket_clients_t* next;
};
static struct socket_clients_t* socket_clients_list=NULL;

typedef struct __pthread_arg
{
    pthread_mutex_t *arg_lock;
    int sock;
    char IP[24];
    int port;
}pthread_arg;

int recv_data(int sock, char *buf, int buf_len, pthread_mutex_t *sock_lock)
{
    int recv_len = 0;
    pthread_mutex_lock(sock_lock);
        recv_len = recv(sock, buf, buf_len, 0);
        if (recv_len < 0)
        {
        	printlogf(LOG_WARNING,"receive timeout sock=%d\n",sock);
            pthread_mutex_unlock(sock_lock);
            return -1;
        }else if(recv_len==0){
			printlogf(LOG_NOTICE,"client disconnected! \n");
			return 1;
		}
		
    pthread_mutex_unlock(sock_lock);
    return 0;
}

int send_data(int sock, char *buf, int buf_len, pthread_mutex_t *sock_lock)
{
    int recv_len = 0;
    pthread_mutex_lock(sock_lock);

    recv_len = send(sock, buf, buf_len, 0);
    if (recv_len < 0)
    {
    	printlogf(LOG_WARNING,"send timeout sock=%d\n",sock);
        pthread_mutex_unlock(sock_lock);
        return -1;
    }else if(recv_len==0){
			printlogf(LOG_NOTICE,"client disconnected! \n");
			return 1;
	}
    pthread_mutex_unlock(sock_lock);
    return 0;
}
void free_one_socket(int sock){
	
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

void *handle_tcp_connection(void *arg)
{
    assert(arg);
	int ret=-1;
    pthread_arg connet_thread_arg = *(pthread_arg *)arg;
    int sock = connet_thread_arg.sock;

	char recv_buffer[BUF_SIZE]={0};
	char send_buffer[BUF_SIZE]={0};
    pthread_mutex_t send_lock;
    pthread_mutex_t recv_lock;
    pthread_mutex_init(&send_lock,NULL);
    pthread_mutex_init(&recv_lock,NULL);
 
	while(tcp_running){
		ret = recv_data(sock, recv_buffer, sizeof(recv_buffer), &recv_lock);
	    if(ret)
	    {	
	        goto err0;
	    }
		printlogf(LOG_NOTICE,"received data is =%s\n",recv_buffer);
		strcpy(send_buffer,"0k!");
	    ret = send_data(sock, send_buffer, strlen(send_buffer),&send_lock);
	    if(ret)
	    {	
	    	goto err0;
	    }
	}
err0:
	printlogf(LOG_NOTICE,"thread exit,socket is %d!\n",sock);
    pthread_mutex_destroy(&send_lock);
    pthread_mutex_destroy(&recv_lock);
    close(sock);
	
	free_one_socket(sock);
}



int close_tcp_server(){
	struct socket_clients_t *socket_ptr, *socket_tmp;
	for (socket_ptr = socket_clients_list; socket_ptr != NULL; socket_ptr = socket_tmp) {
		socket_tmp = socket_ptr->next;
		free(socket_ptr);
	}
	close(lsn_sock);
	tcp_running=0;
	return 0;
}

void* open_tcp_server(){
	lsn_sock = -1;
    con_sock = -1;
    struct sockaddr_in client_addr, server_addr;
    int addrlen = sizeof(server_addr);
	struct timeval timeout = {60, 0};
    int ret = -1;

    pthread_t ntid;
    pthread_arg connt_thread_arg;
    char *ip = NULL;
	

    lsn_sock = socket(AF_INET,SOCK_STREAM,0);
    if(lsn_sock < 0)
    {
    	printlogf(LOG_ERR,"creating socket fialed!\n");
        goto err0;
    }
    memset(&server_addr,0,addrlen);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(global_tcp_config_s.tcp_server_port);
	printlogf(LOG_NOTICE,"binding\n");
	int mw_optval = 1;
 	setsockopt(lsn_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&mw_optval,sizeof(mw_optval));
    if(bind(lsn_sock, (struct sockaddr *)&server_addr, addrlen) < 0)
    {
    	printlogf(LOG_ERR,"binding socked fialed.\n");
        goto err0;
    }
	printlogf(LOG_NOTICE,"listening\n");
    if(listen(lsn_sock, LISTENQ) < 0)
    {
    	printlogf(LOG_ERR,"listening socked failed.\n");
        goto err0;
    }
    while(1)
    {
        memset(&client_addr,0,addrlen);
		printlogf(LOG_NOTICE,"tcp server waiting to link of clients\n");
	
        con_sock = accept(lsn_sock, (struct sockaddr *)&client_addr, &addrlen);
        if(con_sock < 0)
        {
        	printlogf(LOG_NOTICE,"tcp server accept has been interrupted!\n");
            goto err0;
        }
		
		struct socket_clients_t **s;
		for (s=&socket_clients_list;(*s);s=&((*s)->next));
		(*s)=MDTU_COM_MALLOC_STRUCT(socket_clients_t);
		if (*s == NULL) {
			printlogf(LOG_ERR,"don't have enough memory!\n");
			printlogf(LOG_ERR, "%s %s(%d)\n",FILE_NAME(__FILE__),__FUNCTION__,__LINE__);
			goto err0;
		}
		(*s)->socket_id=con_sock;
		(*s)->next=NULL;
		
		timeout.tv_sec = global_tcp_config_s.tcp_server_timeout;
        if(setsockopt(con_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)) == -1)
        {	
        	printlogf(LOG_ERR,"set setsockopt send time out error!\n");
    		break;
        }
		 if(setsockopt(con_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) == -1)
        {
        	printlogf(LOG_ERR,"set setsockopt recv time out error!\n");
            break;
        }
        connt_thread_arg.sock = con_sock;
        ip = inet_ntoa(client_addr.sin_addr);
        memcpy(connt_thread_arg.IP, ip, strlen(ip));
        connt_thread_arg.IP[strlen(ip)] = '\0';
        connt_thread_arg.port = (int)client_addr.sin_port;
		if(DebugOpt)
			printlogf(LOG_NOTICE,"\n the linked coming from %s:%d.\n", connt_thread_arg.IP, connt_thread_arg.port);
        if(pthread_create(&ntid, NULL, handle_tcp_connection, (void *)(&connt_thread_arg)) < 0)
        {
        	printlogf(LOG_ERR,"creating thread failed!\n");
            break;
        }
    }
err0:
	printlogf(LOG_NOTICE,"tcp_server exit!\n");
	close(con_sock);
	close(lsn_sock);
    return;
}

