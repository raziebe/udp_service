#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <limits.h>
#include <sys/epoll.h>

#include "hs_config_file_reader.h"
#include "hs_udp_service.h"
#include "hs_debug.h"
#include "hs_fifo.h"
#include "hs_buffer_pool.h"
#include "hs_udp_fragments.h"
#include "r_udp.h"
#include "sensor_stats.h"
#include "hslog/hslog.h"


const char *fifo_srv  = "/dev/hsfifo_1";
const char *fifo_core = "/dev/hsfifo_0";


uint16_t IOTPort = 0; //8080


static pthread_mutex_t IOTSocketMutex = PTHREAD_MUTEX_INITIALIZER;

void debug_buffer(char *msg, uint8_t *buffer, int bufLen)
{
    int index = 0;
    hslog_error("%s - ", msg);
    for(index = 0;index < bufLen; index++)
        hslog_info("0x%02x ", *(buffer+index));
}

void* iot_receive_routine(void *args)
{
    int                     res = 0;
    int                     waitFds = 0;
    int                     waitTime = 1000; // 10 milliseconds
    int                     epollFd = -1;
    int                     index = 0; 
    Msg_Buf_t               *dataPayLoad = NULL;
    UDP_Payload_t           rxPayLoad; // To store the received.
    Thread_Info_t           *infoPtr = (Thread_Info_t *)args;
    UDP_socket_t            *sock = infoPtr->sockPtr;
    Fifo_Cluster_t          *fifoClusterPtr = infoPtr->fifoClusterPtr;
    struct epoll_event      ev;
    struct epoll_event      events[MAX_EVENTS];
    uint8_t                 frgmntIdx = 0;
    uint8_t                 *dataPtr = NULL;
    extern int 		    service_disabled;
    uint8_t                 txSeqNum        = 0;

    // create the epoll instance
    epollFd = epoll_create1(0);
    if(epollFd ==-1){
        hslog_error("Failed to create epoll instance. Error - %s", strerror(errno));
        return NULL;
    }

    ev.events = EPOLLIN;
    ev.data.fd = sock->sockFD;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, sock->sockFD, &ev) == -1){
        hslog_error("Failed to add listening socket for read events. Error - %s", strerror(errno));
        return NULL;
    }

    while(1){
        // wait for the receive socket to be ready with data
        waitFds = epoll_wait(epollFd, events, MAX_EVENTS, waitTime);
        if(waitFds == -1){
            hslog_error("Listen on receive socket failed.Error - %s", strerror(errno));
            return NULL;
        }
	
	if (waitFds == 0){
            // Timeout continue wait for read. or service is disabled
	    usleep(10000);
            continue;
        }
        
        // Check if data available from iot device
        pthread_mutex_lock(&IOTSocketMutex);
        res = udp_recv(sock, rxPayLoad.data, sizeof(rxPayLoad.data));
        pthread_mutex_unlock(&IOTSocketMutex);
	if (service_disabled){
		hslog_error("Dumping msg\n");	
		continue;
        }

	int frgmnts = res/MAX_MSG_SIZE + 1;
	txSeqNum++;
        if(res > 0){
                // Received data
                // Push data to receive queue
                rxPayLoad.size = res;    

                // We have a valid terminal for this UDP packet.
                dataPtr = rxPayLoad.data;
                frgmntIdx = 0;
                while (res > 0){

                    Fifo_get(fifoClusterPtr->IotThrInFifo, (void **)&dataPayLoad);
		    dataPayLoad->seqNum = txSeqNum; 	
        	    dataPayLoad->msgHeader.frgmnts = frgmnts;
                    if(MAX_MSG_SIZE < res){
                        dataPayLoad->msgLen = MAX_MSG_SIZE;
                        res -= MAX_MSG_SIZE;
                    } else{
                        dataPayLoad->msgLen = res;
                        res = 0;
                    }
                    dataPayLoad->msgHeader.frgmntIdx = frgmntIdx & 0b11111;
                    frgmntIdx++;
		    // Pad out with zeroes
                    memset(dataPayLoad->data, 0x00, MAX_MSG_SIZE);
                    // Get fifo to read data from socket
                    memcpy(dataPayLoad->data, dataPtr, dataPayLoad->msgLen);
                    dataPtr += dataPayLoad->msgLen;
                    Fifo_put(fifoClusterPtr->IotThrOutFifo, (void **)&dataPayLoad);
		   
                }
		update_stats_pkts_recv(1);
		continue;
            }

            if((res < 0) && (errno != EAGAIN)){
                    // Failed to receive data
                    hslog_error("Failed to receive IOT data. Socket failure. Error - %s. Retrying.\n", strerror(errno));
		    update_stats_pkts_err(1);
            }
        
     }        
}

