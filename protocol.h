/*
 * protocol.h
 *
 * Here we describe a protocol between the client and server
 * sides of a network communication implementation based on UDP
 *
 * The protocol is generic. It can be used for most client server
 * communication model with UDP.
 *
 *  Created on: Apr 20, 2013
 *      Author: yiding
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdbool.h>

#include "global.h"

#define COM_SUCCESS 0
#define COM_FAILURE -1

#define DEBUG 1

#if DEBUG
int Printf (const char * format, ...);
#else
#define Printf(...)
#endif

typedef unsigned clientId;
/*
 * The Packaging protocol
 */
#define COM_HEADER_SIZE	12

typedef struct comPackage {
	char header[COM_HEADER_SIZE];
	void * pData;
	size_t uDataBytes;
} comPackage;

comPackage * newComPackage(void);

/*
 *  sender communication protocol
 */

typedef struct comSender {
	int sockfd;
	int flag;
	uint32_t latestId;
} comSender;

comSender * newComSender(void);
/*
 *  receiver communication protocol
 */

typedef struct comReceiver {
	uint32_t lastId;
} comReceiver;

comReceiver * newComReceiver(void);

bool comDetectConnection(const comReceiver* pReceiver);

int comPackData(comPackage* pPackage, void * pData, const size_t uDataSize);
int comSendData(comSender* pSender, comPackage* pPackage);
int comReceiveData(const comReceiver* pReceiver, const comPackage* pPackage, char ** ppData, size_t* pLength);

#endif /* PROTOCOL_H_ */
