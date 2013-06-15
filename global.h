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

#define DEBUG 1

#if DEBUG
int Printf (const char * format, ...);
#else
#define Printf(...)
#endif


/* socket port and max message length */
#define SERVER_PORT "3412"
#define CLIENT_PORT "5937"
#define MAX_MSG 1024
#define BACKLOG 5
typedef float float32_t;
typedef double float64_t;
/*
 * Data transfer Protocol
 */
struct gps_package {
	float64_t lat;
	float64_t lon;
    float32_t alt;
    float32_t speed;
    float32_t heading;
};

#define GPS_PACKAGE_SIZE_BYTE 28


#define SUCCESS 0
#define ERROR   1

#define DELIMITER_BYTE  0xFE
#define NOFIX_BYTE		0x23
#define GPS_BYTE		0x34


typedef uint32_t CheckSum_t;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);
in_port_t get_in_port(struct sockaddr *sa);

CheckSum_t ChecksumCalculator(const void * buf, size_t n);



#endif /* GLOBAL_H_ */