// ////////////////////////////////////////////////////////////////////////////////////////////////
// IMPORTANT NOTE for Terminal compilation:
// Make sure that iot_transmit_routine transmits to IOT device only after receiving data from
// IOT device, terminal doesn't know the IP address of IOT device. We can only know the
// IOT device socket address, after we receive data in recvfrom() and the sockaddr structure
// gets updated.
// ////////////////////////////////////////////////////////////////////////////////////////////////
void* iot_transmit_routine(void *args)
{
    int                 res = 0;
    int                 waitFds = 0;
    int                 waitTime = 1000; // 10 milliseconds
    int                 epollFd = -1;
    int                 dataSizeToSend = 0;
    Msg_Buf_t           *dataPayLoad = NULL;
    Thread_Info_t       *infoPtr = (Thread_Info_t *)args;
    UDP_socket_t        *sock = infoPtr->sockPtr;
    Fifo_Cluster_t      *fifoClusterPtr = infoPtr->fifoClusterPtr;
    UDP_Payload_t       payload;
    struct epoll_event  ev;
    struct epoll_event  events[MAX_EVENTS];
    uint8_t             frgmntIdx = 0;
    uint8_t             *dataPtr = NULL;
    int                 index = 0;
    extern UDP_Fragments_Table_t fragmentsTable;


    // create the epoll instance
    epollFd = epoll_create1(0);
    if(epollFd ==-1){
        hslog_error("Failed to create epoll instance. Error - %s", strerror(errno));
        return NULL;
    }

    ev.events = EPOLLOUT;
    ev.data.fd = sock->sockFD;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, sock->sockFD, &ev) == -1){
        hslog_error("Failed to add listening socket for read events. Error - %s", strerror(errno));
        return NULL;
    }

    while(1) {
        // wait for the receive socket to be ready with data
        waitFds = epoll_wait(epollFd, events, MAX_EVENTS, waitTime);
        if(waitFds == -1){
            hslog_error("Listen on transmit socket failed.Error - %s", strerror(errno));                
            break;
        }

        if(waitFds == 0){
            // Timeout continue wait for write.
     	   
            continue;
        } 

	// Check if data available in transmit queue
        Fifo_get(fifoClusterPtr->fusionThrOutFifo, (void **)&dataPayLoad);

        // Send the data to the terminal's destination;
        //////////////////////////////////////////////////////////////////////////
        // Change this to traverse the doubly linked list in deployment to get the
        // required terminal data.
        ////////////////////////////////////////////////////////////////////////////
        UDP_Fragment_t *msgHandle = NULL;
        res = defragment(&fragmentsTable, dataPayLoad, &msgHandle);
		
       if((res == 0)){
           if(msgHandle != NULL){
                    dataPtr = msgHandle->data;
                    dataSizeToSend = msgHandle->size;
             } else{
                    dataPtr = dataPayLoad->data;
                    dataSizeToSend = dataPayLoad->msgLen;
             }
             // Have data to send.
             res = udp_send(sock, dataPtr, dataSizeToSend);
             if((res < 0) && (errno != EAGAIN))
                    // Failed to send data.
                    hslog_error("Failed to send data to IOT device. Error - %s (%d). Retrying.\n", strerror(errno), errno);
                
                // Release the fragment
             release_fragment(msgHandle);
           }
	   update_stats_pkts_sent(1);
           Fifo_put(fifoClusterPtr->fusionThrInFifo, (void **)&dataPayLoad);
        }
}

