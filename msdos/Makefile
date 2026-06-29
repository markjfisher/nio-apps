WCL ?= wcl

BIN_DIR := bin
FUJINET_NIO_LIB ?= ../../fujinet-nio-lib
FN_INC := -I$(FUJINET_NIO_LIB)/include
FN_SERIAL_LIB := $(FUJINET_NIO_LIB)/build/fujinet-nio-msdos-serial.lib
FN_IOCTL_LIB := $(FUJINET_NIO_LIB)/build/fujinet-nio-msdos-ioctl.lib
FNSVC_LIST_MAX_PAYLOAD ?= 420
CFLAGS := -q -0 -bt=dos -ms -Iinclude $(FN_INC) -s -os -DFNSVC_LIST_MAX_PAYLOAD=$(FNSVC_LIST_MAX_PAYLOAD)
NIO_SRCS := lib/nio.c
FNCTL_SRCS := lib/fnctl.c lib/fnsvc.c
FNTOOLS := fhost fls fin fmount fdrive

.PHONY: all clean

all: $(BIN_DIR)/nioprobe.exe $(BIN_DIR)/nioread.exe $(FNTOOLS:%=$(BIN_DIR)/%.exe)

$(FN_SERIAL_LIB):
	$(MAKE) -C $(FUJINET_NIO_LIB) msdos-serial

$(FN_IOCTL_LIB):
	$(MAKE) -C $(FUJINET_NIO_LIB) msdos-ioctl

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/nioprobe.exe: apps/nioprobe.c $(NIO_SRCS) include/nio.h $(FN_SERIAL_LIB) | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/nioprobe.c $(NIO_SRCS) $(FN_SERIAL_LIB)

$(BIN_DIR)/nioread.exe: apps/nioread.c $(NIO_SRCS) include/nio.h $(FN_SERIAL_LIB) | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/nioread.c $(NIO_SRCS) $(FN_SERIAL_LIB)

$(BIN_DIR)/fhost.exe: apps/fhost.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h $(FN_IOCTL_LIB) | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fhost.c $(FNCTL_SRCS) $(FN_IOCTL_LIB)

$(BIN_DIR)/fls.exe: apps/fls.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h $(FN_IOCTL_LIB) | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fls.c $(FNCTL_SRCS) $(FN_IOCTL_LIB)

$(BIN_DIR)/fin.exe: apps/fin.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h $(FN_IOCTL_LIB) | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fin.c $(FNCTL_SRCS) $(FN_IOCTL_LIB)

$(BIN_DIR)/fmount.exe: apps/fmount.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h $(FN_IOCTL_LIB) | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fmount.c $(FNCTL_SRCS) $(FN_IOCTL_LIB)

$(BIN_DIR)/fdrive.exe: apps/fdrive.c $(FNCTL_SRCS) include/fnctl.h include/fnsvc.h $(FN_IOCTL_LIB) | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/fdrive.c $(FNCTL_SRCS) $(FN_IOCTL_LIB)

clean:
	rm -rf $(BIN_DIR)
	rm -f apps/*.obj lib/*.obj *.obj *.o *.err
