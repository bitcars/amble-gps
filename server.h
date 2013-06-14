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
	clientId cid;
	comReceiver *receiver;
} AmbleClientInfo;

void serverOnLine(void);
void serverOffLine(void);

bool stdinSelected(void);

int serverRings(AmbleClientInfo ** pClientInfoPtr);
void serverHangup(AmbleClientInfo * client);

void handler(AmbleClientInfo * clientInfo);

#endif /* SERVER_H_ */
