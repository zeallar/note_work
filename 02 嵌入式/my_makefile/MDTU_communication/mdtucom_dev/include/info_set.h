#ifndef INFO_SET_H
#define INFO_SET_H
typedef struct{
	char model[32];
	char sn[32];
	char key[64];
	unsigned int hver;
}set_sn_t;
int exec_shell_command(const char *command,char* mes,int buf_len);

void parse_serial_json(void *s,char* text);
int exec_set_serial_operation(void *s);
void set_serial_info(void* text);

void parse_general_json(void *s,char* text);
int exec_set_general_operation(void *s);
void set_general_info(void* text);

void parse_sn_json(void *s,char* text);
int exec_set_sn_operation(void *s);
void set_sn_info(void* text);

void parse_apn_json(void *s,char* text);
int exec_set_apn_operation(void *s);
void set_apn_info(void* text);

void parse_ntp_json(void *s,char* text);
int exec_set_ntp_operation(void *s);
void set_ntp_info(void* text);

void parse_link_json(void *s,char* text);
int exec_set_link_operation(void *s);
void set_link_info(void* text);

void parse_link_json(void *s,char* text);
int exec_set_link_operation(void *s);
void set_link_info(void* text);

void parse_bin_json(void *s,char* text);
int exec_set_bin_operation(void *s);
void set_bin_info(void* text);

void parse_update_json(void *s,char* text);
int exec_set_update_operation(void *s);
void set_update_info(void* text);

void set_restart_info(void* text);


void set_restore_info(void* text);


#endif/*INFO_SET_H*/

