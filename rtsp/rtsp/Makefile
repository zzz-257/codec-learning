CROSS_COMPILE =
CC = @echo " GCC $@"; $(CROSS_COMPILE)gcc
CPP = @echo " G++ $@"; $(CROSS_COMPILE)g++
LD = @echo " LD $@"; $(CROSS_COMPILE)ld
AR = @echo " AR $@"; $(CROSS_COMPILE)ar
RM = @echo " RM $@"; rm -f

LIB_NAME = rtsp_hisi
STATIC_NAME = lib$(LIB_NAME).a
SHARE_NAME = lib$(LIB_NAME).so

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

CFLAGS += -g -Wall -D__LINUX__
CFLAGS += -I./inc
LDFLAGS += #-L./lib
LDFLAGS += #-lx264 -lavformat -lavfilter -lavcodec -lswscale -lavutil -lswresample -lavdevice -pthread -lm -lz -ldl -lstdc++

TARGET:$(STATIC_NAME) #$(SHARE_NAME)

$(STATIC_NAME): $(OBJS)
	@$(AR) -cr $(STATIC_NAME) $(OBJS)

$(SHARE_NAME): $(OBJS)
	@$(CC) -shared -fPIC -o $(SHARE_NAME) $(SRCS) $(CFLAGS)

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -vrf $(OBJS) $(STATIC_NAME) $(SHARE_NAME)
	@find ./ -type f \( -name '*.elf*' -o -name '*.gdb' \) | xargs rm -f
