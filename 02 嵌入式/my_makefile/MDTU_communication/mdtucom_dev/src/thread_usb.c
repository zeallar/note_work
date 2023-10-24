#include "fibo_oe.h"
#include "com_log.h"
#define DEV_USB  "/dev/ttyGS1"
#define SEND_RECEV_MAX_SIZE 1024



int fd_usb =-1;
pthread_t thread_id;
void* thread_usb_recev_fun()
{
	char rec_data[SEND_RECEV_MAX_SIZE];
    int rec_len = 0;
    int ret;
    fd_set readfds;
    int max_fd = 0;
    prctl(PR_SET_NAME, "thread_usb_recev_fun", 0, 0, 0);
    while(1) {
        FD_ZERO(&readfds);
        FD_SET(fd_usb,&readfds);
        memset(rec_data,0,sizeof(rec_data));
        ret = select(fd_usb + 1, &readfds, NULL, NULL, NULL);
	
        rec_len = read(fd_usb,rec_data,sizeof(rec_data));
        if(rec_len > 0) {
			printlogf(LOG_NOTICE,"usb data receive test: (%d)%s\n", rec_len,rec_data);  
        }
    }
}

void* open_usb_thread(){
	int ret,i=0;int a=1;
	struct epoll_event event;
	char send_data[SEND_RECEV_MAX_SIZE];
	fd_usb = open(DEV_USB,O_RDWR|O_NONBLOCK);
    if(fd_usb >= 0 ) {
	printlogf(LOG_NOTICE,"start usb thread!\n");
    ret = pthread_create(&thread_id, NULL, thread_usb_recev_fun, NULL);
    if (ret) {
		printlogf(LOG_ERR,"Error, create thread failed(ret = %d).\n",ret);
        close(fd_usb);
        return;
    	}
    }else {
    	printlogf(LOG_ERR,"deviceopen %s fail,errno: %d\n",DEV_USB,errno);
        return;
    }
	 while(1)
    {
        memset(send_data,0,sizeof(send_data));        
        snprintf(send_data,sizeof(send_data)-1,"usb data send test: %d\r",i++);
        write(fd_usb,send_data,strlen(send_data));
        sleep(1);
    }
    return;
}

