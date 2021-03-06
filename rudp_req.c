// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <pthread.h>
#include <sys/time.h>


#include "hs_udp_fragments.h"
#include "hslog/hslog.h"
#include "r_udp.h"

#define TERM_SERVICE_USER_RELIABLE_UDP 5
int8_t last_pkt_seq = -1;
static 	UDP_socket_t    *fusionSock;

void term_set_cur_pkt_seq(int8_t seqNum)
{
	last_pkt_seq = seqNum;

}

int8_t term_get_cur_pkt_seq(void)
{

	return last_pkt_seq;
}


/* dump timdeout fragments */
void* r_udp_req_routine(void *args)
{
	extern UDP_Fragments_Table_t fragmentsTable;
	struct timeval curTime;
	long tm;
    	Thread_Info_t   *infoPtr  = (Thread_Info_t *)args;
    	fusionSock     = infoPtr->sockPtr2;

	while (1){

		int i = 0;
		int j = 0;
		gettimeofday(&curTime, NULL);

		for (;i< FRAGMENTS_TABLE_COUNT; i++) {
			for (;j < MAX_PKTS; j++){
				UDP_Fragment_t *frag = &fragmentsTable.fragments[i][j];
				if (!frag->used)
					continue;
				tm = curTime.tv_sec - frag->lastTime.tv_sec ;
				if (tm > R_UDP_TIMEOUT ){
					frag->used = false;
					printf("%s Dumping timed out(%ld) fragment Seq=%d\n",
							__func__,
							tm, frag->pktSeqNum);
				}
			}
		}
		sleep(1);
	}
	return NULL;
}

int hub_send_req(rUdpFragmntReq_t* req)
{
    if (udp_send(fusionSock,(void *) req, sizeof(*req)) <0){
	printf("Failed to send to fusion\n");
    }
}

void rrudp_ask_retransmit(uint8_t frgmntIdx, Msg_Header_t* msgHeader)
{
    rUdpFragmntReq_t req;


    // construct a request

    req.msgCode =  MSG_CODE_REQUEST_FRAG;
    req.fragmntLost = frgmntIdx;
    req.seqNum	= msgHeader->seqNum;

    printf("%s seqNum=%d Idx=%d\n",__func__,msgHeader->seqNum ,req.fragmntLost);
    if (!hub_send_req(&req)){
        printf("%s: r-udp rtx error\n", __func__);
   }
}

void rrudp_complete_last_pkt(UDP_Fragments_Table_t *fragmentsTable,uint8_t pkt_seq)
{
    rUdpFragmntReq_t req;

    UDP_Fragment_t* frag;

    frag = find_terminal(fragmentsTable, TERMINAL_ID, pkt_seq);
    if (!frag){
    	printf("%s did not find pkt seq=%d\n", __func__,pkt_seq);
    	return;
    }

    req.msgCode =  MSG_CODE_REQUEST_FRAG;
    req.seqNum	= pkt_seq;

    for (int i = 0 ; i < frag->frgmnts; i++){

    	if (!(frag->frags_mask & (1 << i))) {
    		req.fragmntLost = i;
    	    	
		printf("%s ask rtx:  seq=%d frag=%d\n",
    	    		__func__,  pkt_seq, i);
        	if (!hub_send_req(&req)){
        		printf("%s: rr-udp rtx error\n", __func__);
        	}
    	}
    }
}

int retransmit_msg(Msg_Buf_t *msg)
{
	return msg->msgHeader.msgCode == MSG_CODE_REQUEST_FRAG;
}

