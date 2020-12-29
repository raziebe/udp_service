#ifndef __HS_UDP_FRAGMENTS_H
#define __HS_UDP_FRAGMENTS_H

#include "hs_udp.h"
#include "hs_udp_service.h"

#define FRAGMENTS_TABLE_COUNT     1
#define MAX_PKTS 4	//  Msg_Header.seqNum. We provide at most 16 pkts per Terminal

typedef struct UDP_Fragment {
    bool  used;
    uint8_t frgmntIdx;
    uint8_t accumulatedFrgmnts; // accumalted total number of fragments
    uint8_t frgmnts;
    uint8_t pktSeqNum;		// depends on the number of bits in Msg_Header_t
    uint32_t frags_mask;	// mark the arrived bits
    struct timeval lastTime; // last time time the handle was accessed
    uint16_t size;
    uint16_t terminalId;
    uint8_t data[NET_PACKET_SIZE];
} UDP_Fragment_t;


typedef struct UDP_Fragments_Table {
    uint8_t unusedIdx;
    UDP_Fragment_t fragments[FRAGMENTS_TABLE_COUNT][MAX_PKTS + 1];
} UDP_Fragments_Table_t;


void init_fragments_table(UDP_Fragments_Table_t *fragmentsTable);

// Return valid pointer to UDP_Fragment_t if the reassembly is complete
int defragment(UDP_Fragments_Table_t *fragmentsTable, Msg_Buf_t *msg, UDP_Fragment_t **outFragment);

// Return pointer to existing fragment that holds data for a given terminal ID, if doesn't exist
// finds an used fragment handle and returns it, if no free handles, returns NULL.
UDP_Fragment_t* get_fragment_handle(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId);
UDP_Fragment_t* find_terminal(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId,uint8_t pktSeq);
UDP_Fragment_t* get_freeFragment(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId,uint8_t pktSeq);
int release_fragment(UDP_Fragment_t* handle);

#endif
