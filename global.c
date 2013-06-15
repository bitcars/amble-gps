/*
 * global.c
 *
 *  Created on: Apr 27, 2013
 *      Author: yiding
 */

#include "global.h"

#include <stdarg.h>

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

in_port_t get_in_port(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in*)sa)->sin_port);
    }

    return (((struct sockaddr_in6*)sa)->sin6_port);
}

CheckSum_t ChecksumCalculator(const void * buf, size_t n) {
	CheckSum_t checksum = 0;
	unsigned i;
	for (i = 0; i < n; i++) {
		checksum += *(((uint8_t *)buf) + i);
	}
	//printf("checksum %u\n", checksum);
	return checksum;
}

#if DEBUG
int Printf (const char * format, ...){
    va_list args;
    va_start(args, format);
    int ret = 0;

	ret = vprintf(format, args);

    va_end(args);

	return ret;
}
#endif
