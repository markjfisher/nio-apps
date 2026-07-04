ATARI_STAGE_DIR ?= $(DISK_DIR)/atari-stage

.PHONY: disk-atari
disk-atari: all | $(DISK_DIR)
	rm -rf $(ATARI_STAGE_DIR)
	mkdir -p $(ATARI_STAGE_DIR)
	cp $(BIN_DIR)/$(TARGET)/* $(ATARI_STAGE_DIR)/
	@echo "Staged Atari executables in $(ATARI_STAGE_DIR)"
	@echo "ATR image creation is intentionally a pluggable follow-up target."

DISK_TARGETS += disk-atari
