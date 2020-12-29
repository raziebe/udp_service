
#ifndef __TERMINALS_HANDLER_H_
#define __TERMINALS_HANDLER_H_

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "hs_udp.h"

// Total number of terminals supported. 4096 in MVP. 10 in testing.
#if defined(HUBPP)
#define MAX_TERMINALS_COUNT          4096
#else
#define MAX_TERMINALS_COUNT          1
#endif

// Currently we are using a set number of active terminals for testing
// this will be changed to dynamic number ranging from 1-4096 as and when 
// the terminals register
#if defined(HUBPP)
// Compiling for HUBPP, supports more than 1 terminal.
#define MAX_ACTIVE_TERMINALS         4096
#else
// Compiling for Terminal
#define MAX_ACTIVE_TERMINALS         1
#endif

typedef struct TerminalInfo
{
    bool            used;
    uint16_t        terminalID;
    uint16_t        port;
    uint32_t        ip;
    uint8_t         hiSkyId[6];
}TerminalInfo_t;

/*typedef struct ActiveTerminals
{
    TerminalInfo_t              *activeTerminal;
    struct ActiveTerminals      *next;
    struct ActiveTerminals      *prev;
}ActiveTerminals_t;*/

typedef struct TerminalCluster
{
    // Static pool of Terminals and their Data
    TerminalInfo_t          TerminalPool[MAX_TERMINALS_COUNT];  // Pool of Terminal Objects, one will be allocted everytime a new terminal registers
    pthread_mutex_t         terminalPoolMutex;                  // Mutex for mutual exclusion with respect to Fusion
    uint16_t                ActiveTerminals;                    // Total number of active terminals.
} TerminalCluster_t;

int init_terminal_cluster(TerminalCluster_t **terminalClusterPtr);                                                        // main init func zero out params.
const TerminalInfo_t* get_terminal(TerminalCluster_t *terminalClusterPtr, uint16_t terminalID);
int remove_terminal(TerminalCluster_t *terminalClusterPtr, uint16_t terminalId);
int add_TerminalData(TerminalCluster_t *terminalClusterPtr, uint16_t terminalId, uint16_t port, uint32_t ip, char * hiSkyId);


#endif // __TERMINALS_HANDLER_H_