TARGETS := msdos atari
DEFAULT_TARGET := $(if $(TARGET),$(TARGET),msdos)

.PHONY: all all-targets clean disk $(TARGETS)

all: $(DEFAULT_TARGET)

$(TARGETS):
	$(MAKE) -f makefiles/build.mk TARGET=$@

all-targets: $(TARGETS)

disk:
	$(MAKE) -f makefiles/build.mk TARGET=msdos disk

clean:
	@for target in $(TARGETS); do \
		$(MAKE) -f makefiles/build.mk TARGET=$$target clean; \
	done
