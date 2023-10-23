/*************************************************
Author:zhouBL
Version:
Description:
Others:
created date:2023/10/16 10:26 上午
modified date:
*************************************************/
#ifndef APP_API_H
#define APP_API_H

// _global type

#undef int8_t
typedef signed char     int8_t;

#ifdef int16_t
#undef int16_t
typedef signed short    int16_t;
#endif

#ifdef int32_t
#undef int32_t
typedef signed long     int32_t;
#endif

#undef uint8_t
typedef unsigned char   uint8_t;

#undef uint16_t
typedef unsigned short  uint16_t;


#ifdef uint32_t
#undef uint32_t
typedef unsigned long   uint32_t;
#endif


/* 固件版本号 */
#define FIRMWARE_VERSION (1)
/*mqtt global variable*/
extern char pub_topic[64];/*publish topic*/
extern char sub_topic[64];/*subscribe topic*/
/*log*/
extern uint16_t DebugOpt;
extern uint16_t LogLevel;
extern int8_t *LevelAry[];
/*config file path*/
#define MDTU_COM_CONFIG_PATH "/etc/mdtu_com.ini"
/*enclosure malloc method*/
#define MDTU_COM_MALLOC_STRUCT(s) (struct s*) calloc(1,sizeof(struct s)) 
/*def macro*/
#define FILE_NAME(x) (strrchr(x,'/')?strrchr(x,'/')+1:x)


#endif

