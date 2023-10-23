#include "com_utils.h"
#include "app_api.h"

/*
* @Description:移除双引号
* @param1- 参数:a 要移除双引号的value字段
* @return-
*/
void remove_double_quotation_marks(char* a){
	int i=1;
	while(a[i]!='\"')
	{
		a[i-1]=a[i];
		i++;
	}
	a[i-1]='\0';
}

/*
* @Description:将地址转换成str
* @param1- 参数:addr 
* @param2- 参数:buf
* @param3- 参数:len
* @param4- 参数:pIsIpv6
* @return-
*/
char *addr2str(fibo_data_call_addr_t *addr, char *buf, int len, int *pIsIpv6)
{
    char *ptr;
    int i;
    int isIpv6 = 0;

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

