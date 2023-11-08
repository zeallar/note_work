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

typedef signed char     int8_t;

typedef signed short    int16_t;

typedef signed long     int32_t;

typedef unsigned char   uint8_t;

typedef unsigned short  uint16_t;

typedef unsigned long   uint32_t;



/* 固件版本号 */
#define FIRMWARE_VERSION 1
/*global variabel*/
extern int8_t **wargv;
/*mqtt global variable*/
extern int8_t pub_topic[64];/*publish topic*/
extern int8_t sub_topic[64];/*subscribe topic*/

/*log*/
extern uint16_t DebugOpt;
extern uint16_t LogLevel;
extern int8_t *LevelAry[];
/*config file path*/
#define MDTU_COM_CONFIG_PATH "/etc/mdtu_com.ini"
#define TIMEZONE_PATH "/etc/TZ"
/*enclosure malloc method*/
#define MDTU_COM_MALLOC_STRUCT(s) (struct s*) calloc(1,sizeof(struct s)) 
#define MDTU_COM_MALLOC_DEF(s) (s*) calloc(1,sizeof(s)) 
/*serial info*/
#define FIBO_UART1_DEV      "/dev/ttyS1"
/*update temp save path*/
#define TEMP_IMAGE_PATH "/update_temp"
#define TEMP_IMAGE_SAVE "/update_temp/ota_temp"
/*current app path*/
#define CUR_APP_PATH "/bin/mqtt_test"




#endif

