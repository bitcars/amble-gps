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

#include "global.h"

/*
 * The Packaging protocol
 */

struct _comPackageHeader;

typedef struct comPackage {
	struct _comPackageHeader header;
	void * pData;
	size_t uDataSize;
} comPackage;

comPackage * newComPackage(void);
void deleteComPackage(comPackage * ptr);

/*
 *  sender communication protocol
 */

typedef struct comSender {

} comSender;

/*
 *  receiver communication protocol
 */

typedef struct comReceiver {

} comReceiver;

/*
 * listener communication protocol
 */

typedef struct comListener {

}comListener;

#endif /* PROTOCOL_H_ */
