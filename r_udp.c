
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <sys/epoll.h>
#include <sys/time.h>

#include "hs_config_file_reader.h"
#include "hs_udp_service.h"
#include "hs_debug.h"
#include "hs_fifo.h"
#include "hs_buffer_pool.h"
#include "hs_udp_fragments.h"
#include "sensor_stats.h"
#include "hslog/hslog.h"
#include "r_udp.h"

UDP_socket_t	*sockTx = NULL;

/*
 * Timeout after N seconds
 * if exited from Poll before timeout,
 * parse the request, if request may be served, serve it and free the
 * history. else free theh history
*/
void *r_udp_routine(void *args)
{
    Thread_Info_t       *infoPtr = (Thread_Info_t *)args;
    sockTx = infoPtr->sockPtr2; 

    hslog_info("R-UDP on\n");
    while (1) {
	free_history(infoPtr);
	sleep(1);
    }
}



void	handle_lost_request(rUdpFragmntReq_t *req, UDP_socket_t	*txSocket) 
{
	Msg_Buf_t* pkt;

	hslog_info("Got a loss request for frag=%d seq=%d searching...\n",
               			req->fragmntLost, req->seqNum);
	pkt = hist_find_frag(req->fragmntLost, req->seqNum);
	if (pkt){
		pkt->msgHeader.msgCode = MSG_CODE_REQUEST_FRAG;
		int res = udp_send(sockTx, (uint8_t *)pkt, (pkt->msgLen + sizeof(Msg_Header_t)));
		if((res < 0) && (errno != EAGAIN)){
				hslog_error("Failed to send data to "
				"fusion. Error - %s (%d). Retrying.\n", strerror(errno), errno);
			return;
		}
		hslog_info("Successfully replied frag=%d seq=%d len=%d:%d\n",
               			req->fragmntLost, req->seqNum ,pkt->msgLen, res );
		return;
	}
	hslog_info("Did not find frag=%d seq=%d\n",
               			req->fragmntLost, req->seqNum);
}


int handshake(UDP_socket_t *sockTx)
{
	rUdp_Handshake_t req;
	int dataLen = sizeof(req);

	req.msgCode = MSG_CODE_HANDSHAKE_REQUEST;
	int res = sendto(sockTx->sockFD, &req, dataLen, 0, 
			(struct sockaddr *)(&sockTx->addr), sockTx->addrLen);

	if (res < 0){
		hslog_error("%s %d\n",__func__,__LINE__);
		return -1;
	}
	return 0;
}


/***********************************************************************/
static Msg_Buf_t* ListHead = NULL; 
static Msg_Buf_t* ListTail = NULL;
static pthread_mutex_t sync_hist = PTHREAD_MUTEX_INITIALIZER; 

Msg_Buf_t* hist_find_frag(uint8_t fragLost, uint8_t seqNum)
{
	Msg_Buf_t *n;

	pthread_mutex_lock(&sync_hist);
	for (n = ListHead; n != NULL ;n = n->next){
		if (n->msgHeader.frgmntIdx == fragLost && n->msgHeader.seqNum == seqNum){
			pthread_mutex_unlock(&sync_hist);
			n->time.tv_sec += 1; // linger a bit
			return n;
		}
	}
	pthread_mutex_unlock(&sync_hist);
	return NULL;
}

void hist_pop_frag(Fifo_Cluster_t *fifoClusterPtr)
{
	Msg_Buf_t *head = ListHead;
	ListHead = head->next;

	if (ListHead == NULL){
		ListTail = NULL;
	}
	
	// put back the fragment into the free list
	Fifo_put(fifoClusterPtr->IotThrInFifo, (void **)&head);
}

void hist_push_frag(Msg_Buf_t* dataPayLoad)
{
	struct timeval time;

	gettimeofday(&time, NULL);

	pthread_mutex_lock(&sync_hist);

	time.tv_sec += R_UDP_TIMEOUT; // set the timeout
	dataPayLoad->time = time;
	if (ListHead == NULL) {
		ListHead = dataPayLoad;
	} else{
		ListTail->next = dataPayLoad;
	}
	ListTail = dataPayLoad;
	dataPayLoad->next = NULL;

	pthread_mutex_unlock(&sync_hist);
}

Msg_Buf_t* hist_peek_head(void)
{
	if (ListHead == NULL) {
		return NULL;
	}

	struct timeval time;
	gettimeofday(&time, NULL);
	if (ListHead->time.tv_sec <= time.tv_sec)
		return ListHead;
	return NULL;
}

/*
 * walk on all fragments that must be released
 * and free them
*/
void free_history(void *info)
{
	Msg_Buf_t* h;
	Thread_Info_t *infoPtr = (Thread_Info_t *)info;
        Fifo_Cluster_t *fifoClusterPtr = infoPtr->fifoClusterPtr;

	pthread_mutex_lock(&sync_hist);
	while ( (h = hist_peek_head()) != NULL){
		hist_pop_frag(fifoClusterPtr);
	}	
	pthread_mutex_unlock(&sync_hist);
}


