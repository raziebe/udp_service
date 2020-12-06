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

#define TERM_APP_SERVICE_DISABLE 0x010
#define TERM_APP_SERVICE_ENABLE 0x011
#define TERM_APP_SERVICE_STATS	0x18

#define TERM_APP_HS_UDP_SRV_TX_PORT 13011
#define TERM_APP_HS_UDP_SRV_RX_PORT 13012



#define TERM_APP_OK 	0x00
#define TERM_APP_ERROR	0xFF

typedef struct __attribute__ ((packed)) term_app_control_msg_{
	uint32_t opcode;
	union {
		sensor_stats_t stats;
		char buf[1000];
	}u;
}   __attribute__ ((packed)) term_app_control_msg_t;

static int term_udp_send(void *msg, int size)
{
	int res;
	int sock;
	struct sockaddr_in tx_addr = {0};

	tx_addr.sin_family = AF_INET; 
    	tx_addr.sin_addr.s_addr = INADDR_ANY;
	tx_addr.sin_port = htons(TERM_APP_HS_UDP_SRV_RX_PORT);
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	res = sendto(sock, 
			msg, 
			size,
			 MSG_CONFIRM,
			(const struct sockaddr *) &tx_addr, 
			(socklen_t) sizeof(tx_addr));
	close(sock);
	return res;
}

void process_term_app_msg(term_app_control_msg_t* termmsg)
{
	int bytes;
	
	hslog_debug("opcode %x\n",termmsg->opcode);
	switch (termmsg->opcode)
	{
	case TERM_APP_SERVICE_DISABLE:
			termmsg->opcode  = TERM_APP_OK;
			service_disabled = 1;
			bytes = term_udp_send(termmsg, sizeof(*termmsg));
			if (bytes < 0){
				hslog_error("Service disabled Failed to tx:");
			}
			hslog_error("Service Disabled\n");
		break;

	case   TERM_APP_SERVICE_ENABLE:
			termmsg->opcode  = TERM_APP_OK;
			service_disabled =  0;
			bytes = term_udp_send(termmsg, sizeof(*termmsg));
			if (bytes < 0){
				hslog_error("Service enabled Failed to tx:");
			}
			hslog_error("Service Enabled\n");
		break;

	case   TERM_APP_SERVICE_STATS:
			get_sensor_stats(&termmsg->u.stats);
			hslog_debug("stats %x\n",termmsg->u.stats.pkts_recv);
			termmsg->opcode  = TERM_APP_OK;
			bytes = term_udp_send(termmsg, sizeof(*termmsg));
			if (bytes < 0){
				hslog_error("Service stats failed to tx:");
			}
		break;

	default:
		hslog_error("Unknown msg %d\n", termmsg->opcode);
	}
}


void* service_control(void * dummy)
{
	    int rc,rval;
	    int nfds = 1;
	    int nr_msgs = 0;
	    struct pollfd fds[1];
	    term_app_control_msg_t termmsg;

	    for  (;;) {
		nfds = 1;
		fds[0].fd = listensock;
		fds[0].events = POLLIN;
		fds[0].revents = 0;

		rc = poll(fds, nfds, -1);
		if (rc == 0){
			// timeout.
			printf("%s %d\n",__func__,__LINE__);
			continue;
		}
		if (rc < 0){
			hslog_error("Polling error %s\n",strerror(errno));
			continue;
		}

		if (fds[0].revents & POLLIN) {
			bzero(&termmsg, sizeof(termmsg));
			rval = read(listensock, &termmsg, sizeof(termmsg));
			if (rval <= 0) {
				hslog_error("reading stream message");
				continue;
			}
			process_term_app_msg(&termmsg);	
		}
	    }
	    close(listensock);
}




int start_service_check(char *sockname)
{
    struct sockaddr_in     addr;
    pthread_t tid;

    hslog_error("Starting term app control service\n");
    listensock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (listensock < 0) {
        hslog_error("opening stream socket");
        return -1;
    }

   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY; 
   addr.sin_port = htons(TERM_APP_HS_UDP_SRV_TX_PORT);

   if (bind(listensock, (struct sockaddr *) &addr, sizeof(addr))) {
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

