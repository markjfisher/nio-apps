CC := cl65

CFLAGS += -Osir -O
CFLAGS += --include-dir $(APP_INCLUDE_DIR)
CFLAGS += --include-dir $(PLATFORM_INCLUDE_DIR)
CFLAGS += --include-dir $(NIO_INCLUDE_DIR)
CFLAGS += -DFNSVC_LIST_MAX_PAYLOAD=$(FNSVC_LIST_MAX_PAYLOAD)

LDFLAGS += -t $(TARGET)

define compile_c
	$(CC) -t $(TARGET) -c --create-dep $(@:.o=.d) --listing $(@:.o=.lst) $(CFLAGS) -o $@ $<
endef

define link_program
	$(CC) $(LDFLAGS) -o $@ $^
endef
