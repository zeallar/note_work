OBJS=*.o
ODIR=obj
$(ROOT_DIR)/$(BIN_DIR)/$(BIN):$(ODIR)/$(OBJS)
	$(CC) -o $@ $^ -Wl,-start-group $(LDFLAGS) -Wl,-end-group $(CFLAGS)
	$(STRIP) -g $@
	chmod a+x $@