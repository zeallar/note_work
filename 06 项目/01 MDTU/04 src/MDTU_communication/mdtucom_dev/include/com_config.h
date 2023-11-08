#ifndef COM_CONFIG
#define COM_CONFIG
int32_t get_date_str(const int8_t *section,const int8_t *key,int8_t* location);
int32_t load_config();
int32_t open_ini();
int32_t init_ini();


int32_t ini_put_str(const int8_t *section,const int8_t *key,const int8_t* val);
int32_t ini_put_int(const int8_t *section,const int8_t *key,const int32_t val);


#endif

