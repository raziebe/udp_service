#ifndef __HS_UDP_H
#define __HS_UDP_H

/* ----------------------------
 *           includes
 * ----------------------------
 */
#include <stdint.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <netinet/ip.h>

#define NET_PACKET_SIZE     1500
#define TERMINAL_ID	0

typedef struct HS_udp_socket
{
    uint16_t port;
    int sockFD;
    struct sockaddr_in addr;
    socklen_t addrLen;
    char ipAddr[16];
    int sockOption;
}UDP_socket_t;

typedef struct UDP_Payload
{
    uint16_t size;
    uint8_t data[NET_PACKET_SIZE];
}UDP_Payload_t;

int create_server_socket(UDP_socket_t *sock, uint16_t port);
int create_client_socket(UDP_socket_t *sock, const char *ipAddr, uint16_t port);
int udp_send(UDP_socket_t *sock, uint8_t *data, size_t dataLen);
int udp_recv(UDP_socket_t *sock, uint8_t *data, size_t dataLen);
void close_socket(UDP_socket_t *sock);

#endif // __HS_UDP_H
