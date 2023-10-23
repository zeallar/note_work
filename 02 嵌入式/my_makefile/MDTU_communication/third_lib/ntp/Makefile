#子目录的Makefile直接读取其子目录就行
MY_SUBDIRS:=$(filter-out include/,$(foreach f, $(notdir \
  $(filter-out ./ , $(shell find ./ -maxdepth 1 -type d))), $(f)))
SUBDIRS=$(MY_SUBDIRS)
#以下同根目录下的makefile的相同代码的解释
CUR_SOURCE=${wildcard *.c}
CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}

all:$(SUBDIRS) $(CUR_OBJS)
$(SUBDIRS):ECHO
	make -C $@
$(CUR_OBJS):%.o:%.c
	$(CC)  $(CFLAGS) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
ECHO:
	@echo $(SUBDIRS)