void* fusion_transmit_routine(void *args)
{
    int                         res             = 0;
    int                         epollTxFd       = -1;
    int                         waitFds         = 0;
    int                         waitTime        = 1000; // 10 milliseconds
    Thread_Info_t               *infoPtr        = (Thread_Info_t *)args;
    UDP_socket_t                *sock           = infoPtr->sockPtr;
    Fifo_Cluster_t              *fifoClusterPtr = infoPtr->fifoClusterPtr;
    Msg_Buf_t                   *rxMsgBufPtr    = NULL;
    Msg_Buf_t                   rxMsgPayload;
    struct epoll_event          ev;
    struct epoll_event          events[MAX_EVENTS];

    // create the epoll instance
    epollTxFd = epoll_create1(0);
    if(epollTxFd ==-1){
        hslog_error("Failed to create epoll instance. Error - %s", strerror(errno));
        return NULL;
    }

    ev.events = EPOLLOUT;
    ev.data.fd = sock->sockFD;
    if(epoll_ctl(epollTxFd, EPOLL_CTL_ADD, sock->sockFD, &ev) == -1){
        hslog_error("Failed to add listening socket for read events. Error - %s", strerror(errno));
        return NULL;
    } 

    while(1) {        
        // now wait on read ready
        waitFds = epoll_wait(epollTxFd, events, MAX_EVENTS, waitTime);
        if(waitFds == -1){
            hslog_error("Listen on receive socket failed.Error - %s", strerror(errno));
            return NULL;
        }

	if(waitFds == 0){
            continue;
        }

	// Check if data available in fusion transmit queue
	Fifo_get(fifoClusterPtr->IotThrOutFifo, (void **)&rxMsgBufPtr);

	// Add the running sequence number
	rxMsgBufPtr->msgHeader.seqNum =  rxMsgBufPtr->seqNum & 0b11;
	rxMsgBufPtr->msgHeader.size =  (uint8_t)rxMsgBufPtr->msgLen;
	rxMsgBufPtr->msgHeader.msgCode = MSG_CODE_REG_DATA;

	printf("Send txSeq %d frgmntIdx=%d/%d\n", 
		rxMsgBufPtr->msgHeader.seqNum,
		rxMsgBufPtr->msgHeader.frgmntIdx , rxMsgBufPtr->msgHeader.frgmnts);
 	
	res = udp_send(sock, (uint8_t *)rxMsgBufPtr, (rxMsgBufPtr->msgLen + sizeof(Msg_Header_t)));
	if((res < 0) && (errno != EAGAIN)){
		hslog_error("Failed to send data to "
			"fusion. Error - %s (%d). Retrying.\n", strerror(errno), errno);
	}
	hist_push_frag(rxMsgBufPtr);
      }
    
}

void* fusion_receive_routine(void *args)
{
    int                         res             = 0;
    int                         epollRxFd       = -1;
    int                         waitFds         = 0;
    int                         waitTime        = 1000; 
    uint32_t                    rxSeqNum        = 0;
    uint32_t                    txSeqNum        = 0;
    Thread_Info_t               *infoPtr        = (Thread_Info_t *)args;
    UDP_socket_t                *sock           = infoPtr->sockPtr;
    Fifo_Cluster_t              *fifoClusterPtr = infoPtr->fifoClusterPtr;
    Msg_Buf_t                   *rxMsgBufPtr    = NULL;
    Msg_Buf_t                   rxMsgPayload;
    struct epoll_event          ev;
    struct epoll_event          events[MAX_EVENTS];

    epollRxFd = epoll_create1(0);
    if(epollRxFd ==-1){
        hslog_error("Failed to create epoll instance. Error - %s", strerror(errno));
        return NULL;
    }

    ev.events = EPOLLIN;
    ev.data.fd = sock->sockFD;
    if(epoll_ctl(epollRxFd, EPOLL_CTL_ADD, sock->sockFD, &ev) == -1){
        hslog_error("Failed to add listening socket for read events. Error - %s", strerror(errno));
        return NULL;
    }

    while(1) {    
        // now wait on read ready
        waitFds = epoll_wait(epollRxFd, events, MAX_EVENTS, waitTime);
        if(waitFds == -1){
            hslog_error("Listen on receive socket failed.Error - %s", strerror(errno));
            return NULL;
        }

        if(waitFds == 0){
            continue;
        }
     
        // check if any data available from Fusion
        res = udp_recv(sock, (uint8_t *)&rxMsgPayload, sizeof(Msg_Buf_t));
	
        if(res > 0){
        
		if (rxMsgPayload.msgHeader.msgCode == MSG_CODE_REQUEST_FRAG){
			handle_lost_request((void *)&rxMsgPayload, NULL);
			printf("Got Request fragment\n");
			continue;
		}
	        // Received the data
                rxSeqNum = rxMsgPayload.seqNum;

                Fifo_get(fifoClusterPtr->fusionThrInFifo, (void **)&rxMsgBufPtr);
		rxMsgPayload.msgLen = res - sizeof(Msg_Header_t);
                memcpy(rxMsgBufPtr, &rxMsgPayload, sizeof(Msg_Buf_t));
/*
 * Simulate packet loss: hub to terminal
*/
		static int pkt_drop_count = 0;
		if (pkt_drop_count++ % 13){
                	Fifo_put(fifoClusterPtr->fusionThrOutFifo, (void **)&rxMsgBufPtr);
		}else{
			printf("Droppping packet\n");
		}
		continue;
        }
        if((res < 0) && (errno != EAGAIN)){
                hslog_error("Failed to recieve data from fusion. Error - %s (%d). Retrying.\n", strerror(errno), errno);
        }
    }
}

