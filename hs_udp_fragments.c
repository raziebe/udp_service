#include <stdio.h>
#include <string.h>
#include "hs_udp_fragments.h"
#include "hslog/hslog.h"

void init_fragments_table(UDP_Fragments_Table_t *fragmentsTable)
{
    fragmentsTable->unusedIdx = 0;
}

UDP_Fragment_t* find_terminal(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId)
{
    int index = 0;
    for(index = 0;index < FRAGMENTS_TABLE_COUNT;index++){
        if((fragmentsTable->fragments[index].terminalId == terminalId) && 
            fragmentsTable->fragments[index].used)
            return &fragmentsTable->fragments[index];
    }

    return NULL;
}

UDP_Fragment_t* get_freeFragment(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId)
{
    int index = 0;
    for(index = fragmentsTable->unusedIdx; (index < FRAGMENTS_TABLE_COUNT) && (fragmentsTable->fragments[index].used == 1); index++);

    if(index >= FRAGMENTS_TABLE_COUNT){
        for(index = 0; (index < fragmentsTable->unusedIdx) && (fragmentsTable->fragments[index].used == 1); index++);
        if(index >= fragmentsTable->unusedIdx){
            hslog_error("No free fragment handles in pool.\n");
            return NULL;
        }
    }

    fragmentsTable->unusedIdx = index;
    fragmentsTable->fragments[index].terminalId = terminalId;
    fragmentsTable->fragments[index].used = true;
    return &fragmentsTable->fragments[index];
}


UDP_Fragment_t* get_fragment_handle(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId)
{
    UDP_Fragment_t* handle = find_terminal(fragmentsTable, terminalId);

    // find if the given terminal ID already exists.
    if(handle == NULL){
        // Terminal doesn't exist find an unused one.
        handle = get_freeFragment(fragmentsTable, terminalId);
    }
    
    return handle;
}

int release_fragment(UDP_Fragments_Table_t *fragmentsTable, uint16_t terminalId)
{
    UDP_Fragment_t* handle = find_terminal(fragmentsTable, terminalId);

    if(handle == NULL)
        return -1;
    else
        memset(handle, 0, sizeof(UDP_Fragment_t));    

    return 0;
}

/*
 *	Returns 0 when the entire message is colated
*/
int defragment(UDP_Fragments_Table_t *fragmentsTable, Msg_Buf_t *msg, UDP_Fragment_t **outFragment)
{
    uint8_t frgmntIdx = 0;
    uint16_t terminalId = msg->termId;
    UDP_Fragment_t* handle = NULL;

    if((msg->msgHeader.frgmntByte & (0x01)) || ((msg->msgHeader.frgmntByte & 0xFE) != 0)) {

        // Fragmented packet, needs defragmentation.
        frgmntIdx = (msg->msgHeader.frgmntByte & 0xFE) >> 1;        
        handle = find_terminal(fragmentsTable, terminalId);
        if(!handle){
            // Does not exist, create new one.
            handle = get_freeFragment(fragmentsTable, terminalId);

            // Check if free slots available.
            if(!handle){
                // Cannot process msg, drop the packet
                hslog_error("No free frgament slots available for defragmentation.\n");
                memset(handle, 0, sizeof(UDP_Fragment_t));
                *outFragment = NULL;
                return -1;
            }
        }
        
        frgmntIdx = (msg->msgHeader.frgmntByte & 0xFE) >> 1;
        if(handle->frgmntIdx == frgmntIdx){
            // Copy the contents
            memcpy(handle->data + handle->size, msg->data, msg->msgLen);
            handle->frgmntIdx = frgmntIdx + 1; // Next fragment index.
            handle->size += msg->msgLen;

            if(msg->msgHeader.frgmntByte & (0x01)){
                // More fragments to come, don't return the defragmented data yet.
                *outFragment = NULL;
                return -1;
            }
            else{
                // Last fragment, return the defragmented data.
                *outFragment = handle;
                return 0;
            }
        }
        else{
            // out of order packet, we drop the packet
            hslog_error("Fragment index out of order. handle->frgmntIdx - %d. frgmntIdx - %d\n", handle->frgmntIdx, frgmntIdx);
            memset(handle, 0, sizeof(UDP_Fragment_t));
            *outFragment = NULL;
            return -1;
        }
    }
    else{
        // No defragmentation required, it is a single packet.
        *outFragment = NULL;
    }

    return 0;
}
