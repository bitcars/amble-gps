/*
 * global.h
 *
 *  Created on: Feb 6, 2013
 *      Author: yiding
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_
#include <stdint.h>
#include <stdio.h>

/* socket port and max message length */
#define SERVER_PORT 1600
#define MAX_MSG 1024

/*
 * Data transfer Protocol
 */
struct gps_package {
    float lat;
    float lon;
};


#define SUCCESS 0
#define ERROR   1

#define DELIMITER_BYTE  0xFE
#define NOFIX_BYTE		0x01
#define GPS_BYTE		0x02


typedef uint32_t CheckSum_t;

CheckSum_t ChecksumCalculator(const void * buf, size_t n);

CheckSum_t ChecksumCalculator(const void * buf, size_t n) {
	CheckSum_t checksum = 0;
	unsigned i;
	for (i = 0; i < n; i++) {
		checksum += *(((uint8_t *)buf) + i);
	}
	printf("checksum %u\n", checksum);
	return checksum;
}

#endif /* GLOBAL_H_ */
