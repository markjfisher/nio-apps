SHELL := /usr/bin/env bash
.DEFAULT_GOAL := all

TARGET ?= msdos
FUJINET_NIO_LIB ?= ../fujinet-nio-lib
FNSVC_LIST_MAX_PAYLOAD ?= 420

include makefiles/targets.mk

APP_DIR := apps/common
SRC_DIR := src
APP_INCLUDE_DIR := include/common
PLATFORM_INCLUDE_DIR := include/platform/$(PLATFORM)
NIO_INCLUDE_DIR := $(FUJINET_NIO_LIB)/include
BUILD_DIR ?= build
TARGET_BUILD_DIR := $(BUILD_DIR)/$(TARGET)
OBJ_DIR := $(TARGET_BUILD_DIR)/obj
BIN_DIR := $(TARGET_BUILD_DIR)/bin
DISK_DIR := $(TARGET_BUILD_DIR)/disk

PROGRAMS := fhost fls fin fout fmount fdrive fapp fhttpbin astest clock
PROGRAMS_atari := fsioraw
PROGRAMS += $(PROGRAMS_$(TARGET))
COMMON_SRCS := $(SRC_DIR)/common/fnsvc.c $(SRC_DIR)/platform/$(PLATFORM)/fnctl.c
COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
APP_OBJS := $(PROGRAMS:%=$(OBJ_DIR)/$(APP_DIR)/%.o)
PROGRAM_BINS := $(PROGRAMS:%=$(BIN_DIR)/%$(PROGRAM_EXT))
DEPENDS := $(COMMON_OBJS:.o=.d) $(APP_OBJS:.o=.d)

ifeq ($(COMPILER_FAMILY),wcc)
include makefiles/compiler-wcc.mk
else ifeq ($(COMPILER_FAMILY),cc65)
include makefiles/compiler-cc65.mk
else
$(error Unknown compiler family '$(COMPILER_FAMILY)' for TARGET=$(TARGET))
endif

DISK_TARGETS :=
-include makefiles/disk-$(TARGET).mk

.PHONY: all clean disk $(DISK_TARGETS)
.SECONDARY: $(APP_OBJS) $(COMMON_OBJS)

all: $(PROGRAM_BINS)

disk: $(DISK_TARGETS)

-include $(DEPENDS)

$(NIO_LIB_FILE):
	$(MAKE) -C $(FUJINET_NIO_LIB) $(NIO_LIB_TARGET)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(call compile_c)

$(BIN_DIR)/%$(PROGRAM_EXT): $(OBJ_DIR)/$(APP_DIR)/%.o $(COMMON_OBJS) $(NIO_LIB_FILE) | $(BIN_DIR)
	$(call link_program)

$(BIN_DIR)/fhttpbin$(PROGRAM_EXT): $(OBJ_DIR)/$(APP_DIR)/fhttpbin.o $(NIO_LIB_FILE) | $(BIN_DIR)
	$(call link_program)

$(BIN_DIR)/astest$(PROGRAM_EXT): $(OBJ_DIR)/$(APP_DIR)/astest.o $(NIO_LIB_FILE) | $(BIN_DIR)
	$(call link_program)

$(BIN_DIR)/clock$(PROGRAM_EXT): $(OBJ_DIR)/$(APP_DIR)/clock.o $(NIO_LIB_FILE) | $(BIN_DIR)
	$(call link_program)

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(DISK_DIR):
	mkdir -p $@

clean:
	rm -rf $(TARGET_BUILD_DIR)
