#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "hs_udp.h"
#include "hslog/hslog.h"


static int bind_server_socket(UDP_socket_t *sock, uint16_t port)
{
    int res = -1;
    char *ip = NULL;
    int flags = 0;
    struct sockaddr_in serverAddr;
    /* 
     * bind: associate the parent socket with a port 
     */
    bzero((char *)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons((unsigned short)port);
    res = bind(sock->sockFD, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (res < 0){
	hslog_error("Bind Failed. Error - %s\n", strerror(errno));
	return res;
    }
    // set the socket to non blocking.
    flags = fcntl(sock->sockFD, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sock->sockFD, F_SETFL, flags);

    return 0;
}

int create_server_socket(UDP_socket_t *sock, uint16_t port)
{
    sock->sockFD = socket(AF_INET, SOCK_DGRAM, 0);
    sock->port = port;

    bzero((char *)&sock->addr, sizeof(sock->addr));
    sock->addrLen = sizeof(sock->addr);

    if(sock->sockFD < 0){
        hslog_error("Failed to create socket. Error - %s\n", strerror(errno));
        return sock->sockFD;
    }

    sock->sockOption = 1;
    setsockopt(sock->sockFD, SOL_SOCKET, SO_REUSEADDR,
	   (const void *)&sock->sockOption, sizeof(int));

    return bind_server_socket(sock, port);
}

static int bind_client_address(UDP_socket_t *sock, const char *ipAddr, uint16_t port)
{
    int res = -1;
    char *ip = NULL;
    int flags = 0;
    /* 
	 * bind: associate the parent socket with a port 
	 */
    bzero((char *)&sock->addr, sizeof(sock->addr));
	sock->addr.sin_family = AF_INET;
    sock->addrLen = sizeof(sock->addr);
    sock->addr.sin_addr.s_addr = INADDR_ANY;

   /* res = inet_pton(sock->addr.sin_family, ipAddr, &sock->addr);
    if(res < 0){
        hslog_error("Failed to convert IP address to network format. Error - %s.\n", strerror(errno));
        return -1;
    }*/

	sock->addr.sin_port = htons((unsigned short)port);

    // set the socket to non blocking.
    flags = fcntl(sock->sockFD, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sock->sockFD, F_SETFL, flags);

    return 0;
}

int create_client_socket(UDP_socket_t *sock, const char *ipAddr, uint16_t port)
{
    sock->sockFD = socket(AF_INET, SOCK_DGRAM, 0);
    sock->port = port;

    if(sock->sockFD < 0)
        return sock->sockFD;

    sock->sockOption = 1;
	setsockopt(sock->sockFD, SOL_SOCKET, SO_REUSEADDR,
		   (const void *)&sock->sockOption, sizeof(int));

    return bind_client_address(sock, ipAddr, port);
}

int udp_recv(UDP_socket_t *sock, uint8_t *data, size_t dataLen)
{
    return recvfrom(sock->sockFD, data, dataLen, 0, (struct sockaddr *)(&sock->addr), &sock->addrLen);
}

int udp_send(UDP_socket_t *sock, uint8_t *data, size_t dataLen)
{
    return sendto(sock->sockFD, data, dataLen, 0, (struct sockaddr *)(&sock->addr), sock->addrLen);
}
