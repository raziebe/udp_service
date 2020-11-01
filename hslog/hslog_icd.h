#ifndef __ICD_HSLOGGER__
#define __ICD_HSLOGGER__

#define HSMODEM		1
#define UDP_SRV		2
#define BLE_SRV		3


struct  __attribute__ ((packed)) icd_hslog_hdr  {
	uint32_t	seq;
	uint16_t	len;	// text + header
	uint8_t		module;
	uint8_t		loglevel;
};


struct   __attribute__ ((__packed__)) icd_hslog {
	struct icd_hslog_hdr hdr;
	char		text[1];
} __attribute__ ((aligned(1)))  ;

#endif

