#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>


#include "sensor_stats.h"
#include "hslog/hslog.h"

int service_disabled;
static int listensock;
static int termsock;

#define TERM_APP_SERVICE_DISABLE 0x010
#define TERM_APP_SERVICE_ENABLE 0x011
#define TERM_APP_SERVICE_STATS	0x18

#define TERM_APP_OK 	0x00
#define TERM_APP_ERROR	0xFF

typedef struct term_app_control_msg_{
	uint32_t opcode;
	union {
		sensor_stats_t stats;
		char buf[1000];
	}u;
} term_app_control_msg_t;


void process_term_app_msg(term_app_control_msg_t* termmsg)
{
	int bytes;

	switch (termmsg->opcode)
	{
	case TERM_APP_SERVICE_DISABLE:
			termmsg->opcode = TERM_APP_OK;
			service_disabled=1;
			bytes = write(termsock, termmsg, sizeof(*termmsg));
			if (bytes < 0){
				hslog_error("Service disabled Failed to tx:");
			}
			hslog_error("Service Disabled\n");
		break;

	case   TERM_APP_SERVICE_ENABLE:
			termmsg->opcode = TERM_APP_OK;
			service_disabled=0;
			bytes = write(termsock, termmsg, sizeof(*termmsg));
			if (bytes < 0){
				hslog_error("Service enabled Failed to tx:");
			}
			hslog_error("Service Enabled\n");
		break;

	case   TERM_APP_SERVICE_STATS:
			get_sensor_stats(&termmsg->u.stats);
			bytes = write(termsock, termmsg, sizeof(*termmsg));
			if (bytes < 0){
				hslog_error("Service stats failed to tx:");
			}
		break;

	default:
		hslog_error("Unknown msg %d\n",termmsg->opcode);
	}
}


void* service_control(void * dummy)
{
	    int rc,rval;
	    int nfds = 1;
	    int nr_msgs = 0;
	    struct pollfd fds[2];
	    term_app_control_msg_t termmsg;

	    for  (;;) {

	    	nfds = 1;
			fds[0].fd = listensock;
			fds[0].events =  POLLIN;
			fds[0].revents = 0;
			if (termsock > 0) {
				fds[1].fd = termsock;
				fds[1].events = POLLIN;
				fds[1].revents = 0;
				nfds = 2;
			}

			int timeout = 5000;
			rc = poll(fds, nfds, timeout);
			if (rc == 0){
				// timeout.
				continue;
			}

			if (rc < 0){
				hslog_error("Polling error %s\n",strerror(errno));
				continue;
			}

			if (fds[0].revents & POLLIN) {

				termsock = accept(listensock, 0, 0);
				if (termsock == -1){
					hslog_error("accept");
				}
				continue;
			}

			if (fds[1].revents & (POLLHUP|POLLERR)){
				close(termsock);
				termsock = -1;
				continue;
			}

			if (fds[1].revents & POLLIN) {

				bzero(&termmsg, sizeof(termmsg));
				rval = read(termsock, &termmsg, sizeof(termmsg));
				if (rval <= 0) {
					hslog_error("reading stream message");
					close(termsock);
					termsock=-1;
					continue;
				}

				process_term_app_msg(&termmsg);

			}
	    }
	    close(listensock);
}




int start_service_check(char *sockname)
{
    struct sockaddr_un server;
    pthread_t tid;

    unlink(sockname);
    hslog_error("Starting term app control service\n");
    listensock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listensock < 0) {
        hslog_error("opening stream socket");
        return -1;
    }

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, sockname);

    if (bind(listensock, (struct sockaddr *) &server, sizeof(struct sockaddr_un))) {
        hslog_error("failed on binding stream socket");
        return -1;
    }

    listen(listensock, 2);
   
    if (!pthread_create(&tid, NULL, service_control, NULL)) {
	return 0;
    }
    hslog_error("Service control thread failed");
    return -1;
}

