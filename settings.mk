
CC=gcc

LINUX_CC := clang
LINUX_CXX := clang++
#LINUX_CFLAGS := -Doverride=""

WIN_CFLAGS := -DWIN32_LEAN_AND_MEAN -O2
LINUX_CFLAGS :=  -O2
CFLAGS := -g
PI_CFLAGS := -pipe -march=armv6j -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard
CXXFLAGS := -std=c++0x
