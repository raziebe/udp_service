#include <string.h>
#include <stdio.h>

#include "hs_udp_fragments.h"
#include "hslog/hslog.h"

/* UDP server global data */
UDP_Fragments_Table_t fragmentsTable;

void init_fragments_table(UDP_Fragments_Table_t *fragmentsTable)
{
    int index;
    int jndex;

    fragmentsTable->unusedIdx = 0;
    for(index = 0;index < FRAGMENTS_TABLE_COUNT;index++){
    	for (jndex = 0; jndex < MAX_PKTS; jndex++){
    				UDP_Fragment_t* frag;
     				frag = &fragmentsTable->fragments[index][jndex];
     				memset(frag, 0x00, sizeof(*frag));
    	}
    }
}

UDP_Fragment_t* find_terminal(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId,uint8_t pktSeq,int ver)
{
    int index = 0;

    if (pktSeq >= MAX_PKTS && ver == 2){
    	printf("%s Seq too high\n",__func__);
    	return NULL;
    }

    for(index = 0;index < FRAGMENTS_TABLE_COUNT;index++){

    		if ((fragmentsTable->fragments[index][pktSeq].terminalId == terminalId) &&
    				fragmentsTable->fragments[index][pktSeq].used){

			return &fragmentsTable->fragments[index][pktSeq];
    		}
    }

    return NULL;
}



UDP_Fragment_t* get_freeFragment(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId,uint8_t pktSeq)
{
    int index = 0;
    UDP_Fragment_t* frag = NULL;

    for(index = 0;index < FRAGMENTS_TABLE_COUNT;index++){
     	if (!fragmentsTable->fragments[index][pktSeq].used){
     				frag = &fragmentsTable->fragments[index][pktSeq];
     				break;
     	}
    }

    if (!frag)
    	return NULL;
    fragmentsTable->unusedIdx = index;
    frag->terminalId = terminalId;
    frag->used = true;
    frag->frgmntIdx = 0;
    frag->accumulatedFrgmnts = 0;
    frag->frgmnts =  0;
    frag->frags_mask = 0;
    frag->size = 0 ;
    frag->pktSeqNum = 0xFF;
    return frag;
}

int release_fragment(UDP_Fragment_t* handle)
{
    memset(handle, 0, sizeof(UDP_Fragment_t));
    return 0;
}


