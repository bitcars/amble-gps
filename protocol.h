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
#include <stdint.h>
#include <netdb.h>

#include "global.h"

#define COM_SUCCESS 0
#define COM_FAILURE -1

typedef unsigned clientId;
/*
 * The Packaging protocol
 */
#define COM_HEADER_BYTES	12

typedef struct comPackage {
	char header[COM_HEADER_BYTES];
	void * pData;
	size_t uDataBytes;
} ComPackage;

ComPackage * newComPackage(void);

/*
 *  sender communication protocol
 */

typedef struct comSender {
	int sockfd;
	int flag;
	uint32_t latestId;
	struct sockaddr recvAddr;
	socklen_t addrlen;
} ComSender;

ComSender * newComSender(void);
/*
 *  receiver communication protocol
 */

typedef struct comReceiver {
	int sockfd;
	uint32_t lastId;
	struct sockaddr_storage sender;
} ComReceiver;


/* simple 32-bit floating point encapsulation */
uint32_t htonf(float f);
float ntohf(uint32_t i);

ComReceiver * newComReceiver(void);

#define PACK_GPS(buf, gps) pack((buf), "ddfff", (gps)->lat, (gps)->lon, (gps)->alt, (gps)->speed, (gps)->heading)
#define UNPACK_GPS(buf, gps) unpack((buf), "ddfff", &((gps).lat), &((gps).lon), &((gps).alt), &((gps).speed), &((gps).heading))

int32_t pack(unsigned char *buf, char *format, ...);
void unpack(unsigned char *buf, char *format, ...);

bool comDetectConnection(const ComReceiver* pReceiver);

int comPackData(ComPackage* pPackage, void * pData, const size_t uDataSize);
int comSendData(ComSender* pSender, ComPackage* pPackage);
int comReceiveData(ComReceiver* pReceiver, ComPackage* pPackage);

#endif /* PROTOCOL_H_ */
