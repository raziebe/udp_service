/*
   udp_sat_data.h 
*/
#ifndef __UDP_SAT_DATA_H__
#define __UDP_SAT_DATA_H__
/****************************************** Includes *************************************/
#include <inttypes.h>


/******************************************** types ***************************************/

typedef struct  __attribute__ ((__packed__)) udp_sat_data_ {
    // Bit 0, if set indicates that this payload is a fragment of bigger message.
    // Bit 1-7, indicate the fragment index supporting a maximum of 127 fragments.
    uint8_t     frgmntByte;
    uint16_t    port;       //UDP port on the Terminal side
    uint8_t     data[1];
} udp_sat_hub_t;

#endif //__UDP_SAT_DATA_H__
