CROSS =
CC = @echo " GCC $@"; $(CROSS)gcc
CPP = @echo " G++ $@"; $(CROSS)g++
LD = @echo " LD $@"; $(CROSS)ld
AR = @echo " AR $@"; $(CROSS)ar
RM = @echo " RM $@"; rm -f

TARGET = venctest

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

CFLAGS += -g -O0 -Wall
CFLAGS += -I./inc
LDFLAGS += -L./lib
LDFLAGS += -lx264 -lx265 -lavformat -lavfilter -lavcodec -lswscale -lavutil -lswresample -lavdevice -pthread -lm -lz -ldl -lstdc++

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
	@clear