/*
 *	Returns 0 when the entire message is colated
*/
int defragment(UDP_Fragments_Table_t *fragmentsTable, Msg_Buf_t *msg, UDP_Fragment_t **outFragment)
{
    uint8_t frgmntIdx = 0;
    UDP_Fragment_t* handle = NULL;

    printf("%s: E FragIdx:%d, Frags:%d Seq=%d msgCode=%x\n",
                __func__, 
		msg->msgHeader.frgmntIdx, 
		msg->msgHeader.frgmnts,
		msg->msgHeader.seqNum,
		msg->msgHeader.msgCode);

    if(msg->msgHeader.frgmntIdx < msg->msgHeader.frgmnts) {
        // Fragmented packet, needs defragmentation.
        frgmntIdx = msg->msgHeader.frgmntIdx;

        handle = find_terminal(fragmentsTable, TERMINAL_ID, msg->msgHeader.seqNum);
        if (handle && !retransmit_msg(msg))
        	term_set_cur_pkt_seq(msg->msgHeader.seqNum);
        if(!handle){
            // Does not exist, create new one.
            handle = get_freeFragment(fragmentsTable, TERMINAL_ID, msg->msgHeader.seqNum);
            // Check if free slots available.
            if(!handle){
                // Cannot process msg, drop the packet
                printf("%s No free frgament slots available for defragmentation.\n",__func__);
                memset(handle, 0, sizeof(UDP_Fragment_t));
                *outFragment = NULL;
                return -1;
            }
        }


        if (term_get_cur_pkt_seq() != msg->msgHeader.seqNum) {
        	// A new packet arrived while the last one is not complete
        	rrudp_complete_last_pkt(fragmentsTable, term_get_cur_pkt_seq() );
        }

        handle->pktSeqNum = msg->msgHeader.seqNum;
        frgmntIdx = msg->msgHeader.frgmntIdx; // Frag Index starts with a zero

        if (frgmntIdx < msg->msgHeader.frgmnts){

           uint8_t *data;

       	   if (msg->msgHeader.frgmnts) {
        		// we have a fragment
        		data  = &handle->data[frgmntIdx * MAX_MSG_SIZE];
        	}else {
        		// no fragment.
        		data  = &handle->data[0];
            }
            memcpy(data, msg->data, msg->msgLen);
       	    handle->size += msg->msgLen;
            handle->accumulatedFrgmnts++;
            handle->pktSeqNum = msg->msgHeader.seqNum;
            handle->frgmnts = msg->msgHeader.frgmnts;
            handle->frags_mask |= (1<< msg->msgHeader.frgmntIdx); // history
            if (!retransmit_msg(msg)) {
            	term_set_cur_pkt_seq(msg->msgHeader.seqNum);
	    }
            gettimeofday(&handle->lastTime, NULL);
            printf("%s: After: handle->frgmntIdx=%d TotSise:%d accumulated frmgnts:%d pktSeq=%d RTXpkt=%x curSeq=%x\n",
              		  __func__,
			handle->frgmntIdx,
			handle->size, 
			handle->accumulatedFrgmnts, 
			handle->pktSeqNum,
			retransmit_msg(msg), 
			term_get_cur_pkt_seq() );

            if (msg->msgHeader.frgmntIdx > (handle->frgmntIdx+1)){
            		// Naive : we jumped over a fragment,  a retransmit
             		rrudp_ask_retransmit(msg->msgHeader.frgmntIdx -1 , &msg->msgHeader);
            }

            // cache last fragment
            handle->frgmntIdx = msg->msgHeader.frgmntIdx;

            if (msg->msgHeader.frgmnts <= handle->accumulatedFrgmnts) {
                	// Last fragment, return the defragmented data.
                	*outFragment = handle;
                	printf("%s Seq=%d frgmnts=%d TotSize %d accumFrags %d completed\n",
                			__func__, 
					msg->msgHeader.seqNum,
					msg->msgHeader.frgmnts,		
					handle->size,
					handle->accumulatedFrgmnts);
                	return 0;
            }

            if (msg->msgHeader.frgmntIdx < msg->msgHeader.frgmnts) {
               		 // More fragments to come, don't return the defragmented data yet.
                	 *outFragment = NULL;
               		 return -1;
           	}
        }

        printf("%s clearing seq %d\n",__func__, handle->pktSeqNum);
        memset(handle, 0, sizeof(UDP_Fragment_t));
        *outFragment = NULL;
        return -1;

    }
    else{
        // No defragmentation required, it is a single packet.
        *outFragment = NULL;
    }

    return 0;
}


