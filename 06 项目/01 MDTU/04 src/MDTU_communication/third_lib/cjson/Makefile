
MY_SUBDIRS:=$(filter-out include,$(foreach f, $(notdir \
  $(filter-out ./ , $(shell find ./ -maxdepth 1 -type d))), $(f)))
SUBDIRS=$(MY_SUBDIRS)

CUR_SOURCE=${wildcard *.c}
CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}

all:$(SUBDIRS) $(CUR_OBJS)
$(SUBDIRS):ECHO
	make -C $@
$(CUR_OBJS):%.o:%.c
	$(CC)  $(CFLAGS) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR)/$@
ECHO:
	@echo $(SUBDIRS)