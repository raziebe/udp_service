#define _LARGEFILE_SOURCE
#define _GNU_SOURCE
//
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "hslog.h"

#define MAX_SIZE 10000
char *get_logfilename(void);
int get_logfilesize(void);
void hslog_sync_lock(void);
void hslog_sync_unlock(void);

void GetTimestamp(char *strtimestamp, int length)
{    
	if (length > 19) {

	struct timeval tv;
	struct tm lt;

        memset(strtimestamp, 0, length);
	if (gettimeofday(&tv, NULL) == 0 && localtime_r(&tv.tv_sec, &lt) != NULL) {
		if (snprintf(strtimestamp, length, "%04d-%02d-%02d-%02d-%02d-%02d", lt.tm_year+1900,
        	         lt.tm_mon+1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec) != 19) {
				printf("INVALID TIME: ");
        		        memset(strtimestamp, 0, length);
			}
		} else {
			printf("GET TIME ERR: ");
		}
	} else {
		memset(strtimestamp, 0, length);
	}
}

void hslog_rotatelogfile(void)
{
    int fd = -1;
    int res = 0;
    struct stat sb;
    char logfilename[256];
    char timeStamp[32];
    char archiveName[256];
    unsigned long long fileSize = 0;

    memset(logfilename, 0, sizeof(logfilename));
    sprintf(logfilename, "%s.log", get_logfilename());

    hslog_sync_lock();
    fd = open(logfilename, O_CREAT|O_APPEND|O_WRONLY|O_LARGEFILE, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fd < 0) {
        printf("Error open: %s\n", strerror(errno));
        hslog_sync_unlock();
        return;
    }

    // Get file size
    res = fstat(fd, &sb);
    if (res != 0) {
        printf("Error fstat res(%d): %d (%s)\n", res, errno, strerror(errno));
        hslog_sync_unlock();
        return;
    }

    fileSize = sb.st_size;
    if(fileSize > get_logfilesize()){
        // Needs log rotation.
        GetTimestamp(timeStamp, sizeof(timeStamp));
        close(fd);
        memset(archiveName, 0, sizeof(archiveName));
        snprintf(archiveName, sizeof(archiveName), "%s-%s.log", get_logfilename(), timeStamp);
        rename(logfilename, archiveName);
        hslog(HSLOG_INFO, "Logfile archived.");
    }
    else{
        // No need to rotate log file yet.
        close(fd);
    }

    hslog_sync_unlock();
    return;
}