int get_configuration(uint16_t *IOTPortPtr, uint16_t *fusionRxPortPtr, uint16_t *fusionTxPortPtr)
{
    char *iotDevicePortStr = NULL;
    char *fusionRxPortStr = NULL;
    char *fusionTxPortStr = NULL;

    // Read configuration from usp_service.cfg file.
    iotDevicePortStr = get_config_param(GetConfigFileName(), "IOT_Device_Port");
    if(iotDevicePortStr == NULL){
        hslog_error("Failed to parse IOT device port from config file.\n");
        return -1;
    }
    
    *IOTPortPtr = strtoul(iotDevicePortStr, NULL, 10);
    if((*IOTPortPtr == 0) || (*IOTPortPtr == ULONG_MAX)){
        // Failed to convert port number
        hslog_error("Failed to parse IOT device port from config file.IOT device port number %d out of range.\n", *IOTPortPtr);
        return -1;
    }

    fusionRxPortStr = get_config_param(GetConfigFileName(), "Fusion_RxPort");
    if(fusionRxPortStr == NULL){
        hslog_error("Failed to parse fusion port from config file.\n");
        return -1;
    }
    
    *fusionRxPortPtr = strtoul(fusionRxPortStr, NULL, 10);
    if((*fusionRxPortPtr == 0) || (*fusionRxPortPtr == ULONG_MAX)){
        // Failed to convert port number
        hslog_error("Failed to parse IOT device port from config file.IOT device port number %d out of range.\n", *fusionRxPortPtr);
        return -1;
    }

    fusionTxPortStr = get_config_param(GetConfigFileName(), "Fusion_TxPort");
    if(fusionTxPortStr == NULL){
        hslog_error("Failed to parse fusion port from config file.\n");
        return -1;
    }
    
    *fusionTxPortPtr = strtoul(fusionTxPortStr, NULL, 10);
    if((*fusionTxPortPtr == 0) || (*fusionTxPortPtr == ULONG_MAX)){
        // Failed to convert port number
        hslog_error("Failed to parse IOT device port from config file.IOT device port number %d out of range.\n", *fusionTxPortPtr);
        return -1;
    }

    return 0;
}

static  Thread_Info_t IotThreadsInfo;
static  Thread_Info_t fusionTxThreadInfo;
static  Thread_Info_t fusionRxThreadInfo;
static  Thread_Info_t rUdpRxThreadInfo;

static  UDP_socket_t  IOTSocket;
static  UDP_socket_t  fusionRxSocket;
static  UDP_socket_t  fusionTxSocket;
static  UDP_socket_t  rUDPsocket;

Fifo_Cluster_t fifosCluster;
TerminalCluster_t *terminalCLusterPtr = NULL;
uint16_t fusionRxPort = 0; // 8081
uint16_t fusionTxPort = 0; // 8082
uint16_t rUdpRxPort= R_UDP_PORT_TERM;

