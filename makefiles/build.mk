SHELL := /usr/bin/env bash
.DEFAULT_GOAL := all

TARGET ?= msdos
FUJINET_NIO_LIB ?= ../fujinet-nio-lib
FNSVC_LIST_MAX_PAYLOAD ?= 420

include makefiles/targets.mk

ifeq ($(TARGET),bbc)
FNSVC_LIST_MAX_PAYLOAD := 180
endif

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

COMMON_APP_SRCS := $(sort $(wildcard $(APP_DIR)/*.c))
COMMON_PROGRAMS_ALL := $(basename $(notdir $(COMMON_APP_SRCS)))
COMMON_PROGRAMS_EXCLUDE_msdos := fsioraw
COMMON_PROGRAMS_EXCLUDE_atari := fboot
COMMON_PROGRAMS_EXCLUDE_bbc := fboot fsioraw
COMMON_PROGRAMS_EXCLUDE_bbc-clib := fboot fsioraw config-nio
COMMON_PROGRAMS_EXCLUDE := $(COMMON_PROGRAMS_EXCLUDE_$(TARGET))
PROGRAMS := $(filter-out $(COMMON_PROGRAMS_EXCLUDE),$(COMMON_PROGRAMS_ALL))
MSDOS_APP_SRCS := $(if $(filter msdos,$(TARGET)),$(sort $(wildcard msdos/apps/*.c)))
MSDOS_PROGRAMS := $(basename $(notdir $(MSDOS_APP_SRCS)))

STANDALONE_PROGRAMS := astest clock fhttpbin irqmon
NO_NIO_LIB_PROGRAMS := irqmon
COMMON_SRCS := $(SRC_DIR)/common/fnsvc.c $(SRC_DIR)/platform/$(PLATFORM)/fnctl.c
COMMON_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
APP_OBJS := $(PROGRAMS:%=$(OBJ_DIR)/$(APP_DIR)/%.o)
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
else
$(error Unknown compiler family '$(COMPILER_FAMILY)' for TARGET=$(TARGET))
endif

CONFIG_NIO_SRCS_COMMON := \
	$(SRC_DIR)/common/config_nio_state.c \
	$(SRC_DIR)/common/config_nio_store.c \
	$(SRC_DIR)/common/config_nio_browse.c \
	$(SRC_DIR)/common/config_nio_ui.c \
	$(SRC_DIR)/platform/$(PLATFORM)/config_nio_ui.c

CONFIG_NIO_SRCS_bbc := \
	$(SRC_DIR)/common/config_nio_state.c \
	$(SRC_DIR)/common/config_nio_store.c \
	$(SRC_DIR)/platform/$(PLATFORM)/config_nio_ui.c

CONFIG_NIO_SRCS := $(if $(CONFIG_NIO_SRCS_$(TARGET)),$(CONFIG_NIO_SRCS_$(TARGET)),$(CONFIG_NIO_SRCS_COMMON))
CONFIG_NIO_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(CONFIG_NIO_SRCS))
DEPENDS += $(CONFIG_NIO_OBJS:.o=.d)

ifeq ($(TARGET),msdos)
PDCURSES_DIR ?= ../PDCurses
PDCURSES_MSDOS_LIB ?= ../../build/pdcurses/msdos-small/pdcurses.lib
CONFIG_NIO_LIBS := $(PDCURSES_MSDOS_LIB)
CONFIG_NIO_DEPS := $(PDCURSES_MSDOS_LIB)
CFLAGS += -i=$(PDCURSES_DIR)
endif

DISK_TARGETS :=
-include makefiles/disk-$(TARGET).mk
-include makefiles/boot-disk.mk

.PHONY: all clean disk boot-disk install-boot-disk $(PROGRAMS) $(MSDOS_PROGRAMS) $(DISK_TARGETS) $(BOOT_DISK_TARGETS)
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

$(BIN_DIR)/%$(PROGRAM_EXT): $(OBJ_DIR)/$(APP_DIR)/%.o $(COMMON_OBJS) $(NIO_LIB_FILE) | $(BIN_DIR)
	$(call link_program)

define COMMON_PROGRAM_RULE
$(BIN_DIR)/$(1)$(PROGRAM_EXT): $(OBJ_DIR)/$(APP_DIR)/$(1).o $$(if $$(filter $(1),$$(STANDALONE_PROGRAMS)),,$$(COMMON_OBJS)) $$(if $$(filter $(1),config-nio),$$(CONFIG_NIO_OBJS) $$(CONFIG_NIO_DEPS)) $$(if $$(filter $(1),$$(NO_NIO_LIB_PROGRAMS)),,$$(NIO_LIB_FILE)) | $(BIN_DIR)
	$$(call link_program)
endef

$(foreach prog,$(PROGRAMS),$(eval $(call COMMON_PROGRAM_RULE,$(prog))))

ifeq ($(TARGET),bbc)
CFLAGS += -DCONFIG_NIO_BBC_LITE -DFNSVC_IO_BUF_SIZE=256 -DFNSVC_LIST_NAME_MAX=64
BBC_CONFIG_NIO_START_ADDRESS ?= 0x1900
BBC_CONFIG_NIO_HIMEM ?= 0x9000
$(BIN_DIR)/config-nio$(PROGRAM_EXT): LDFLAGS := -t $(TARGET) --start-addr $(BBC_CONFIG_NIO_START_ADDRESS) -Wl -D,__HIMEM__=$(BBC_CONFIG_NIO_HIMEM)
endif

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
