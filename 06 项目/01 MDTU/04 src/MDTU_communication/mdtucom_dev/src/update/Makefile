
MY_SUBDIRS:=$(filter-out include,$(foreach f, $(notdir \
  $(filter-out ./ , $(shell find ./ -maxdepth 1 -type d))), $(f)))
SUBDIRS=$(MY_SUBDIRS)

CUR_SOURCE=${wildcard *.c}
CUR_OBJS=$(addsuffix .o,$(addprefix $(ROOT_DIR)/$(OBJS_DIR)/,$(basename $(notdir $(CUR_SOURCE)))))

all:$(SUBDIRS) $(CUR_OBJS)
$(SUBDIRS):ECHO
	make -C $@

$(CUR_OBJS):$(ROOT_DIR)/$(OBJS_DIR)/%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@ 
ECHO:
	@echo $(SUBDIRS)