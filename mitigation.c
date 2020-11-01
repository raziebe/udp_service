#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include "hslog/hslog.h"

int get_fifo_avail(const char *dev, size_t *avail)
{
       int fd;
       int ret;
       struct stat sb;

       fd = open(dev, O_WRONLY | O_NONBLOCK);
       if (fd < 0){
		hslog_error("get fifo size: failed to open");
		return -1;
       }

       ret = fstat(fd, &sb);
       if(ret) {
		hslog_error("get fifo size: failed to fstat");
		close(fd);
		return ret;
       }
 
       ret =  ioctl(fd, _IOR(minor(sb.st_rdev), 0, size_t*), avail);
       if (ret < 0 ){
	  hslog_error("get fifo size:");
       }	      
       close(fd);
       return ret;
}
