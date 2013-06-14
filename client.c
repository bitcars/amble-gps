#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>

#include "client.h"

int clientCall(char * serverName, comSender * sender) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	//char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	int errcnt = 1;
	fprintf(stderr, "attempt #%d to connect %s : ", errcnt, serverName);
	while ((rv = getaddrinfo(serverName, SERVER_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "failed\n");
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		//sleep(2);
		if (errcnt == CALL_TRIES) {
			return -1;
		}
		errcnt++;
		fprintf(stderr, "attempt #%d to connect %s : ", errcnt, serverName);
	}
	fprintf(stderr, "success\n");

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL ; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			//perror("client: socket");
			continue;
		}
		break;
	}

	if (p == NULL ) {
		fprintf(stderr, "client: failed to connect %s:%s\n", serverName, SERVER_PORT);
		return -1;
	}

	/* save the connection info in sender object */
	sender->recvAddr = *(p->ai_addr);
	sender->addrlen = p->ai_addrlen;

	sender->sockfd = sockfd;

	char str[] = "lalala";
	sendto(sockfd, str, strlen(str), 0, p->ai_addr, p->ai_addrlen);

	/* we are now done with this structure */
    freeaddrinfo(servinfo);

	return sockfd;
}

static int clientfd;

void *Listen(void *threadId) {
	long tid;
	tid = (long)threadId;
	char buf1[MAX_MSG];
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	Printf("Listener thread %d starts\n", tid);

    fd_set readfds;
	while (1) {
	    FD_ZERO(&readfds);
	    FD_SET(clientfd, &readfds);

	    // don't care about writefds and exceptfds:
	    select(clientfd+1, &readfds, NULL, NULL, &tv);

	    if (FD_ISSET(clientfd, &readfds)) {
	        printf("A key was pressed!\n");
	        recv(clientfd, buf1, sizeof buf1, 0);
	    }
	    else
	        printf("Timed out.\n");

	    return 0;
	}

	pthread_exit(NULL);
}

pthread_t listener;

/*
 * Initialize the client's listening port
 */
static void clientOnLine(void) {
	struct addrinfo hints, *clientinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo(NULL, CLIENT_PORT, &hints, &clientinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
	}
	// loop through all the results and bind to the first we can
	int on=1;
	for (p = clientinfo; p != NULL ; p = p->ai_next) {
		if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (setsockopt(clientfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
			perror("set enable address reuse");
			continue;
		}

		if (bind(clientfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(clientfd);
			perror("listener: bind");
			continue;
		}
		break;
	}
	if (p == NULL ) {
		fprintf(stderr, "listener: failed to bind socket\n");
	}
	freeaddrinfo(clientinfo);
}

void startIt(void) {
	// start the listening port
	clientOnLine();

	// start the listening thread
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	int rc;
	long t = 0;

	rc = pthread_create(&listener, &attr, Listen, (void *) t);
	if (rc) {
		Printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}

	pthread_attr_destroy(&attr);
}

void endIt(void) {
	int rc;
	void *status;

	// stop the listener thread

	// join the thread
    rc = pthread_join(listener, &status);

    if (rc) {
       Printf("ERROR; return code from pthread_join() is %d\n", rc);
       exit(-1);
    }
}


void sendGPSPackage(comSender* sender, struct gps_package* gps, uint8_t flag) {
	/* package GPS data */
	comPackage* package = newComPackage();
	unsigned char buf[MAX_MSG];
	PACK_GPS(buf, gps);
	comPackData(package, (void *) buf, GPS_PACKAGE_SIZE_BYTE);

	/* send out data package */
	comSendData(sender, package);

	/* clean up */
	free(package);
}



