CROSS =
CC = @echo " GCC $@"; $(CROSS)gcc
CPP = @echo " G++ $@"; $(CROSS)g++
LD = @echo " LD $@"; $(CROSS)ld
AR = @echo " AR $@"; $(CROSS)ar
RM = @echo " RM $@"; rm -f

TARGET = online

SRCS := $(wildcard src/*.c)
OBJS := $(patsubst %.c, %.o, $(SRCS))

CFLAGS += -g -Wall
CFLAGS += -I/home/chenjianping/others/codec-learning-2/rtsp-2/fifo/inc
CFLAGS += -I/home/chenjianping/others/codec-learning-2/rtsp-2/rtsp/inc
CFLAGS += -I/home/chenjianping/others/codec-learning-2/x264/_install/include
CFLAGS += -I/home/chenjianping/others/codec-learning-2/x265/_install/include

LDFLAGS += -L/home/chenjianping/others/codec-learning-2/rtsp-2/fifo/lib
LDFLAGS += -L/home/chenjianping/others/codec-learning-2/rtsp-2/rtsp/lib
LDFLAGS += -L/home/chenjianping/others/codec-learning-2/rtsp-2/v4l2/lib
LDFLAGS += -L/home/chenjianping/others/codec-learning-2/rtsp-2/x264/lib
LDFLAGS += -L/home/chenjianping/others/codec-learning-2/rtsp-2/x265/lib
LDFLAGS += -L/home/chenjianping/others/codec-learning-2/x264/_install/lib
LDFLAGS += -L/home/chenjianping/others/codec-learning-2/x265/_install/lib

LDFLAGS += -lx264 -lx265 -lgx265 -lgx264 -lrtsp_hisi -lgfifo -lgv4l2 \
		   -pthread 
		  # -pthread -lm -lz -ldl

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/chenjianping/others/codec-learning-2/x264/_install/lib
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/chenjianping/others/codec-learning-2/x265/_install/lib

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
