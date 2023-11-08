#ifndef TCP_SERVER_H
#define TCP_SERVER_H
void* open_tcp_server();
int32_t close_tcp_server();
int32_t send_data(void* s);
#endif