int start_udp_service(void)
{
    int result = 0;
    int index = 0;
    pthread_attr_t attr;
    BufPool_t *IOT_rxBuffer = NULL;
    BufPool_t *fusionRxMsgBuffer = NULL;
    uint8_t *bufferPtr = NULL;
    pthread_t udpIOTReceiveThreadId;
    pthread_t udpIOTTransmitThreadId;
    pthread_t udpFusionCommsThreadId;
    pthread_t udpReliableThreadId;

    /////////////////////////////////////////////////////////////////
    // TBD - This will be a doubly linked list and not array of pointers
    // in deployment. For time being and testing, we have used array of pointers.
    TerminalInfo_t *activeTerminals[MAX_ACTIVE_TERMINALS];
    ////////////////////////////////////////////////////////////////////

    // Open configuration File and read the fields
    if(get_configuration(&IOTPort, &fusionRxPort, &fusionTxPort) != 0){
        hslog_error("Failed to get port configuration.\n");
        return -1;
    }

    hslog_info("IOT device Port number - %d\n", IOTPort);
    hslog_info("Fusion Rx Port number - %d\n", fusionRxPort);
    hslog_info("Fusion Tx Port number - %d\n", fusionTxPort);
    hslog_info("R-UDP Rx Port number - %d\n",  fusionRxPort);

    // Create Server socket for communication with IOT device client
    // ////////////////////////////////////////////////////////////////////////////////////////////////
    // IMPORTANT NOTE for Terminal compilation:
    // Make sure that iot_transmit_routine transmits to IOT device only after receiving data from
    // IOT device, terminal doesn't know the IP address of IOT device. We can only know the
    // IOT device socket address, after we receive data in recvfrom() and the sockaddr structure
    // gets updated.
    // ////////////////////////////////////////////////////////////////////////////////////////////////
    if(create_server_socket(&IOTSocket, IOTPort) != 0){
        hslog_error("Failed to create IOT device socket.\n");
        return -1;
    }

    // Since fusion executes on the same machine, we cannot have same port number
    // for sending and receiving data. We cannot do two binds on the same
    // IP->port pair in two different programs (UDP service and Fusion).
    // Hence we have different Rx port and Tx port.
    if(create_server_socket(&fusionRxSocket, fusionRxPort) != 0){
        hslog_error("Failed to create fusion receive socket.\n");
        return -1;
    }

    // Create Client socket for communication with Fusion
    if(create_client_socket(&fusionTxSocket, "127.0.0.1", fusionTxPort) != 0){
        hslog_error("Failed to create fusion transmit socket.\n");
        return -1;
    }

    // Create R-UDP Server socket for communication with Fusion
    if(create_server_socket(&rUDPsocket, rUdpRxPort) != 0){
        hslog_error("Failed to create r-udp socket.\n");
        return -1;
    }

    fifosCluster.IotThrInFifo = Fifo_create();
    if(!fifosCluster.IotThrInFifo){
        hslog_error("Failed to create fifo.Line - %d.\n", __LINE__);
        return -1;
    }

    fifosCluster.IotThrOutFifo = Fifo_create();
    if(!fifosCluster.IotThrOutFifo){
        hslog_error("Failed to create fifo.Line - %d.\n", __LINE__);
        return -1;
    }

    fifosCluster.fusionThrInFifo = Fifo_create();
    if(!fifosCluster.fusionThrInFifo){
        hslog_error("Failed to create fifo.Line - %d.\n", __LINE__);
        return -1;
    }

    fifosCluster.fusionThrOutFifo = Fifo_create();
    if(!fifosCluster.fusionThrOutFifo){
        hslog_error("Failed to create fifo.Line - %d.\n", __LINE__);
        return -1;
    }

    fifosCluster.rUdpThrInFifo = Fifo_create();
    if(!fifosCluster.rUdpThrInFifo){
        hslog_error("Failed to create r-udp fifo\n");
        return -1;
    }


    // create the IOT receive Buffer.
    result = create_bufpool(MAX_BUFFER_ELEMS, sizeof(Msg_Buf_t), &IOT_rxBuffer, "IOT Receive Buffer");
    if(result != 0){
        hslog_error("Failed to createa IOT rx buffer");
        return -1;
    }

    // Feed the tx thread fifo
    for(index = 0;index < MAX_BUFFER_ELEMS-1;index++){
        bufferPtr = bufpool_getNextFreeBuffer(IOT_rxBuffer);
        Fifo_put(fifosCluster.IotThrInFifo, (void **)&bufferPtr);
    }

    // create the IOT receive Buffer.
    result = create_bufpool(MAX_BUFFER_ELEMS, sizeof(Msg_Buf_t), &fusionRxMsgBuffer, "IOT Transmit Buffer");
    if(result != 0){
        hslog_error("Failed to createa IOT rx buffer");
        return -1;
    }

    // Feed the tx thread fifo
    for(index = 0;index < MAX_BUFFER_ELEMS-1;index++){
        bufferPtr = bufpool_getNextFreeBuffer(fusionRxMsgBuffer);
        Fifo_put(fifosCluster.fusionThrInFifo, (void **)&bufferPtr);
    }

    // Init the Terminal cluster
    if(init_terminal_cluster(&terminalCLusterPtr) != 0){
        hslog_error("Failed to init terinal cluster.\n");
        return -1;
    }

    // Create threads for receiving and sending to IOT
    result = pthread_attr_init(&attr);
    if (result != 0){
        hslog_error("Failed to init pthread attribute entity.\n");
        return result;    
    }

    // set detached state for the thread
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // create the threads.
    // 1. IOT receive thread.
    hslog_info("Starting receiver thread for data from IOT device.\n");
    IotThreadsInfo.sockPtr = &IOTSocket;
    IotThreadsInfo.fifoClusterPtr = &fifosCluster;

    result = pthread_create(&udpIOTReceiveThreadId, &attr, &iot_receive_routine, &IotThreadsInfo);
    if (result < 0){
        hslog_error("Failed to create iot receive thread. Error - %s.\n", strerror(result));
        return result;
    }

    // 2. transmitter thread for Data to IOT device.
    hslog_info("Starting transmitter thread for data to IOT device.\n");
    result = pthread_create(&udpIOTTransmitThreadId, &attr, &iot_transmit_routine, &IotThreadsInfo);
    if (result < 0){
        hslog_error("Failed to create iot tansmit thread. Error - %s.\n", strerror(result));
        return result;
    }

    // Fusion data handler thread.
    hslog_info("Starting Fusion data transmit thread.\n");
    fusionTxThreadInfo.sockPtr = &fusionTxSocket;
    fusionTxThreadInfo.fifoClusterPtr = &fifosCluster;
    fusionTxThreadInfo.terminalClusterPtr = NULL; // Unused by Fusion transmit thread
    result = pthread_create(&udpFusionCommsThreadId, &attr, &fusion_transmit_routine, &fusionTxThreadInfo);
    if (result < 0){
        hslog_error("Failed to create fusion thread. Error - %s.\n", strerror(result));
        return result;
    }

    hslog_info("Starting Fusion data receive thread.\n");
    fusionRxThreadInfo.sockPtr = &fusionRxSocket;
    fusionRxThreadInfo.fifoClusterPtr = &fifosCluster;
    fusionRxThreadInfo.terminalClusterPtr = NULL; // Unused by fusion receive thread
    result = pthread_create(&udpFusionCommsThreadId, &attr, &fusion_receive_routine, &fusionRxThreadInfo);
    if (result < 0){
        hslog_error("Failed to create fusion thread. Error - %s.\n", strerror(result));
        return result;
    }

    rUdpRxThreadInfo.sockPtr = &rUDPsocket;
    rUdpRxThreadInfo.fifoClusterPtr = &fifosCluster;
    rUdpRxThreadInfo.sockPtr2 = &fusionTxSocket;

    result = pthread_create(&udpReliableThreadId, &attr, &r_udp_routine, &rUdpRxThreadInfo);
    if (result < 0){
        hslog_error("Failed to create fusion thread. Error - %s.\n", strerror(result));
        return result;
    }

    result = pthread_create(&udpReliableThreadId, &attr, &r_udp_req_routine, &rUdpRxThreadInfo);
    if (result < 0){
        hslog_error("Failed to create fusion thread. Error - %s.\n", strerror(result));
        return result;
    }
    return 0;
}
