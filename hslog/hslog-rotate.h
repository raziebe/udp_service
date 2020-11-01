#ifndef __HS_LOG_ROTATE_H
#define __HS_LOG_ROTATE_H

#define LOG_MAX_SIZE 10000 //500000

void hslog_rotatelogfile(void);
void GetTimestamp(char *, int);
void debug_datestamp(void);

#endif // __HS_LOG_ROTATE_H