/*
 * protocal.c
 *
 *  Created on: Apr 21, 2013
 *      Author: yiding
 */

#include <ctype.h>
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
	ptr->sockfd = -1;

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
	/* encapsulate header */
	size_t datasize = COM_HEADER_SIZE+ pPackage->uDataBytes;
	unsigned char *data = (unsigned char *) malloc(datasize);
	if (data == NULL) {
		printf("Unable to malloc\n");
		exit(0);
	}
	pack(data, "lll", header->protocolId, header->packageId, header->ack);
	/* append content */
	unsigned char *content = data + COM_HEADER_SIZE;
	memcpy(content, pPackage->pData, pPackage->uDataBytes);

	/* assuming a connection is previously established */
	int bytesSent = sendto(pSender->sockfd, data, datasize, 0, &pSender->recvAddr, pSender->addrlen);

	free(data);
	return bytesSent;
}

int comReceiveData(const comReceiver* pReceiver, const comPackage* pPackage, char ** ppData, size_t* pLength) {
	return 0;
}

/*********************  Data Encapsulation  ***************************/
/**
 * Host to net for floating point
 * @params:
 * a floating point number that will be encapsulated in net order
 * @returns:
 * a 4 byte integer
 */
uint32_t htonf(float f) {
	uint32_t capsule;
	uint32_t sign;

	sign = (f < 0) ? 1 : 0;
	f = (f < 0) ? -f : f;
	capsule = ((((uint32_t)f)&0x7fff)<<16) | (sign<<31); // whole part and sign
	capsule |= (uint32_t)(((f - (int)f) * 65536.0f))&0xffff; // fraction
	return capsule;
}

/**
 * Net to host for floatig point
 * @params:
 * a 4 byte integer number that will be encapsulated in host order
 * @returns:
 * a floating point number
 */
 float ntohf(uint32_t p) {
	 float f = ((p>>16)&0x7fff); // whole part
	 f += (p&0xffff) / 65536.0f; // fraction
	 if (((p>>31)&0x1) == 0x1) { f = -f; } // sign bit set
	 return f;
 }

 /**
  * Pack a floats and double in IEEE754 format, mostly, not including
  * NaN or Infinity, which can be fixed in future work
  * @params:
  * f - the float or double number to be encapsualated
  * bits - either 32 (float) or 64 (double)
  * expbits - either 8 (float) or 11 (double)
  * @returns:
  * an 8 byte integer
  */
uint64_t pack754(long double f, unsigned bits, unsigned expbits) {
	long double fnorm;
	int shift;
	long long sign, exp, significand;
	unsigned significandbits = bits - expbits - 1; // -1 for sign bit
	if (f == 0.0)
		return 0; // get this special case out of the way
	// check sign and begin normalization
	if (f < 0) {
		sign = 1;
		fnorm = -f;
	} else {
		sign = 0;
		fnorm = f;
	}
	// get the normalized form of f and track the exponent
	shift = 0;
	while (fnorm >= 2.0) {
		fnorm /= 2.0;
		shift++;
	}
	while (fnorm < 1.0) {
		fnorm *= 2.0;
		shift--;
	}
	fnorm = fnorm - 1.0;
	// calculate the binary form (non-float) of the significant data
	significand = fnorm * ((1LL << significandbits) + 0.5f);
	// get the biased exponent
	exp = shift + ((1 << (expbits - 1)) - 1); // shift + bias
	// return the final answer
	return (sign << (bits - 1)) | (exp << (bits - expbits - 1)) | significand;
}

/**
 * UnPack a 64-bit integer to IEEE754 format, mostly, not including
 * NaN or Infinity, which can be fixed in future work
 * @params:
 * i - the 64-bit integer
 * bits - either 32 (float) or 64 (double)
 * expbits - either 8 (float) or 11 (double)
 * @returns:
 * a double number
 */
long double unpack754(uint64_t i, unsigned bits, unsigned expbits) {
	long double result;
	long long shift;
	unsigned bias;
	unsigned significandbits = bits - expbits - 1; // -1 for sign bit
	if (i == 0)
		return 0.0;
	// pull the significant
	result = (i & ((1LL << significandbits) - 1)); // mask
	result /= (1LL << significandbits); // convert back to float
	result += 1.0f; // add the one back on
	// deal with the exponent
	bias = (1 << (expbits - 1)) - 1;
	shift = ((i >> significandbits) & ((1LL << expbits) - 1)) - bias;
	while (shift > 0) {
		result *= 2.0;
		shift--;
	}
	while (shift < 0) {
		result /= 2.0;
		shift++;
	}
	// sign it
	result *= (i >> (bits - 1)) & 1 ? -1.0 : 1.0;
	return result;
}

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

