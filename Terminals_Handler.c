
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "Terminals_Handler.h"
#include "hslog/hslog.h"

int init_terminal_cluster(TerminalCluster_t **terminalClusterPtr)
{
    // Allocate memory for the Terminal Cluster.
    *terminalClusterPtr = (TerminalCluster_t *)malloc(sizeof(TerminalCluster_t));

    if(terminalClusterPtr == NULL){
        hslog_error("Failed to allocate memory for Terminal Pool.\n");
        return -1;
    }

    memset(*terminalClusterPtr, 0, sizeof(TerminalCluster_t));

    // init cluster mutex
    pthread_mutex_init(&(*terminalClusterPtr)->terminalPoolMutex, NULL);

    return 0;
}

int add_TerminalData(TerminalCluster_t *terminalClusterPtr, uint16_t terminalId, uint16_t port, uint32_t ip, char * hiSkyId)
{
    int terminalIndex = terminalId - 1;
    const int HiSKyIdLen = 6;

    if(terminalClusterPtr == NULL){
        hslog_error("Terminal Pool not initialised.\n");
        return -1;
    }

    if(terminalId > MAX_TERMINALS_COUNT){
        hslog_error("Invalid Terminal ID - %d.\n", terminalId);
        return -1;
    }

    // Find the next free entry
    pthread_mutex_lock(&terminalClusterPtr->terminalPoolMutex);

    // Update the terminal data
    if(!terminalClusterPtr->TerminalPool[terminalIndex].used){
        terminalClusterPtr->TerminalPool[terminalIndex].used = true;
        terminalClusterPtr->ActiveTerminals++;
    }
    terminalClusterPtr->TerminalPool[terminalIndex].ip = ip;
    terminalClusterPtr->TerminalPool[terminalIndex].port = port;
    terminalClusterPtr->TerminalPool[terminalIndex].terminalID = terminalId;
    memcpy(terminalClusterPtr->TerminalPool[terminalIndex].hiSkyId, hiSkyId, HiSKyIdLen);
    pthread_mutex_unlock(&terminalClusterPtr->terminalPoolMutex);
    return 0;
}

int remove_terminal(TerminalCluster_t *terminalClusterPtr, uint16_t terminalId)
{
    uint16_t terminalIndex = terminalId - 1;
    if((terminalId > MAX_TERMINALS_COUNT) || !terminalId ){
        hslog_error("Invalid Terminal ID - %d.\n", terminalId);
        return -1;
    }

    if(terminalClusterPtr == NULL){
        hslog_error("Terminal Pool not initialised.\n");
        return -1;
    }

    pthread_mutex_lock(&terminalClusterPtr->terminalPoolMutex);
    if(terminalClusterPtr->TerminalPool[terminalIndex].used){
        memset(&terminalClusterPtr->TerminalPool[terminalIndex], 0, sizeof(terminalClusterPtr->TerminalPool[terminalIndex]));
        terminalClusterPtr->ActiveTerminals--;
        pthread_mutex_unlock(&terminalClusterPtr->terminalPoolMutex);
    }
    else{
        pthread_mutex_unlock(&terminalClusterPtr->terminalPoolMutex);
        hslog_error("Terminal ID(%d) not in use.\n", terminalId);
        return -1;
    }
    
    return 0;
}

const TerminalInfo_t* get_terminal(TerminalCluster_t *terminalClusterPtr, uint16_t terminalId)
{
    uint16_t terminalIndex = terminalId - 1;
    if((terminalId > MAX_TERMINALS_COUNT) || !terminalId ){
        hslog_error("Invalid Terminal ID - %d.\n", terminalId);
        return NULL;
    }

    if(terminalClusterPtr == NULL){
        hslog_error("Terminal Pool not initialised.\n");
        return NULL;
    }

    if(!terminalClusterPtr->TerminalPool[terminalIndex].used){
        hslog_error("Terminal (%d) not in use.\n", terminalId);
        return NULL;
    }

    return &terminalClusterPtr->TerminalPool[terminalIndex];
}
