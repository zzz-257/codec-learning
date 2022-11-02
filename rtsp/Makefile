make_cross=1

ifdef make_cross
	CROSS_COMPILE =/home/chenjianping/gerrit/nv100/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
endif

CC = @echo " GCC $@"; $(CROSS_COMPILE)gcc
CPP = @echo " G++ $@"; $(CROSS_COMPILE)g++
LD = @echo " LD $@"; $(CROSS_COMPILE)ld
AR = @echo " AR $@"; $(CROSS_COMPILE)ar
RM = @echo " RM $@"; rm -f

TARGET = online

SRCS += $(wildcard fifo/src/*.c)
SRCS += $(wildcard rtsp/src/*.c)
SRCS += $(wildcard v4l2/src/*.c)
SRCS += $(wildcard x264/src/*.c)
SRCS += $(wildcard x265/src/*.c)
SRCS += $(wildcard test/src/main-online.c)
#SRCS += $(wildcard test/src/main-native.c)

OBJS := $(patsubst %.c, %.o, $(SRCS))

CFLAGS += -g -c -Wall -D__LINUX__

CFLAGS += -I./fifo/inc
CFLAGS += -I./rtsp/inc
ifdef make_cross
	CFLAGS += -I../x264/_install_cross/include
	CFLAGS += -I../x265/_install_cross/include
else
	CFLAGS += -I../x264/_install/include
	CFLAGS += -I../x265/_install/include
endif
ifdef make_cross
	LDFLAGS += -L../x264/_install_cross/lib
	LDFLAGS += -L../x265/_install_cross/lib
else
	LDFLAGS += -L../x264/_install/lib
	LDFLAGS += -L../x265/_install/lib
endif

LDFLAGS += -lx264 -lx265 -pthread -ldl -lm -lstdc++ -static

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(RM) $@;
	@$(CC) $^ $(LDFLAGS) -o $@;

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -vrf $(TARGET) $(OBJS);
	@rm -rvf $(OBJSTMP);
	@find ./ -type f \( -name '*.elf*' -o -name '*.gdb' \) | xargs rm -f
	@rm -rvf *.h264 *.h265
	@ctags -R
	@clear
