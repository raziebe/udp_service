#ifndef __HS_UDP_FRAGMENTS_H
#define __HS_UDP_FRAGMENTS_H

#include "hs_udp.h"
#include "hs_udp_service.h"

#if defined(HUBPP)
// In HUB, we will have 64 elements in the fragments table, each processing defragmentation for each terminal
// it is an estimate that we will be receiving data from 64 active terminals.
// After processing 500 bytes for a terminal that terminal's fragment will be released for data coming from another terminal.
// This way we do not create huge buffer but still cater for 4096 terminals. If we make entry for each terminal
// in the table we need 4096 elements in the fragments table, each assembling 1500 bytes data for each terminal.
// we will then need 1500*4096 bytes buffer which is 5 Mbytes. 64x1500 gives 93Kbytes buffer. 
#define FRAGMENTS_TABLE_COUNT     64
#else
#define FRAGMENTS_TABLE_COUNT     1
#endif

typedef struct UDP_Fragment
{
    bool used;
    uint8_t frgmntIdx;
    uint16_t size;
    uint16_t terminalId;
    uint8_t data[NET_PACKET_SIZE];
}UDP_Fragment_t;

typedef struct UDP_Fragments_Table
{
    uint8_t unusedIdx;
    UDP_Fragment_t fragments[FRAGMENTS_TABLE_COUNT];
}UDP_Fragments_Table_t;

void init_fragments_table(UDP_Fragments_Table_t *fragmentsTable);

// Return valid pointer to UDP_Fragment_t if the reassembly is complete
int defragment(UDP_Fragments_Table_t *fragmentsTable, Msg_Buf_t *msg, UDP_Fragment_t **outFragment);

// Return pointer to existing fragment that holds data for a given terminal ID, if doesn't exist
// finds an used fragment handle and returns it, if no free handles, returns NULL.
UDP_Fragment_t* get_fragment_handle(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId);
int release_fragment(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId);




#endif //