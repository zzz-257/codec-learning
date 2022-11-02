CROSS_COMPILE =
CC = @echo " GCC $@"; $(CROSS_COMPILE)gcc
CPP = @echo " G++ $@"; $(CROSS_COMPILE)g++
LD = @echo " LD $@"; $(CROSS_COMPILE)ld
AR = @echo " AR $@"; $(CROSS_COMPILE)ar
RM = @echo " RM $@"; rm -f

LIB_NAME = gx265
STATIC_NAME = lib$(LIB_NAME).a
#SHARE_NAME = lib$(LIB_NAME).so

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

CFLAGS += -g -O0 -Wall
CFLAGS += -I../fifo/inc
CFLAGS += -I/home/chenjianping/others/codec-learning/x265/_install/include
LDFLAGS += -L/home/chenjianping/others/codec-learning/x265/_install/lib -lx265 -lpthread

TARGET:$(STATIC_NAME) #$(SHARE_NAME)

$(STATIC_NAME): $(OBJS)
	@$(AR) -cr $(STATIC_NAME) $(OBJS)

#$(SHARE_NAME): $(OBJS)
#	@$(CC) -shared -fPIC -o $(SHARE_NAME) $(SRCS) $(CFLAGS)

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -vrf $(OBJS) $(STATIC_NAME) #$(SHARE_NAME)
	@find ./ -type f \( -name '*.elf*' -o -name '*.gdb' \) | xargs rm -f
