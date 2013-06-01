/*
 * protocal.c
 *
 *  Created on: Apr 21, 2013
 *      Author: yiding
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <stdarg.h>
#include "protocol.h"

#define UNINITPKG  0xDEADBEEF
#define PROTOCOL   0x8EF3F38E

#define COMPILE_ASSERT(pred) switch(0){case 0:case pred:;}

typedef struct _comPackageHeader {
	uint32_t protocolId;
	uint32_t packageId;
	uint32_t ack;
} comHeader;

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

void compile_time_assertions(void) {
	COMPILE_ASSERT(sizeof(comHeader) == COM_HEADER_SIZE)
}

comPackage * newComPackage(void) {
	comPackage * ptr = (comPackage *) malloc( sizeof(comPackage) );
	if (ptr == NULL) {
		printf("Fail to allocate memory space\n");
		exit(1);
	}

	((comHeader *) ptr)->protocolId = UNINITPKG;
	return ptr;
}

comSender * newComSender(void) {
	comSender * ptr = (comSender *) malloc( sizeof(comSender) );
	if (ptr == NULL) {
		printf("Fail to allocate memory space\n");
		exit(1);
	}
	return  ptr;
}

comReceiver * newComReceiver(void) {
	comReceiver * ptr = (comReceiver *) malloc( sizeof(comSender) );
	if (ptr == NULL) {
		printf("Fail to allocate memory space\n");
		exit(1);
	}

	return ptr;
}

int comPackData(comPackage* pPackage, void * pData, const size_t uDataSize) {
	if (pData != NULL && pPackage != NULL) {
		((comHeader *)pPackage)->protocolId = PROTOCOL;
		pPackage->pData = pData;
		pPackage->uDataBytes = uDataSize;

		return COM_SUCCESS;
	}
	else {
		return COM_FAILURE;
	}
}

int comSendData(comSender* pSender, comPackage* pPackage) {
	if (pSender == NULL || pPackage == NULL)
		return COM_FAILURE;

	/* validate if package has been initialized */
	comHeader* header = (comHeader *) pPackage;
	if (header->protocolId == UNINITPKG)
		return COM_FAILURE;

	/* Increment the latest Id */
	pSender->latestId++;

	/* create data package to send */
	char * data = (char *) malloc(sizeof(comHeader) + pPackage->uDataBytes);
	((comHeader *)data)->protocolId = header->protocolId;
	((comHeader *)data)->packageId  = pSender->latestId;
	((comHeader *)data)->ack        = header->ack;

	data += sizeof(struct _comPackageHeader);
	memcpy(data, pPackage->pData, pPackage->uDataBytes);

	/* assuming a connection is previously established */
	int bytesSent = send(pSender->sockfd, data, pPackage->uDataBytes, pSender->flag);

	free(data);
	return bytesSent;
}

int comReceiveData(const comReceiver* pReceiver, const comPackage* pPackage, char ** ppData, size_t* pLength) {
	return 0;
}
