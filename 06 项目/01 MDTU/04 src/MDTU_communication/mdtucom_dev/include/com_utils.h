#ifndef COM_UTILS_H
#define COM_UTILS_H
#include "fibo_oe.h"
void remove_double_quotation_marks(int8_t* a);
int8_t *addr2str(fibo_data_call_addr_t *addr, int8_t *buf, int32_t len, int32_t *pIsIpv6);
/*common methods*/
int32_t open_serial();
void info_publish(void * _s);
void select_handler_method(void * _s);
void clean_thread_info(void* s);

#endif /*COM_UTILS_H*/