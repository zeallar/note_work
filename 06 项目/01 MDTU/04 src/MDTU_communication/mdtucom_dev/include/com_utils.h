#ifndef COM_UTILS_H
#define COM_UTILS_H
#include "fibo_oe.h"
void remove_double_quotation_marks(char* a);
char *addr2str(fibo_data_call_addr_t *addr, char *buf, int len, int *pIsIpv6);
#endif /*COM_UTILS_H*/