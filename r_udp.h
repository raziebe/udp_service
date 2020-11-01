#ifndef __R_UDP_H__
#define __R_UDP_H__

#define     R_UDP_TIMEOUT	20
#define     R_UDP_TIMEOUT_MS   (1000 * R_UDP_TIMEOUT)
#define     R_UDP_PORT_TERM	9999
#define     R_UDP_PORT_HUB	9998

struct HS_udp_socket;
struct Msg_Buf;

int   handshake(struct HS_udp_socket	*sockTx);
void* r_udp_routine(void *arg);
void  free_history(void *info);
void  hist_push_frag(struct Msg_Buf* dataPayLoad);
void handle_lost_request(rUdpFragmntReq_t *req,UDP_socket_t * );
struct Msg_Buf*  hist_find_frag(uint8_t FragLost, uint8_t seqNum);

#endif
