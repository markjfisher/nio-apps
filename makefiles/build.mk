SHELL := /usr/bin/env bash
.DEFAULT_GOAL := all

TARGET ?= msdos
FUJINET_NIO_LIB ?= ../fujinet-nio-lib

include makefiles/targets.mk

FNSVC_LIST_MAX_PAYLOAD ?= 420

APP_DIR := apps/test
SRC_DIR := src
APP_INCLUDE_DIR := include/common
CONFIG_NIO_INCLUDE_DIR := include/common
PLATFORM_INCLUDE_DIR := include/platform/$(PLATFORM)
NIO_INCLUDE_DIR := $(FUJINET_NIO_LIB)/include
BUILD_DIR ?= build
TARGET_BUILD_DIR := $(BUILD_DIR)/$(TARGET)
OBJ_DIR := $(TARGET_BUILD_DIR)/obj
BIN_DIR := $(TARGET_BUILD_DIR)/bin
DISK_DIR := $(TARGET_BUILD_DIR)/disk

APP_SRCS := $(sort $(wildcard $(APP_DIR)/*.c))
PROGRAMS_ALL := $(basename $(notdir $(APP_SRCS)))
PROGRAMS_EXCLUDE_msdos := fsioraw
PROGRAMS_EXCLUDE_atari :=
PROGRAMS_EXCLUDE_bbc := fsioraw
PROGRAMS_EXCLUDE_bbc-clib := fsioraw
PROGRAMS_EXCLUDE_linux :=
PROGRAMS_EXCLUDE := $(PROGRAMS_EXCLUDE_$(TARGET))
PROGRAMS := $(filter-out $(PROGRAMS_EXCLUDE),$(PROGRAMS_ALL))
MSDOS_APP_SRCS := $(if $(filter msdos,$(TARGET)),$(sort $(wildcard msdos/apps/*.c)))
MSDOS_PROGRAMS := $(basename $(notdir $(MSDOS_APP_SRCS)))

STANDALONE_PROGRAMS := astest clock fhttpbin fmount_inhibit_exp_a fmount_inhibit_exp_b inhibitpoc irqmon
NO_NIO_LIB_PROGRAMS := irqmon
COMMON_SRCS := $(SRC_DIR)/common/fnsvc.c $(SRC_DIR)/platform/$(PLATFORM)/fnctl.c
COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
APP_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(APP_SRCS))
MSDOS_LIB_OBJS := $(if $(filter msdos,$(TARGET)),$(OBJ_DIR)/msdos/lib/nio.o)
MSDOS_APP_OBJS := $(MSDOS_PROGRAMS:%=$(OBJ_DIR)/msdos/apps/%.o)
PROGRAM_BINS := $(PROGRAMS:%=$(BIN_DIR)/%$(PROGRAM_EXT)) $(MSDOS_PROGRAMS:%=$(BIN_DIR)/%$(PROGRAM_EXT))
DEPENDS := $(COMMON_OBJS:.o=.d) $(APP_OBJS:.o=.d) $(MSDOS_LIB_OBJS:.o=.d) $(MSDOS_APP_OBJS:.o=.d)

ifeq ($(COMPILER_FAMILY),wcc)
include makefiles/compiler-wcc.mk
else ifeq ($(COMPILER_FAMILY),cc65)
include makefiles/compiler-cc65.mk
else ifeq ($(COMPILER_FAMILY),gcc)
include makefiles/compiler-gcc.mk
else ifeq ($(COMPILER_FAMILY),amigagcc)
include makefiles/compiler-amigagcc.mk
else
$(error Unknown compiler family '$(COMPILER_FAMILY)' for TARGET=$(TARGET))
endif

DISK_TARGETS :=
-include makefiles/disk-$(TARGET).mk

.PHONY: all clean disk $(PROGRAMS) $(MSDOS_PROGRAMS) $(DISK_TARGETS)
.SECONDARY: $(APP_OBJS) $(COMMON_OBJS)

all: $(PROGRAM_BINS)

disk: $(DISK_TARGETS)

$(PROGRAMS): %: $(BIN_DIR)/%$(PROGRAM_EXT)
$(MSDOS_PROGRAMS): %: $(BIN_DIR)/%$(PROGRAM_EXT)

-include $(DEPENDS)

$(NIO_LIB_FILE):
	$(MAKE) -C $(FUJINET_NIO_LIB) $(NIO_LIB_TARGET)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(call compile_c)

$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	ca65 -t $(TARGET) $(ASMFLAGS) -I /home/markf/dev/nio/fujinet-nio-workspace/repos/cc65/libsrc/bbc -o $@ $<

define APP_PROGRAM_RULE
$(BIN_DIR)/$(1)$(PROGRAM_EXT): $(OBJ_DIR)/$(APP_DIR)/$(1).o $$(if $$(filter $(1),$$(STANDALONE_PROGRAMS)),,$$(COMMON_OBJS)) $$(if $$(filter $(1),$$(NO_NIO_LIB_PROGRAMS)),,$$(NIO_LIB_FILE)) | $(BIN_DIR)
	$$(call link_program)
endef

$(foreach prog,$(PROGRAMS),$(eval $(call APP_PROGRAM_RULE,$(prog))))

$(OBJ_DIR)/msdos/%.o: msdos/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(call compile_c)

define MSDOS_PROGRAM_RULE
$(BIN_DIR)/$(1)$(PROGRAM_EXT): $(OBJ_DIR)/msdos/apps/$(1).o $$(if $$(filter $(1),$$(STANDALONE_PROGRAMS)),,$$(MSDOS_LIB_OBJS)) $$(if $$(filter $(1),$$(NO_NIO_LIB_PROGRAMS)),,$$(NIO_LIB_FILE)) | $(BIN_DIR)
	$$(call link_program)
endef

$(foreach prog,$(MSDOS_PROGRAMS),$(eval $(call MSDOS_PROGRAM_RULE,$(prog))))

$(OBJ_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

$(DISK_DIR):
	mkdir -p $@

clean:
	rm -rf $(TARGET_BUILD_DIR)
