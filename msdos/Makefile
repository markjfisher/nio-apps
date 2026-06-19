WCL ?= wcl

BIN_DIR := bin
CFLAGS := -q -0 -bt=dos -ms -Iinclude -s -os
NIO_SRCS := lib/nio.c lib/serial.c
FNCTL_SRCS := lib/fnctl.c lib/fnsvc.c
FNTOOLS := fhost fls fin fmount fdrive

.PHONY: all clean

all: $(BIN_DIR)/nioprobe.exe $(BIN_DIR)/nioread.exe $(FNTOOLS:%=$(BIN_DIR)/%.exe)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/nioprobe.exe: apps/nioprobe.c $(NIO_SRCS) include/nio.h include/serial.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/nioprobe.c $(NIO_SRCS)

$(BIN_DIR)/nioread.exe: apps/nioread.c $(NIO_SRCS) include/nio.h include/serial.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/nioread.c $(NIO_SRCS)

$(BIN_DIR)/fhost.exe: apps/fhost.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fhost.c $(FNCTL_SRCS)

$(BIN_DIR)/fls.exe: apps/fls.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fls.c $(FNCTL_SRCS)

$(BIN_DIR)/fin.exe: apps/fin.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fin.c $(FNCTL_SRCS)

$(BIN_DIR)/fmount.exe: apps/fmount.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fmount.c $(FNCTL_SRCS)

$(BIN_DIR)/fdrive.exe: apps/fdrive.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fdrive.c $(FNCTL_SRCS)

clean:
	rm -rf $(BIN_DIR)
	rm -f apps/*.obj lib/*.obj *.obj *.o *.err
