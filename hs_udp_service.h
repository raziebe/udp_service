#ifndef __HS_UDP_SERVICE_H
#define __HS_UDP_SERVICE_H

#include <sys/time.h>
#include "hs_udp.h"
#include "hs_fifo.h"
#include "Terminals_Handler.h"
#include "pkt_headers.h"

#define MAX_BUFFER_ELEMS    64
#define MAX_EVENTS          1

int start_udp_service(void);
void end_udp_service(uint32_t serviceId);
int get_fifo_avail(const char *,size_t *avail);


typedef struct Connect_Info
{
    int     IOTServicePort;
    int     fusionPort;
    char    IOTip[16];
    char    fusionIp[16];
}Connect_Info_t;

typedef struct Fifo_Cluster
{
    fifo_t  IotThrInFifo;
    fifo_t  IotThrOutFifo;
    fifo_t  fusionThrInFifo;
    fifo_t  fusionThrOutFifo;
    fifo_t  rUdpThrInFifo;
}Fifo_Cluster_t;

typedef struct Thread_Info
{
    UDP_socket_t            *sockPtr;
    UDP_socket_t            *sockPtr2;
    Fifo_Cluster_t          *fifoClusterPtr;
    TerminalCluster_t       *terminalClusterPtr;
}Thread_Info_t;


#endif //__HS_UDP_SERVICE_H
