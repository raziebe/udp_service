#ifndef   __SERVICE_CHECK__
#define  __SERVICE_CHECK__

	
extern int service_disabled;

int start_service_check(char *sockname);

#define SERV_CTRL_SOCKNAME	"/tmp/udp_service_control"

#endif
