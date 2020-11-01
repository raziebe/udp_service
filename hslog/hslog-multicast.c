  
 
 #include <stdio.h>          /* printf(), snprintf() */
 #include <stdlib.h>         /* strtol(), exit() */
 #include <sys/types.h>
 #include <sys/socket.h>     /* socket(), setsockopt(), bind(), 
                                      recvfrom(), sendto() */
 #include <errno.h>          /* perror() */
 #include <netinet/in.h>     /* IPPROTO_IP, sockaddr_in, htons(), 
                                      htonl() */
 #include <arpa/inet.h>      /* inet_addr() */
 #include <unistd.h>         /* fork(), sleep() */
 #include <sys/utsname.h>    /* uname() */
 #include <string.h>         /* memset() */
 #include <stdbool.h>
 #include <pthread.h>
 #include "hslog.h"
 #include "hslog_icd.h"
 
 uint8_t curModule = 0;
 #define MAXLEN 1024
 #define DELAY 2
 #define TTL 1

 typedef struct Multicast_logger
 {
     int send_s, recv_s;     /* Sockets for sending and receiving. */
     int sock_unicast;
     u_char ttl;
     struct sockaddr_in mcast_group;
     struct sockaddr_in	 servaddr;
     struct ip_mreq mreq;
     struct utsname name;
     bool networkLoggingEnabled;
     bool unicastLoggingEnabled;
     int seq;
 }Multicast_logger_t;

 static Multicast_logger_t networkLogger;

 void* process_network_logging_commands(void *args);
 
