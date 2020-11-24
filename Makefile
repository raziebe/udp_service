EXEC = hs_udp_service
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
		hslog/hslog.o \
		hslog/hslog-multicast.o \
		hslog/hslog-rotate.o

LIBS = -lpthread
CC=arm-linux-gnueabihf-gcc
#CC=gcc -g
all: $(EXEC)

hs_udp_service: $(APP_OBJS)
	$(CC) -g $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS$(LDLIBS_$@)) $(LIBS) $(CFLAGS)

clean:
	rm -f *.o hslog/*.o  $(EXEC) *.gdb *.elf *.a
