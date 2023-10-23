#/*************************************************
#Author:zhouBL
#Version:
#Description:
#Others:
#created date:10/13/2023 6:42 下午
#modified date:
#*************************************************/

CCTOOLBASE:=${CROSSTOOLS_DIR}
CCLIBS:=${CCTOOLBASE}/usr/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/lib
CCINCS:=${CCTOOLBASE}/usr/arm-buildroot-linux-uclibcgnueabi/sysroot/usr/include

MDTUBASE:=$(shell dirname $(EXTEND_PATH))
MDTUBASE:=${MDTUBASE}/app_demo/MDTU_communication


INC_PATHS := \
	${CCINCS} \
	${FIBO_AUTO_GEN} \
	${CCINCS}/qmi \
	${CCINCS}/qmi-framework \
	${CCINCS}/loc-api-v02 \
	${CCINCS}/dsutils \
	${CCINCS}/data \
	${INCLUDE_DIR} \
	${MDTUBASE}/mdtucom_dev/include \
	${MDTUBASE}/third_lib/cjson \
	${MDTUBASE}/third_lib/ini \
	${MDTUBASE}/third_lib/ntp \
	${MDTUBASE}/third_lib/semaphore \
	${MDTUBASE}/third_lib/thpool \
	${MDTUBASE}/mdtucom_application \

LIB_PATHS := \
        . \
        ${CCLIBS} \
        $(APP_LIB) 

SO_TYPE := share_lib

INC_FLAGS := $(foreach path,$(INC_PATHS),-I$(path) )
LIB_FLAGS := $(foreach path,$(LIB_PATHS),-L$(path) )

USR_WERROR = #-Werror

CFLAGS += -g $(INC_FLAGS) $(USR_WERROR)
CFLAGS += -DLINUX -D_FEATURE_SUPPORT_SYSLOG_
LDFLAGS= $(LIB_FLAGS)
LDFLAGS += -lumdpperipheral -lumdp -lumdpcommon -lsqlite -lpthread  -lrt -lpaho-embed-mqtt3c -lpaho-embed-mqtt3cc -lm 

#要编译的子目录
SUBDIRS := third_lib mdtucom_dev mdtucom_application
#获取项目根路径
ROOT_DIR=$(shell pwd)
#最终bin文件的名字，可以更改为自己需要的
BIN=mqtt_test
#目标文件所在的目录
OBJS_DIR=debug/obj
#bin文件所在的目录
BIN_DIR=debug/bin
#获取当前目录下的c文件集，放在变量CUR_SOURCE中
CUR_SOURCE=${wildcard *.c}
#将对应的c文件名转为o文件后放在下面的CUR_OBJS变量中
CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
#将以下变量导出到子shell中，本次相当于导出到子目录下的makefile中
export BIN OBJS_DIR BIN_DIR ROOT_DIR CFLAGS LDFLAGS
#注意这里的顺序，需要先执行SUBDIRS最后才能是DEBUG
all:$(SUBDIRS) $(CUR_OBJS) DEBUG
#递归执行子目录下的makefile文件，这是递归执行的关键
$(SUBDIRS):ECHO
	make -C $@
DEBUG:ECHO
#直接去debug目录下执行makefile文件
	make -C debug
ECHO:
	@echo $(SUBDIRS)
#将c文件编译为o文件，并放在指定放置目标文件的目录中即OBJS_DIR
$(CUR_OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@ 
CLEAN:
	@rm $(OBJS_DIR)/*.o
	@rm -rf $(BIN_DIR)/*