int init_network_log(char *multicast_groupIp, char *ttl, short port)
{
    u_char no = 0;
    u_int yes = 1;      /* Used with SO_REUSEADDR. 
                            In Linux both u_int */
                        /* and u_char are valid. */


    memset(&networkLogger.mcast_group, 0, sizeof(networkLogger.mcast_group));
    networkLogger.mcast_group.sin_family = AF_INET;
    networkLogger.mcast_group.sin_port = htons(port);
    networkLogger.mcast_group.sin_addr.s_addr = inet_addr(multicast_groupIp);

    if ( (networkLogger.send_s=socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror ("send socket");
        exit(1);
    }   

    /* If ttl supplied, set it */   
    if (ttl != NULL) {
        networkLogger.ttl = strtol(ttl, NULL, 0);
    } else {
        networkLogger.ttl = TTL;
    }

    if (setsockopt(networkLogger.send_s, IPPROTO_IP, IP_MULTICAST_TTL, &networkLogger.ttl,
            sizeof(networkLogger.ttl)) < 0) {
        perror ("ttl setsockopt");
        exit(1);
    }

    /* Disable Loop-back */
    if (setsockopt(networkLogger.send_s, IPPROTO_IP, IP_MULTICAST_LOOP, &no,
        sizeof(no)) < 0) {
        perror ("loop setsockopt");
        exit(1);
    }

    if ( (networkLogger.recv_s=socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror ("recv socket");
        exit(1);
    }

    if (setsockopt(networkLogger.recv_s, SOL_SOCKET, SO_REUSEADDR, &yes, 
                sizeof(yes)) < 0) {
        perror("reuseaddr setsockopt");
        exit(1);
    }

    if (bind(networkLogger.recv_s, (struct sockaddr*)&networkLogger.mcast_group, 
                sizeof(networkLogger.mcast_group)) < 0) {
        perror ("bind");
        exit(1);
    }

    /* Tell the kernel we want to join that multicast group. */
    networkLogger.mreq.imr_multiaddr = networkLogger.mcast_group.sin_addr;
    networkLogger.mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(networkLogger.recv_s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &networkLogger.mreq, 
                sizeof(networkLogger.mreq)) < 0) {
        perror ("add_membership setsockopt");
        exit(1);
    }

    // prepare a unicast socket
    if ((networkLogger.sock_unicast = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror ("send socket");
        exit(1);
    }   

    memset(&networkLogger.servaddr, 0, sizeof(networkLogger.servaddr));
   
    networkLogger.servaddr.sin_family = AF_INET; 
    networkLogger.servaddr.sin_port = htons(7777); 
    networkLogger.servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (uname(&networkLogger.name) < 0) {
        perror ("uname");
        exit(1);
    }

    // Process network logging command now.
    pthread_t networkLoggingThreadId;
    pthread_attr_t attr;
    int result = 0;
    result = pthread_attr_init(&attr);
    if (result != 0){
        printf("Failed to init pthread attribute entity.\n");
        return result;    
    }

    // set detached state for the thread
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    result = pthread_create(&networkLoggingThreadId, &attr, &process_network_logging_commands, NULL);
    if (result < 0){
        printf("Failed to create iot receive thread. Error - %s.\n", strerror(result));
        return result;
    }

    return 0;
}

bool is_unicast_logging_enabled(void)
{
    return networkLogger.unicastLoggingEnabled;
}

void enable_unicast_logging(void)
{
   networkLogger.unicastLoggingEnabled = true;
}

void disable_unicast_logging(void)
{
   networkLogger.unicastLoggingEnabled = false;
}

void enable_network_logging(void)
{
    networkLogger.networkLoggingEnabled = true;
}

void disable_network_logging(void)
{
    networkLogger.networkLoggingEnabled = false;
}

bool is_network_logging_enabled(void)
{
    return networkLogger.networkLoggingEnabled;
}

void* process_network_logging_commands(void *args)
{
    int n;
    int len;
    struct sockaddr_in from;
    char message[MAXLEN];

    // Check for START_LOGGING or STOP_LOGGING command
    for (;;) {
        len=sizeof(from);
        if ( (n=recvfrom(networkLogger.recv_s, message, MAXLEN, 0, 
                        (struct sockaddr*)&from, (unsigned int *)&len)) < 0) {
            perror ("recv");
            exit(1);
        }
        message[n] = '\0'; /* null-terminate string */
        if(!strcmp(message, "START_LOGGING")){
            enable_network_logging();
        }
        else if(!strcmp(message, "STOP_LOGGING")){
            disable_network_logging();
        }
    }
}

void hslog_to_network(const char* pStr, const HSlogDate *pDate, int nType, char *module)
{
//    char sDate[64];
    char msg[1400]; // Leave 100 bytes space for IP packet fields. Size has been chosen to avoid fragmentation.
    struct icd_hslog *icd = (struct icd_hslog *)&msg;
    char logSeverity[][32] = {
        "LIVE",
        "INFO",
        "WARNING",
        "DEBUG",
        "ERROR",
        "FATAL",
        "PANIC"
    };

    char *txt = (char*)&icd->text[0];
//    snprintf(sDate, sizeof(sDate), "%02d.%02d.%02d-%02d:%02d:%02d.%02d", 
  //                  pDate->year, pDate->mon, pDate->day, pDate->hour, 
    //                pDate->min, pDate->sec, pDate->usec);

    memset(msg, 0, sizeof(msg));
 
    snprintf(txt, sizeof(msg) - 1, "<%s> [%s] - %s", logSeverity[nType], module, pStr);
    icd->hdr.seq = ++networkLogger.seq;
    icd->hdr.module = UDP_SRV;
    icd->hdr.loglevel = nType;
    int msglen = strlen(txt);
    icd->hdr.len = msglen + sizeof(icd->hdr);
    // send it over the network.

    if (is_network_logging_enabled()) {
    		
	    if (sendto(networkLogger.send_s, icd->text, msglen, 0,
			(struct sockaddr*)&networkLogger.mcast_group, 
			sizeof(networkLogger.mcast_group)) < (msglen - 1)) {
		printf("mcast: sendto failed - %s\n", strerror(errno));
	    }
    }
 
    if (is_unicast_logging_enabled()) {

	    if (sendto(networkLogger.sock_unicast, icd, icd->hdr.len, 0,
			(struct sockaddr*)&networkLogger.servaddr, 
			sizeof(networkLogger.servaddr)) < icd->hdr.len) {
		printf("unicast: sendto failed - %s\n", strerror(errno));
	    }
    }
	 
}
