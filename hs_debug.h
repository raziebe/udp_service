#ifndef __HS_DEBUG_H
#define __HS_DEBUG_HEBUG

#define DEBUG
#define LOG_MAX_SIZE 10000 //500000

void rotatelogfile(void);
void exportlogfile(void);
void archivelogfile(void);
void removelogfile(void);
void GetTimestamp(char *, int);
void debug_datestamp(void);

#ifdef DEBUG
	#define debug(fmt, ...) ( { char strtimestamp[32];GetTimestamp(strtimestamp, sizeof(strtimestamp));log_debug_message("%s"fmt, strtimestamp, ##__VA_ARGS__);} )
#else
	#define debug(fmt, ...)
#endif

#endif