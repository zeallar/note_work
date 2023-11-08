/*************************************************
Author:zhouBL
Version:
Description:get and set 工具类方法
Others:
created date:2023/11/7 3:13 下午
modified date:
*************************************************/


#include "fibo_type.h"
#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t
#include "fibo_uart.h"
#include "com_utils.h"
#include "def_structs.h"

#include "thread_usb.h"
#include "com_log.h"
#include "mqtt.h"
#include "tcp_server.h"

/*
* @Description:移除双引号
* @param1- 参数:a 要移除双引号的value字段
* @return-
*/
void remove_double_quotation_marks(int8_t* a){
	int32_t i=1;
	while(a[i]!='\"')
	{
		a[i-1]=a[i];
		i++;
	}
	a[i-1]='\0';
}

/*
* @Description:将地址转换成str
* @param1- addr:
* @param2- buf:
* @param3- len:
* @param4- pIsIpv6:
* @return- ip地址
*/
int8_t *addr2str(fibo_data_call_addr_t *addr, int8_t *buf, int32_t len, int32_t *pIsIpv6)
{
    int8_t *ptr;
    int32_t i;
    int32_t isIpv6 = 0;

    if(addr->valid_addr==0)
    {
        return NULL;
    }

    if(addr->addr.ss_family == AF_INET6)
    {
        isIpv6 = 1;
    }

    if(pIsIpv6)
    {
        pIsIpv6[0] = isIpv6;
    }
    if(isIpv6==0)
    {
        snprintf(buf, len, "%d.%d.%d.%d", addr->addr.__ss_padding[0],
                addr->addr.__ss_padding[1],addr->addr.__ss_padding[2],addr->addr.__ss_padding[3]);
    }
    else {
        inet_ntop(AF_INET6, &(addr->addr.__ss_padding), buf, len);
    }
    return buf;
}
/*
* @Description:清理主线程任务
* @return- none
*/
void clean_thread_info(void* _s){
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t *)_s;
	list_node_t *list_node_s= list_find(task_list, &(s->task_id));
	list_remove(task_list,list_node_s);
}

/*
* @Description:选择交互处理方法。根据client类型不同,选择相应的处理。
* @return- none
*/
void select_handler_method(void * _s){
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t *)_s;
	list_node_t *list_node_s= list_find(task_list, &(s->task_id));
	switch (s->flag)
		{
		case 0:
			info_publish((void*)s);
			break;
		case 1:
			{
			/*tcp server interface*/
			if(!s->data_type){
				send_data(s);
			}else{
				list_remove(task_list,list_node_s);
			}
			break;
		}
			
		case 2:
			{
			if(!s->data_type){
				usb_send_fun(s);
			}else{
				list_remove(task_list,list_node_s);
			}
			break;
		}
			
		}
}
/*
* @Description:发布信息
* @param1- _s:thread_info_transmit_t
* @return- none
*/
void info_publish(void * _s){
	struct thread_info_transmit_t *s=(struct thread_info_transmit_t *)_s;
	mqtt_publish(pub_topic,(void*)s);
	if(DebugOpt){
		LOGC(LOG_NOTICE,"%s",s->json_text);
	}
}
/*
* @Description:打开串口
* @return-串口文件描述符
*/
int32_t open_serial(){
	int32_t fd = -1;
	fibo_uart_dcb_t uart_dcb;
	memset(&uart_dcb, 0x0, sizeof(uart_dcb));
	uart_dcb.flowctrl = E_FC_NONE;
	uart_dcb.baudrate = E_B_9600;
	fd = fibo_uart_open(FIBO_UART1_DEV, &uart_dcb);
	if (fd < 0)
    {
    	info_publish("fibo_uart_open failure!\n");
		
		LOGC(LOG_ERR, "fibo_uart_open failed reason:", strerror(errno));
    }
	return fd;
}


