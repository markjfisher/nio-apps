TARGETS := msdos atari bbc bbc-clib
DEFAULT_TARGET := $(if $(TARGET),$(TARGET),all-targets)

.PHONY: all all-targets clean disk disk-all $(TARGETS)

all: $(DEFAULT_TARGET)

$(TARGETS):
	$(MAKE) -f makefiles/build.mk TARGET=$@

all-targets: $(TARGETS)

disk:
	$(MAKE) -f makefiles/build.mk TARGET=msdos disk

disk-all:
	@for target in $(TARGETS); do \
		$(MAKE) -f makefiles/build.mk TARGET=$$target disk; \
	done

clean:
	rm -rf build
