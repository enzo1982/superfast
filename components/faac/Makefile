########## BoCA component makefile ##########

# Change these variables to fit your project:
TARGET	  = superfaac
TYPE	  = encoder
VERSION	  = 1.0

BOCA_PATH = ../..

# Enter object files here:
OBJECTS	  = config.o dllinterface.o faac.o worker.o

# Enter additional defines here:
DEFINE	  = -I$(SRCDIR) -DFAACAPI=

# Enter additional library dependencies here:
LIBS	  =

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
