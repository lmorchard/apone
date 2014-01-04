
CFLAGS :=
CXXFLAGS :=
OBJS :=
MODULES :=
LIBS :=
LDFLAGS :=
TARGET :=
FILES :=
INCLUDES :=
OBJDIR := obj/

SRC_PATTERNS := .cpp .cxx .cc .c .s .glsl

ifeq ($(CC),cc)
CC = gcc
endif

# Figure out HOST if it is not set
ifeq ($(HOST),)

ifneq ($(EMSCRIPTEN),)
	HOST = emscripten
else ifeq ($(OS),Windows_NT)
	HOST = windows
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
    endif
else
    UNAME_S := $(shell uname -s)
    UNAME_N := $(shell uname -n)
    ifeq ($(UNAME_S),Linux)
    	HOST = linux
    endif
    ifeq ($(UNAME_N),raspberrypi)
    	HOST = raspberrypi
    	ARM = 1
    endif

    ifeq ($(UNAME_S),Darwin)
    	HOST = apple
    endif

    UNAME_P := $(shell uname -p)
    ifeq ($(UNAME_P),x86_64)

    endif

    ifneq ($(filter %86,$(UNAME_P)),)
        #CCFLAGS += -D IA32
    endif

    ifneq ($(filter arm%,$(UNAME_P)),)
        ARM = 1
    endif
endif

endif

ifeq ($(HOST),android)

  APP_PLATFORM=android-10
  NDK_PLATFORM=android-14
  PREFIX=arm-linux-androideabi-
  TARGET_PRE=lib
  TARGET_EXT=.so
  CFLAGS += -DANDROID
  LDFLAGS += -fPIC -Wl,-shared -no-canonical-prefixes -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now 

  # Test for prerequisites
  ACC := $(shell which $(PREFIX)$(CC))
  ifneq ($(ACC),)
    ANDROID_TOOLCHAIN := $(subst /bin/,,$(dir $(ACC)))
    $(info Android toolchain at $(ANDROID_TOOLCHAIN))
  else
    $(error Can not find Android C compiler)
  endif

  ANDROID_TOOL := $(shell which android)
  ifneq ($(ANDROID_TOOL),)
    ANDROID_SDK := $(subst /tools/,,$(dir $(ANDROID_TOOL)))
    $(info Android SDK at $(ANDROID_SDK))
  else
    $(error Can not find Android SDK. Make sure the 'android' command is in your path.)
  endif

  ifeq ($(ANDROID_NDK),)
    SDK_PARENT := $(dir $(ANDROID_SDK))
    ANDROID_NDK := $(lastword $(sort $(wildcard $(SDK_PARENT)*ndk*)))
    ifneq ($(ANDROID_NDK),)
      $(info Found NDK at $(ANDROID_NDK))
    else
      $(error Can not find an NDK)
    endif
  endif    

  ifeq ($(shell which ant),)
    $(error You need ant to build android projects)
  endif

else ifeq ($(HOST),emscripten)
	CC = emcc
	CXX = em++
 	TARGET_EXT = .html
 	CFLAGS += -DGL_ES
 	COMP_CFLAGS += -Wno-warn-absolute-paths
else ifeq ($(HOST),raspberrypi)
	CFLAGS += -DRASPBERRYPI
endif

ifneq ($(ARM),)
	CFLAGS += -DARM
endif

ifndef CGC
CGC := cgc
endif
ifndef XXD
XXD := xxd
endif


CC := $(PREFIX)$(CC)$(C_VERSION)
CXX := $(PREFIX)$(CXX)$(C_VERSION)

#ifneq ($(USE_CCACHE),)
#CC := ccache $(CC)
#CXX := ccache $(CXX)
#endif

LD := $(CXX)
ANT := ant

ifeq ($(AR),)
AR := ar
endif
RANLIB=ranlib
AS := $(PREFIX)as
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
