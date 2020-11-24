#ifndef __HS_UDP_SERVICE_H
#define __HS_UDP_SERVICE_H

#include "hs_udp.h"
#include "hs_fifo.h"
#include "Terminals_Handler.h"

#define MAX_BUFFER_ELEMS    64
#define MAX_MSG_SIZE        60 // Data size between Fusion and UDP service, packet will have 200 bytes data + header

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
}Fifo_Cluster_t;

typedef struct Thread_Info
{
    UDP_socket_t            *sockPtr;
    Fifo_Cluster_t          *fifoClusterPtr;
    TerminalCluster_t       *terminalClusterPtr;
}Thread_Info_t;

typedef struct   __attribute__ ((packed))  Msg_Header  {
    // Bit 0, if set indicates that this payload is a fragment of bigger message.
    // Bit 1-7, indicate the fragment index supporting a maximum of 127 fragments.
    uint8_t     frgmntByte; 
    uint8_t      size;      
    uint16_t     port;
}Msg_Header_t;

typedef struct   __attribute__ ((packed))   Msg_Buf
{
    Msg_Header_t    msgHeader;
    uint8_t         data[MAX_MSG_SIZE];
    uint16_t    seqNum;
    uint8_t     opCode;
    uint16_t    termId;
    uint8_t     hiSkyId[6];
    uint16_t    msgLen;
}Msg_Buf_t;

typedef struct HUB_Message
{
    uint16_t    size;
    uint8_t     data[MAX_MSG_SIZE];
}HUB_Message_t;


#endif //__HS_UDP_SERVICE_H
