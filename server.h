/*
 * server.h
 *
 * Defines server behavior.
 *  Created on: Apr 22, 2013
 *      Author: yiding
 */

#ifndef SERVER_H_
#define SERVER_H_

#include "protocol.h"


typedef struct ambleOperator {
	int remotefd;
	struct sockaddr_storage remoteAddr;
} AmbleClientInfo;

void serverOnLine(void);
void serverOffLine(void);

int serverRings(AmbleClientInfo ** pClientInfoPtr);
void serverHangup(AmbleClientInfo * client);

void handler(AmbleClientInfo * clientInfo);

#endif /* SERVER_H_ */
