WCL ?= wcl

BIN_DIR := bin
CFLAGS := -q -0 -bt=dos -ms -Iinclude -s -os
NIO_SRCS := lib/nio.c lib/serial.c

.PHONY: all clean

all: $(BIN_DIR)/nioprobe.exe $(BIN_DIR)/nioread.exe

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/nioprobe.exe: apps/nioprobe.c $(NIO_SRCS) include/nio.h include/serial.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/nioprobe.c $(NIO_SRCS)

$(BIN_DIR)/nioread.exe: apps/nioread.c $(NIO_SRCS) include/nio.h include/serial.h | $(BIN_DIR)
	$(WCL) $(CFLAGS) -fe=$@ apps/nioread.c $(NIO_SRCS)

clean:
	rm -rf $(BIN_DIR)
	rm -f apps/*.obj lib/*.obj *.obj *.o *.err
