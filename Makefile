########## BoCA directory makefile ##########

BOCA_PATH = .

include $(dir $(firstword $(MAKEFILE_LIST)))/$(BOCA_PATH)/Makefile-options

all:
	mkdir -p $(BOCA_PATH)/$(BINDIR) $(BOCA_PATH)/$(LIBDIR)

	+ $(call makein,components)

clean:
	+ $(call cleanin,components)

ifneq ($(SRCDIR),$(CURDIR))
	rmdir $(BOCA_PATH)/$(BINDIR) $(BOCA_PATH)/$(LIBDIR) || true
endif

install:
ifneq ($(BUILD_WIN32),True)
	$(call makein,components,install)
endif

uninstall:
ifneq ($(BUILD_WIN32),True)
	$(call makein,components,uninstall)
endif
