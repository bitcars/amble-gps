/*
 * protocal.c
 *
 *  Created on: Apr 21, 2013
 *      Author: yiding
 */

#include "protocol.h"


struct __comPackageHeader {
	uint16_t protocolId;
	uint16_t packageId;
	uint32_t ack;
};
