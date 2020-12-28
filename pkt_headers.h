#ifndef __PKT_HEADERS__
#define __PKT_HEADERS__

#define MAX_MSG_SIZE        60 // Data size between Fusion and UDP service, packet will have 200 bytes data + header

#define MSG_CODE_REG_DATA		0x0
#define MSG_CODE_REPLY_FRAG		0x1
#define MSG_CODE_REQUEST_FRAG		0x2

#define SEQ_MASK			0b1111

typedef struct   __attribute__ ((packed))  Msg_Header  {
    uint16_t     msgCode:2;
    uint16_t     frgmntIdx:5; 
    uint16_t     frgmnts:5;
    uint16_t	 seqNum:4; // Combined with frgmntIdx the pkt id is within 2^7

    uint8_t      size;      
} Msg_Header_t;

/*
 * Handshake 
*/
typedef struct   __attribute__ ((packed))  rUdp_Handshake {
    uint8_t     msgCode:2; 
    uint8_t     pad:6; 
    uint16_t 	transmition_timeout_secs;
    uint16_t	retransmission_timeout_secs;
    uint16_t    iot_port;
} rUdp_Handshake_t; 

typedef struct   __attribute__ ((packed))  rUdpFragmntReq  {
    uint8_t      msgCode:2; 
    uint8_t      pad:6; 
    uint8_t      fragmntLost;
    uint8_t      seqNum;
} rUdpFragmntReq_t; 


typedef struct   __attribute__ ((packed))   Msg_Buf
{
    Msg_Header_t    msgHeader;
    uint8_t         data[MAX_MSG_SIZE];
    uint8_t         opCode;
    uint16_t        termId;
    uint8_t         hiSkyId[6];
    uint16_t        msgLen;
    uint8_t	    seqNum;
    struct 	    timeval  time;
    struct  Msg_Buf *next;
}Msg_Buf_t;

typedef struct HUB_Message
{
    uint16_t    size;
    uint8_t     data[MAX_MSG_SIZE];
}HUB_Message_t;

#endif
