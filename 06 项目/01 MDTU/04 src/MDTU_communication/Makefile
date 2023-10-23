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


SUBDIRS := third_lib mdtucom_dev mdtucom_application
ROOT_DIR=$(shell pwd)
BIN=mqtt_test
OBJS_DIR=debug/obj
BIN_DIR=debug/bin
CUR_SOURCE=${wildcard *.c}
CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
export BIN OBJS_DIR BIN_DIR ROOT_DIR CFLAGS LDFLAGS
all:$(SUBDIRS) $(CUR_OBJS) DEBUG
$(SUBDIRS):ECHO
	make -C $@
DEBUG:ECHO
	make -C debug
ECHO:
	@echo $(SUBDIRS)
$(CUR_OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@ 
CLEAN:
	@rm $(OBJS_DIR)/*.o
	@rm -rf $(BIN_DIR)/*