/**
 * packi16()
 * store a 16-bit int into a char buffer (like htons())
 */
void packi16(unsigned char *buf, uint16_t i) {
	*buf++ = i >> 8;
	*buf++ = i;
}
/*
 ** packi32() -- store a 32-bit int into a char buffer (like htonl())
 */
void packi32(unsigned char *buf, uint32_t i) {
	*buf++ = i >> 24;
	*buf++ = i >> 16;
	*buf++ = i >> 8;
	*buf++ = i;
}
/*
 ** unpacki16() -- unpack a 16-bit int from a char buffer (like ntohs())
 */
uint16_t unpacki16(unsigned char *buf) {
	return (buf[0] << 8) | buf[1];
}
/*
 ** unpacki32() -- unpack a 32-bit int from a char buffer (like ntohl())
 */
uint32_t unpacki32(unsigned char *buf) {
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}
/*
** pack() -- store data dictated by the format string in the buffer
**
**  h - 16-bit              l - 32-bit
**  c - 8-bit char          f - float, 32-bit
**  s - string (16-bit length is automatically prepended)
*/
int32_t pack(unsigned char *buf, char *format, ...)
{
    va_list ap;
    int16_t h;
    int32_t l;
    int8_t c;
    float32_t f;
    char *s;
    int32_t size = 0, len;

    va_start(ap, format);

    for(; *format != '\0'; format++) {
        switch(*format) {
        case 'h': // 16-bit
            size += 2;
            h = (int16_t)va_arg(ap, int); // promoted
            packi16(buf, h);
            buf += 2;
            break;

        case 'l': // 32-bit
            size += 4;
            l = va_arg(ap, int32_t);
            packi32(buf, l);
            buf += 4;
            break;

        case 'c': // 8-bit
            size += 1;
            c = (int8_t)va_arg(ap, int); // promoted
            *buf++ = (c>>0)&0xff;
            break;

        case 'f': // float
            size += 4;
            f = (float32_t)va_arg(ap, double); // promoted
            l = pack754_32(f); // convert to IEEE 754
            packi32(buf, l);
            buf += 4;
            break;

        case 's': // string
            s = va_arg(ap, char*);
            len = strlen(s);
            size += len + 2;
            packi16(buf, len);
            buf += 2;
            memcpy(buf, s, len);
            buf += len;
            break;
        }
    }

    va_end(ap);

    return size;
}

/*
** unpack() -- unpack data dictated by the format string into the buffer
*/
void unpack(unsigned char *buf, char *format, ...)
{
    va_list ap;
    int16_t *h;
    int32_t *l;
    int32_t pf;
    int8_t *c;
    float32_t *f;
    char *s;
    int32_t len, count, maxstrlen=0;

    va_start(ap, format);

    for(; *format != '\0'; format++) {
        switch(*format) {
        case 'h': // 16-bit
            h = va_arg(ap, int16_t*);
            *h = unpacki16(buf);
            buf += 2;
            break;

        case 'l': // 32-bit
            l = va_arg(ap, int32_t*);
            *l = unpacki32(buf);
            buf += 4;
            break;

        case 'c': // 8-bit
            c = va_arg(ap, int8_t*);
            *c = *buf++;
            break;

        case 'f': // float
            f = va_arg(ap, float32_t*);
            pf = unpacki32(buf);
            buf += 4;
            *f = unpack754_32(pf);
            break;

        case 's': // string
            s = va_arg(ap, char*);
            len = unpacki16(buf);
            buf += 2;
            if (maxstrlen > 0 && len > maxstrlen)
            	count = maxstrlen - 1;
            else
            	count = len;
            memcpy(s, buf, count);
            s[count] = '\0';
            buf += len;
            break;

        default:
            if (isdigit(*format)) { // track max str len
                maxstrlen = maxstrlen * 10 + (*format-'0');
            }
            break;
        }

        if (!isdigit(*format)) maxstrlen = 0;
    }

    va_end(ap);
}
