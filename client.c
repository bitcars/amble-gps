#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "client.h"


int clientCall(char * serverName) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	int errcnt = 0;
	while ((rv = getaddrinfo(serverName, SERVER_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		sleep(2);
		errcnt++;
		if (errcnt == CALL_TRIES) {
			return -1;
		}
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL ; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
				== -1) {
			//perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			//perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL ) {
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    fprintf(stderr, "client: connected to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

//
//	struct termios termios;
//
//	tcgetattr(sockfd, &termios);
//	termios.c_lflag &= ~ICANON; /* Set non-canonical mode */
//	termios.c_cc[VTIME] = 50; /* Set timeout of 10.0 seconds */
//	tcsetattr(sockfd, TCSANOW, &termios);

	return sockfd;

}


void *Listen(void *threadId) {
	long tid;
	tid = (long)threadId;

	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 100000;

	Printf("Listener thread %d starts\n", tid);

    fd_set readfds;
	while (1) {
	    FD_ZERO(&readfds);
	    FD_SET(fd, &readfds);

	    // don't care about writefds and exceptfds:
	    select(fd+1, &readfds, NULL, NULL, &tv);

	    if (FD_ISSET(STDIN, &readfds))
	        printf("A key was pressed!\n");
	    else
	        printf("Timed out.\n");

	    return 0;
	}
	}

	pthread_exit(NULL);
}

pthread_t listener;

void startIt(void) {
   pthread_attr_t attr;

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   int rc;
   long t = 0;

   rc = pthread_create(&listener, &attr, Listen, (void *)t);
   if (rc){
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

