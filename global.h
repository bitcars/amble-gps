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
#include <arpa/inet.h>

/* socket port and max message length */
#define SERVER_PORT "3412"
#define CLIENT_PORT "5937"
#define MAX_MSG 1024
#define BACKLOG 5

/*
 * Data transfer Protocol
 */
struct gps_package {
    float lat;
    float lon;
    float alt;
    float speed;
    float heading;
};


#define SUCCESS 0
#define ERROR   1

#define DELIMITER_BYTE  0xFE
#define NOFIX_BYTE		0x23
#define GPS_BYTE		0x34


typedef uint32_t CheckSum_t;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

CheckSum_t ChecksumCalculator(const void * buf, size_t n);



#endif /* GLOBAL_H_ */
