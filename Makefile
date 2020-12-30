# UDP and r-UDP services Makefile for both PC (simulation) and ARM (Terminal) platforms
#

#------------- Configuration -------------------
VERSION = 301
BRANCH := $(shell git rev-parse --abbrev-ref HEAD)

EXEC = hs_rudp_service
ifeq "$(ARCH)" "arm" #32bit arm arch
CROSS_TOOLS_PATH = ~/BSP/hiSkyDev/3rdparty/zedboard/toolchain/gcc-arm-linux-gnueabi/bin/#
CROSS_TOOLS_PREFIX = arm-linux-gnueabihf-
ARCH = arm
ARCH_FLAG=ARM
else 	# default x86 arch
CROSS_TOOLS_PATH = 
CROSS_TOOLS_PREFIX = 
ARCH = x86
ARCH_FLAG=X86
endif

CC = $(CROSS_TOOLS_PATH)$(CROSS_TOOLS_PREFIX)gcc
AR = $(CROSS_TOOLS_PATH)$(CROSS_TOOLS_PREFIX)ar

DBG_FLAGS := -O0 -g -D_DEBUG -ffunction-sections -fdata-sections
GCC_FLAGS := -std=gnu99 -Wall -Wextra -Wshadow -Wdouble-promotion -Wformat=2 -Wundef -fno-common -fstack-usage -Wconversion #-Werror 
CFLAGS :=$(DBG_FLAGS) $(GCC_FLAGS) # $(IDIR)

APP_OBJS = 	hs_udp.o \
		mitigation.o \
		sensor_stats.o \
		conf_read.o \
		hs_udp_service.o \
		service_check.o \
		hs_fifo.o \
		hs_config_file_reader.o \
		main.o \
		hs_buffer_pool.o \
		Terminals_Handler.o \
		hs_udp_fragments.o \
		inotify.o \
		r_udp.o \
		rudp_req.o \
		hslog/hslog.o \
		hslog/hslog-multicast.o \
		hslog/hslog-rotate.o

LIBS = -lpthread

all: $(EXEC)

hs_rudp_service: $(APP_OBJS)
	$(CC) -g $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS$(LDLIBS_$@)) $(LIBS) $(CFLAGS)

clean:
	rm -f *.o hslog/*.o  $(EXEC) *.gdb *.elf *.a
