###########################################
### EDIT THESE PATHS FOR YOUR OWN SETUP ###
###########################################

HL2SDK_DOTA = ../hl2sdk-dota
MMSOURCE = ../metamod-source

#####################################
### EDIT BELOW FOR OTHER PROJECTS ###
#####################################

PROJECT = d2vdump

OBJECTS = \
	d2vdump.cpp    \
	jsondumper.cpp

##############################################
### CONFIGURE ANY OTHER FLAGS/OPTIONS HERE ###
##############################################

C_OPT_FLAGS = -DNDEBUG -O3 -funroll-loops -pipe -fno-strict-aliasing
C_DEBUG_FLAGS = -D_DEBUG -DDEBUG -g -ggdb3
C_GCC4_FLAGS = -fvisibility=hidden -fPIC
CPP_GCC4_FLAGS = -fvisibility-inlines-hidden
CPP = clang

HL2PUB = $(HL2SDK_DOTA)/public

INCLUDE += -I../jansson-2.5/src
METAMOD = $(MMSOURCE)/core

LIB_EXT = so
HL2LIB = $(HL2SDK_DOTA)/lib/linux

LIB_PREFIX = lib
LIB_SUFFIX = .$(LIB_EXT)

INCLUDE += -I. -I.. 

LINK += -Wl,--exclude-libs,ALL -lm -lgcc_eh -lstdc++ $(HL2LIB)/tier1_i486.a $(LIB_PREFIX)vstdlib$(LIB_SUFFIX) $(LIB_PREFIX)tier0$(LIB_SUFFIX) $(HL2LIB)/interfaces_i486.a linuxdeps/libjansson.a

INCLUDE += -I$(HL2PUB) -I$(HL2PUB)/engine -I$(HL2PUB)/tier0 -I$(HL2PUB)/tier1 -I$(METAMOD) \
	-I$(METAMOD)/sourcehook 

LINK += -m64 -lm -ldl -shared

CFLAGS += -D_LINUX -DLINUX -DPOSIX -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
	-D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -DCOMPILER_GCC -Wall \
	-Wno-overloaded-virtual -Wno-switch -Wno-unused -msse -DHAVE_STDINT_H -m64 -DPLATFORM_64BITS
CPPFLAGS += -Wno-non-virtual-dtor -fno-exceptions -std=c++11

################################################
### DO NOT EDIT BELOW HERE FOR MOST PROJECTS ###
################################################

BINARY = $(PROJECT).$(LIB_EXT)

ifeq "$(DEBUG)" "true"
	BIN_DIR = Debug
	CFLAGS += $(C_DEBUG_FLAGS)
else
	BIN_DIR = Release
	CFLAGS += $(C_OPT_FLAGS)
endif

LIB_EXT = so

IS_CLANG := $(shell $(CPP) --version | head -1 | grep clang > /dev/null && echo "1" || echo "0")

ifeq "$(IS_CLANG)" "1"
	CPP_MAJOR := $(shell $(CPP) --version | grep clang | sed "s/.*version \([0-9]\)*\.[0-9]*.*/\1/")
	CPP_MINOR := $(shell $(CPP) --version | grep clang | sed "s/.*version [0-9]*\.\([0-9]\)*.*/\1/")
else
	CPP_MAJOR := $(shell $(CPP) -dumpversion >&1 | cut -b1)
	CPP_MINOR := $(shell $(CPP) -dumpversion >&1 | cut -b3)
endif

# If not clang
ifeq "$(IS_CLANG)" "0"
	CFLAGS += -mfpmath=sse
endif

# Clang || GCC >= 4
ifeq "$(shell expr $(IS_CLANG) \| $(CPP_MAJOR) \>= 4)" "1"
	CFLAGS += $(C_GCC4_FLAGS)
	CPPFLAGS += $(CPP_GCC4_FLAGS)
endif

# Clang >= 3 || GCC >= 4.7
ifeq "$(shell expr $(IS_CLANG) \& $(CPP_MAJOR) \>= 3 \| $(CPP_MAJOR) \>= 4 \& $(CPP_MINOR) \>= 7)" "1"
	CFLAGS += -Wno-delete-non-virtual-dtor
endif

# OS is Linux and not using clang
#ifeq "$(shell expr $(IS_CLANG) \= 0)" "1"
#	LINK += -static-libgcc
#endif

OBJ_BIN := $(OBJECTS:%.cpp=$(BIN_DIR)/%.o)

MAKEFILE_NAME := $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))

$(BIN_DIR)/%.o: %.cpp
	$(CPP) $(INCLUDE) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

all:
	mkdir -p $(BIN_DIR)
	ln -sf $(HL2LIB)/$(LIB_PREFIX)vstdlib$(LIB_SUFFIX); \
	ln -sf $(HL2LIB)/$(LIB_PREFIX)tier0$(LIB_SUFFIX); \
	$(MAKE) -f $(MAKEFILE_NAME) extension

extension: $(OBJ_BIN)
	$(CPP) $(INCLUDE) $(OBJ_BIN) $(LINK) -o $(BIN_DIR)/$(BINARY)

debug:
	$(MAKE) -f $(MAKEFILE_NAME) all DEBUG=true

default: all

clean:
	rm -rf $(BIN_DIR)/*.o
	rm -rf $(BIN_DIR)/$(BINARY)

