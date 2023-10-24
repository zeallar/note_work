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
OBJS_DIR=build/obj
BIN_DIR=build/bin
CUR_SOURCE=${wildcard *.c}
CUR_OBJS=$(addsuffix .o,$(addprefix $(ROOT_DIR)/$(OBJS_DIR)/,$(basename $(notdir $(CUR_SOURCE)))))
export BIN OBJS_DIR BIN_DIR ROOT_DIR CFLAGS LDFLAGS
all:$(SUBDIRS) $(CUR_OBJS) BUILD
$(SUBDIRS):ECHO
	make -C $@
BUILD:ECHO
	make -C build
ECHO:
	@echo $(SUBDIRS)
$(CUR_OBJS):$(ROOT_DIR)/$(OBJS_DIR)/%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@ 
CLEAN:
	@rm $(OBJS_DIR)/*.o
	@rm -rf $(BIN_DIR)/*
