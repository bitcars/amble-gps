/*
 * client.h
 *
 * Defines client behavior
 *
 *  Created on: Apr 22, 2013
 *      Author: yiding
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "protocol.h"


#define CALL_TRIES  3
#define RETRY_TIMER 10.0

int clientCall(char * serverName);

#endif /* CLIENT_H_ */
