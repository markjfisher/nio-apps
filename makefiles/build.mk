SHELL := /usr/bin/env bash
.DEFAULT_GOAL := all

TARGET ?= msdos
FUJINET_NIO_LIB ?= ../fujinet-nio-lib

include makefiles/targets.mk

FNSVC_LIST_MAX_PAYLOAD ?= 420
ifeq ($(TARGET),bbc)
FNSVC_LIST_MAX_PAYLOAD := 250
endif

CORE_APP_DIR := apps/core
TEST_APP_DIR := apps/test
CONFIG_NIO_DIR := apps/config-nio
SRC_DIR := src
APP_INCLUDE_DIR := include/common
CONFIG_NIO_INCLUDE_DIR := $(CONFIG_NIO_DIR)/include
PLATFORM_INCLUDE_DIR := include/platform/$(PLATFORM)
NIO_INCLUDE_DIR := $(FUJINET_NIO_LIB)/include
BUILD_DIR ?= build
TARGET_BUILD_DIR := $(BUILD_DIR)/$(TARGET)
OBJ_DIR := $(TARGET_BUILD_DIR)/obj
BIN_DIR := $(TARGET_BUILD_DIR)/bin
DISK_DIR := $(TARGET_BUILD_DIR)/disk

CORE_APP_SRCS := $(sort $(wildcard $(CORE_APP_DIR)/*.c))
TEST_APP_SRCS := $(sort $(wildcard $(TEST_APP_DIR)/*.c))
APP_SRCS := $(CORE_APP_SRCS) $(TEST_APP_SRCS)
APP_PROGRAMS_ALL := $(basename $(notdir $(APP_SRCS)))
APP_PROGRAMS_EXCLUDE_msdos := fsioraw keycode
APP_PROGRAMS_EXCLUDE_atari := fboot keycode
APP_PROGRAMS_EXCLUDE_bbc := fapp fboot fdrive fhost fin fls fmount fout fsioraw
APP_PROGRAMS_EXCLUDE_bbc-clib := fboot fsioraw config-nio keycode
APP_PROGRAMS_EXCLUDE_linux := keycode
APP_PROGRAMS_EXCLUDE := $(APP_PROGRAMS_EXCLUDE_$(TARGET))
APP_PROGRAMS := $(filter-out $(APP_PROGRAMS_EXCLUDE),$(APP_PROGRAMS_ALL))
CONFIG_NIO_PROGRAMS := $(if $(filter bbc-clib,$(TARGET)),,config-nio)
PROGRAMS := $(APP_PROGRAMS) $(CONFIG_NIO_PROGRAMS)
MSDOS_APP_SRCS := $(if $(filter msdos,$(TARGET)),$(sort $(wildcard msdos/apps/*.c)))
MSDOS_PROGRAMS := $(basename $(notdir $(MSDOS_APP_SRCS)))

STANDALONE_PROGRAMS := astest clock fhttpbin irqmon keycode
ifeq ($(TARGET),bbc)
STANDALONE_PROGRAMS += config-nio
endif
NO_NIO_LIB_PROGRAMS := irqmon keycode
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
else
$(error Unknown compiler family '$(COMPILER_FAMILY)' for TARGET=$(TARGET))
endif

CONFIG_NIO_SRCS_COMMON := \
	$(CONFIG_NIO_DIR)/common/config_nio_tables.c \
	$(CONFIG_NIO_DIR)/common/config_nio_state.c \
	$(CONFIG_NIO_DIR)/common/config_nio_store.c \
	$(CONFIG_NIO_DIR)/common/config_nio_browse.c \
	$(CONFIG_NIO_DIR)/common/config_nio_ui.c \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_ui.c

CONFIG_NIO_SRCS_bbc := \
	$(CONFIG_NIO_DIR)/common/fnsvc_config_nio_bbc.c \
	$(SRC_DIR)/platform/$(PLATFORM)/fnctl.c \
	$(CONFIG_NIO_DIR)/common/config_nio_state.c \
	$(CONFIG_NIO_DIR)/common/config_nio_store.c \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_template.c \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_ui.c

CONFIG_NIO_ASM_SRCS_bbc := \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/bbc_oscli.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_edit.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_path.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_screen.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_template_data.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_template_decompress.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_store_bbc.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_state.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_tables.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_xram_bank.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/fnsvc_list_dir.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/fnsvc_set_mount.s

CONFIG_NIO_SRCS := $(if $(CONFIG_NIO_SRCS_$(TARGET)),$(CONFIG_NIO_SRCS_$(TARGET)),$(CONFIG_NIO_SRCS_COMMON))
CONFIG_NIO_COMMON_OBJS := $(if $(filter bbc,$(TARGET)),,$(COMMON_OBJS))
CONFIG_NIO_MAIN_OBJ := $(OBJ_DIR)/$(CONFIG_NIO_DIR)/main.o
CONFIG_NIO_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(CONFIG_NIO_SRCS))
CONFIG_NIO_ASM_SRCS := $(CONFIG_NIO_ASM_SRCS_$(TARGET))
CONFIG_NIO_ASM_OBJS := $(patsubst %.s,$(OBJ_DIR)/%.o,$(CONFIG_NIO_ASM_SRCS))
DEPENDS += $(CONFIG_NIO_MAIN_OBJ:.o=.d) $(CONFIG_NIO_OBJS:.o=.d)

ifeq ($(TARGET),bbc)
CONFIG_NIO_BBC_TEMPLATE_INPUTS := \
	bbc/assets/config-nio-templates/CNHOSTS \
	bbc/assets/config-nio-templates/CNBROW \
	bbc/assets/config-nio-templates/CNSLOTS \
	bbc/config_nio_layout.json \
	bbc/scripts/generate_config_nio_templates.py
CONFIG_NIO_BBC_GENERATED := \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_template_data.s \
	$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_layout.h

$(CONFIG_NIO_BBC_GENERATED): $(CONFIG_NIO_BBC_TEMPLATE_INPUTS)
	python3 bbc/scripts/generate_config_nio_templates.py

$(OBJ_DIR)/$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_template_data.o: $(CONFIG_NIO_BBC_GENERATED)
$(OBJ_DIR)/$(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_ui.o: $(CONFIG_NIO_DIR)/platform/$(PLATFORM)/config_nio_layout.h
endif

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
-include makefiles/config-nio-bbc.mk

.PHONY: all clean disk boot-disk install-boot-disk $(PROGRAMS) $(MSDOS_PROGRAMS) $(DISK_TARGETS) $(BOOT_DISK_TARGETS)
.SECONDARY: $(APP_OBJS) $(COMMON_OBJS) $(CONFIG_NIO_MAIN_OBJ) $(CONFIG_NIO_OBJS) $(CONFIG_NIO_ASM_OBJS)

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
$(BIN_DIR)/$(1)$(PROGRAM_EXT): $(OBJ_DIR)/$$(patsubst %.c,%.o,$$(filter %/$(1).c,$$(APP_SRCS))) $$(if $$(filter $(1),$$(STANDALONE_PROGRAMS)),,$$(COMMON_OBJS)) $$(if $$(filter $(1),$$(NO_NIO_LIB_PROGRAMS)),,$$(NIO_LIB_FILE)) | $(BIN_DIR)
	$$(call link_program)
endef

$(foreach prog,$(APP_PROGRAMS),$(eval $(call APP_PROGRAM_RULE,$(prog))))

$(BIN_DIR)/config-nio$(PROGRAM_EXT): $(CONFIG_NIO_MAIN_OBJ) $(CONFIG_NIO_COMMON_OBJS) $(CONFIG_NIO_OBJS) $(CONFIG_NIO_ASM_OBJS) $(CONFIG_NIO_DEPS) $(NIO_LIB_FILE) | $(BIN_DIR)
	$(call link_program)

ifeq ($(TARGET),bbc)
BBC_CONFIG_NIO_START_ADDRESS ?= 0x1900
BBC_CONFIG_NIO_HIMEM ?= 0x7C00
$(BIN_DIR)/config-nio$(PROGRAM_EXT): CFLAGS += -DCONFIG_NIO_BBC_LITE
$(CONFIG_NIO_ASM_OBJS): ASMFLAGS += -DCONFIG_NIO_BBC_LITE
ifeq ($(BBC_CONFIG_NIO_SHADOW_MODE),1)
$(BIN_DIR)/config-nio$(PROGRAM_EXT): CFLAGS += -DCONFIG_NIO_BBC_SHADOW_MODE
endif
ifeq ($(BBC_CONFIG_NIO_XRAM_TABLES),1)
$(BIN_DIR)/config-nio$(PROGRAM_EXT): CFLAGS += -DCONFIG_NIO_BBC_XRAM_TABLES
$(CONFIG_NIO_ASM_OBJS): ASMFLAGS += -DCONFIG_NIO_BBC_XRAM_TABLES
endif
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
