MSDOS_IMAGE ?= $(DISK_DIR)/nio-apps-msdos.img

.PHONY: disk-msdos
disk-msdos: all | $(DISK_DIR)
	python3 msdos/scripts/create_msdos_img.py -i $(BIN_DIR) -o $(MSDOS_IMAGE)

DISK_TARGETS += disk-msdos
