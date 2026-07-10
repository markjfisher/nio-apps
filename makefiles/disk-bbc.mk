BBC_IMAGE ?= $(DISK_DIR)/nio-apps-bbc.ssd
CREATE_SSD ?= $(FUJINET_NIO_LIB)/scripts/create_ssd.py
BBC_STAGE_DIR ?= $(DISK_DIR)/stage

DFS_NAME_fhost := FHOST
DFS_NAME_fls := FLS
DFS_NAME_fin := FIN
DFS_NAME_fmount := FMOUNT
DFS_NAME_fdrive := FDRIVE
DFS_NAME_fapp := FAPP
DFS_NAME_fhttpbin := FHTTP
DFS_NAME_astest := ASTEST
DFS_NAME_clock := CLOCK

define BBC_DFS_NAME
$(or $(DFS_NAME_$(1)),$(1))
endef

.PHONY: disk-bbc
disk-bbc: all | $(DISK_DIR)
	rm -rf "$(BBC_STAGE_DIR)"
	mkdir -p "$(BBC_STAGE_DIR)"
	$(foreach prog,$(PROGRAMS),cp "$(BIN_DIR)/$(prog)" "$(BBC_STAGE_DIR)/$(call BBC_DFS_NAME,$(prog))";)
	$(foreach prog,$(PROGRAMS),printf '$$.%s 001900 001900\n' "$(call BBC_DFS_NAME,$(prog))" > "$(BBC_STAGE_DIR)/$(call BBC_DFS_NAME,$(prog)).inf";)
	python3 "$(CREATE_SSD)" -i "$(BBC_STAGE_DIR)" -o "$(BBC_IMAGE)" -t NIOAPPS

DISK_TARGETS += disk-bbc
