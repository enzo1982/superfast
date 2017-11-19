########## BoCA component makefile ##########

# Change these variables to fit your project:
TARGET	  = supercoreaudio
TYPE	  = encoder
VERSION	  = 1.0

BOCA_PATH = ../..

include $(dir $(firstword $(MAKEFILE_LIST)))/$(BOCA_PATH)/Makefile-options

# Enter object files here:
OBJECTS	  = config.o coreaudio.o dllinterface.o worker.o

# Enter additional defines here:
DEFINE	  = -Wno-multichar

ifneq ($(BUILD_OSX),True)
	DEFINE += -I$(SRCDIR)
endif

ifeq ($(BUILD_WIN32),True)
	DEFINE += -DUNICODE
endif

# Enter additional library dependencies here:
ifeq ($(BUILD_WIN32),True)
	LIBS	= -lole32
else ifeq ($(BUILD_OSX),True)
	LIBS	= -framework CoreFoundation -framework AudioToolbox
endif

# Enter addition commands for targets all and clean here:
ALLCMD1   =
ALLCMD2   =
CLEANCMD1 =
CLEANCMD2 =
INSTCMD1  =
INSTCMD2  = 
UINSTCMD1 =
UINSTCMD2 =

## Do not change anything below this line. ##

include $(dir $(firstword $(MAKEFILE_LIST)))/$(BOCA_PATH)/Makefile-commands
