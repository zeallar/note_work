
EXTRA_CFLAGS := -D DEBUG
CFLAGS_sub3.o := -D DEBUG_SUB3

SRCS_FILES=$(wildcard *.c)
OBJS_FILES=$(SRCS_FILES:.c=.o)
obj-y += $(OBJS_FILES)

MY_SUBDIRS:=$(filter-out include/,$(foreach f, $(notdir \
  $(filter-out ./ , $(shell find ./ -maxdepth 1 -type d))), $(f)/))
obj-y+=$(MY_SUBDIRS)