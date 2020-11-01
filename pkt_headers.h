#ifndef __PKT_HEADERS__
#define __PKT_HEADERS__

#define MAX_MSG_SIZE        60 // Data size between Fusion and UDP service, packet will have 200 bytes data + header

#define MSG_CODE_REG_DATA		0x1
#define MSG_CODE_HANDSHAKE_REQUEST	0x2
#define MSG_CODE_HANDSHAKE_REPLY	0x3
#define MSG_CODE_REQUEST_FRAG		0x4
#define MSG_CODE_XX3			0x5
#define MSG_CODE_XX2			0x6
#define MSG_CODE_REQ_VERSION		0x7

typedef struct   __attribute__ ((packed))  Msg_Header  {
    uint16_t     msgCode:3;
    uint16_t     rudp:1;
    uint16_t     frgmntIdx:5; 
    uint16_t     frgmnts:5;
    uint16_t	 seqNum:2; // Combined with frgmntIdx the pkt id is within 2^7

    uint8_t      size;      
} Msg_Header_t;

/*
 The old header is:
typedef struct   __attribute__ ((packed))  Msg_Header  {
    // Bit 0, if set indicates that this payload is a fragment of bigger message.
    // Bit 1-7, indicate the fragment index supporting a maximum of 127 fragments.
    uint8_t     frgmntByte;
    uint8_t      size;
    uint16_t     port;
}Msg_Header_t;

Therefore, as it is possible that a hibridiant network would consist of several Terminals generations, 
we mark this generation as gen 2. A terminal's version is defined in the NMS.
*/

/*
 * Handshake 
*/
typedef struct   __attribute__ ((packed))  rUdp_Handshake {
    uint8_t     msgCode:3; 
    uint8_t     pad:5; 
    uint16_t 	transmition_timeout_secs;
    uint16_t	retransmission_timeout_secs;
    uint16_t    iot_port;
} rUdp_Handshake_t; 

typedef struct   __attribute__ ((packed))  rUdpFragmntReq  {
    uint8_t      msgCode:3; 
    uint8_t      pad:5; 
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
