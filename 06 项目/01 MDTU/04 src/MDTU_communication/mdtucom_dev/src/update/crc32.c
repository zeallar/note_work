/*************************************************
Author:zhouBL
Version:
Description:crc32类方法
Others:
created date:2023/11/7 3:14 下午
modified date:
*************************************************/
#include <stdio.h>
#include <errno.h>
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/types.h>
#include "fibo_type.h"

#define int32_t bf_int32_t
#define uint32_t bf_uint32_t
#include "app_api.h"
#undef int32_t
#undef uint32_t

#include "com_log.h"

#define    POLYCRC32    0XEDB88320L            //CRC32标准
static uint32_t crc_table32[256];		  //CRC查询表


 //生成CRC查询表
 void init_crc32_tab()
 {
	 int32_t i, j;
	 uint32_t crc;
 
	 for(i = 0; i < 256; i++)
	 {
		 crc = (uint32_t )i;
		 for(j = 0; j < 8; j++)
		 {
			 if(crc & 0X00000001L)
			 {
				 crc = (crc >> 1) ^ POLYCRC32;
			 }
			 else
			 {
				 crc = crc >> 1;
			 }
		 }
		 crc_table32[i] = crc;
	 }
 
 }
 //获得CRC
 uint32_t get_crc(uint32_t crcinit, uint8_t *bs, uint32_t bssize)
 {
	 uint32_t crc = crcinit ^ 0XFFFFFFFF;
	 while(bssize--)
	 {
		 crc = (crc>>8)^crc_table32[(crc&0XFF)^*bs++];
	 }
	 return crc ^ 0XFFFFFFFF;
 }
 //获得文件CRC
 
 int32_t calc_img_crc(const int8_t *pFileName, uint32_t *uiCrcValue)
 {
 	int32_t ret=0;
	 if(!pFileName || !uiCrcValue)
	 {
		 LOGC(LOG_ERR,"bad parameter");
		 ret=-1;
	 }
	 if(access(pFileName, F_OK|R_OK)!=0)
	 {
		 LOGC(LOG_ERR,"file not exist or reading file has not permission");
		 ret=-1;
	 }
	 const uint32_t size = 16*1024;
	 uint8_t crcbuf[size];
	 uint32_t rdlen;
	 uint32_t crc = 0;	// CRC初始值为0
 
	 FILE *fd = NULL;
	 if((fd = fopen(pFileName, "r"))==NULL)
	 {
		 LOGC(LOG_ERR,"to do crc 32 check, open file error");
		 ret=-1;
	 }
	 while((rdlen=fread(crcbuf, sizeof(uint8_t), size, fd))>0)
	 {
		 crc = get_crc(crc, crcbuf, rdlen);
	 }
	 *uiCrcValue = crc;
	 fclose(fd);
	 return ret;;
 }
/*
* @Description:读取文件最后4位crc32
* @return-crc32
*/
uint32_t read_crc32(){
	uint8_t crc32_buf[4];
	FILE* fp=fopen(TEMP_IMAGE_SAVE,"r+");
	if (fp == NULL)
	{
		LOGC(LOG_ERR,"file can not open!\n\r");
		return;
	}
	fseek(fp, -4, SEEK_END);
	fread(&crc32_buf, sizeof(crc32_buf), 1, fp);
	//if(DebugOpt)
	LOGC(LOG_NOTICE,"parse_crc_value = %x\n", *((uint32_t*)crc32_buf));
	fclose(fp);
	return *((uint32_t*)crc32_buf);
}
/*恢复文件:抹去文件后4位crc32*/
int32_t recover_file(){
	uint32_t length; 
	uint8_t crc32_buf[4];
	FILE* fp=fopen(TEMP_IMAGE_SAVE,"r+");
	if (fp == NULL)
	{
		LOGC(LOG_ERR,"file can not open!\n\r");
		return -1;
	}
	fseek(fp,-4, SEEK_END);
	length = ftell(fp);
	ftruncate(fileno(fp), length);
	LOGC(LOG_NOTICE,"length = %d\n",length);
	fclose(fp);
	return 0;

}
/*
* @Description:校验crc323
* @return- 0:success 1:failed
*/
int32_t check_file_crc32()
{	
	int32_t ret=0;
	
	uint32_t update_file_crc=read_crc32();
	usleep(10);
	//去除最后4位crc32，恢复文件
	ret=recover_file();
	if(ret){
		LOGC(LOG_ERR,"recover_file failed");
		return;
	}
	usleep(10);
	
	init_crc32_tab();
    uint32_t crc_value = 0;
    if(calc_img_crc(TEMP_IMAGE_SAVE, &crc_value) == 0)
    {
        LOGC(LOG_NOTICE,"crc_value = %x\n", crc_value);
    }
    else
    {
        LOGC(LOG_ERR,"calculate crc 32 value error");
        ret=-1;
    }

	if (crc_value!=update_file_crc){
		ret=-1;
	}
	return ret;
	
}

