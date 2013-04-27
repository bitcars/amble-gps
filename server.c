#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> 
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>

#include "server.h"

#define SUCCESS 0
#define ERROR   1


/* auxillary functions to read newline-terminated strings from a file/socket */
int readnf (int, char *);
int readline(int, char *, int);

int ReadGPSPackage(int fd, struct gps_package * gpkg);

int server;         /* listening socket descriptor */


/**
 * cleanup() is called to kill the thread upon SIGINT. 
**/
void cleanup()
{
    close(server);
    pthread_exit(NULL);
    return;
} /* cleanup() */


#include <yajl/yajl_tree.h>

/* 
 * Parse the json package
 */
int parse_gps_json(const char * buffer, struct gps_package * data) {
    yajl_val node;
    char errbuf[1024];

    /* null plug buffers */
    errbuf[0] = 0;

    /* we have the whole config file in memory.  let's parse it ... */
    node = yajl_tree_parse((const char *) buffer, errbuf, sizeof(errbuf));

    /* parse error handling */
    /*if (node == NULL) {
        fprintf(stderr, "parse_error: ");
        if (strlen(errbuf)) fprintf(stderr, " %s", errbuf);
        else fprintf(stderr, "unknown error");
        fprintf(stderr, "\n");
        return 0;
    }*/

    /* ... and extract a nested value from the json file */
    {
        const char * lat_path[] = { "lat", (const char *) 0 };
        const char * lon_path[] = { "lon", (const char *) 0 };
        yajl_val lat = yajl_tree_get(node, lat_path, yajl_t_number);
	yajl_val lon = yajl_tree_get(node, lon_path, yajl_t_number);
        if (lat) data->lat = YAJL_GET_DOUBLE(lat);
        else return 0;
        if (lon) data->lon = YAJL_GET_DOUBLE(lon);
        else return 0;
    }

    yajl_tree_free(node);

    return 1;
}

static int C=0;

/**
 * Thread handler for incoming connections...
**/
void handler(AmbleClientInfo * clientInfo) {
    char line[MAX_MSG];
    int client_local;   /* keep a local copy of the client's socket descriptor */
    FILE *fp;
     
    char outfile[50];
    sprintf(outfile, "Connection%03d.txt", C++);

    fp = fopen(outfile, "w");
    client_local = clientInfo->remotefd; /* store client socket descriptor */
    
    /* reset line */
    memset(line, 0, MAX_MSG);
    
    struct gps_package gps;
    while(ReadGPSPackage(client_local, &gps)!= ERROR) {
        fprintf(fp, "lat:%f log:%f\n", gps.lat, gps.lon);
    }
    
    fclose(fp);
    serverHangup(clientInfo);
    return;
} /* handler() */

///* main function */
//int main (int argc, char *argv[])
//{
//    int client;         /* client socket descriptor */
//    int addr_len;       /* used to store length (size) of sockaddr_in */
//    pthread_t thread;   /* thread variable */
//
//    struct sockaddr_in cliAddr;   /* socket address for client */
//    struct sockaddr_in servAddr;  /* socket address for server */
//
//    signal(SIGINT, cleanup);      /* now handle SIGTERM and SIGINT */
//    signal(SIGTERM, cleanup);
//
//    /* now create the server socket
//       make it an IPV4 socket (PF_INET) and stream socket (TCP)
//       and 0 to select default protocol type */
//    server = socket(PF_INET, SOCK_STREAM, 0);
//    if (server < 0) {
//        perror("cannot open socket ");
//        return ERROR;
//    }
//
//    /* now fill in values of the server sockaddr_in struct
//       s_addr and sin_port are in Network Byte Order (Big Endian)
//       Since Intel CPUs use Host Byte Order (Little Endian), conversion
//       is necessary (e.g. htons(), and htonl() */
//    servAddr.sin_family = AF_INET;  /* again ipv4 */
//    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* local address */
//    servAddr.sin_port = htons(SERVER_PORT);
//    memset(servAddr.sin_zero, 0, 8);
//
//    /* now bind server port
//       associate socket (server) with IP address:port (servAddr) */
//    if (bind(server, (struct sockaddr *) &servAddr, sizeof(struct sockaddr)) < 0) {
//        perror("cannot bind port ");
//        return ERROR;
//    }
//
//    /* wait for connection from client with a pending queue of size 5 */
//    listen(server, 5);
//
//    while(1) /* infinite loop */
//    {
//        printf("%s: waiting for new connection on port TCP %u\n", argv[0], SERVER_PORT);
//        addr_len = sizeof(cliAddr);
//
//        /* new socket for client connection
//           accept() will block until a connection is present
//           accept will return a NEW socket for the incoming connection
//           server socket will continue listening
//           store client address in cliAddr */
//        client = accept(server, (struct sockaddr *) &cliAddr, &addr_len);
//        if (client < 0) {
//            perror("cannot accept connection ");
//            break;
//        }
//        pthread_create(&thread, 0, (void*)&handler, (void*) &client);
//    } /* while (1) */
//
//    close(server);
//    exit(0);
//} /* main() */

/* A not so elegant implementation that constantly checks if a
 * new connection is coming
 * TODO: we should use select instead
 */
int serverRings(AmbleClientInfo ** pClientInfoPtr) {
	char s[INET6_ADDRSTRLEN];
	struct sockaddr_storage remoteAddr;
	int remotefd;

	/* non-blocking accept */
	socklen_t sin_size = sizeof (remoteAddr);
	remotefd = accept(server, (struct sockaddr *)&remoteAddr, &sin_size);

	if (remotefd != -1) {
		/* allocate an AmbleClient */
		*pClientInfoPtr = (AmbleClientInfo *) malloc(sizeof(AmbleClientInfo));
		(*pClientInfoPtr)->remotefd = remotefd;
		(*pClientInfoPtr)->remoteAddr = remoteAddr;

		inet_ntop(remoteAddr.ss_family,
				get_in_addr((struct sockaddr *)&remoteAddr),
				s, sizeof s);
		printf("server: got connection from %s\n", s);
		return 1;
	}
	else {
		*pClientInfoPtr = NULL;
		return 0;
	}
}

/* Clean up after the connection is broken */
void serverHangup(AmbleClientInfo * pWorker) {
	close(pWorker->remotefd);

	free(pWorker);
}

/*
 * Initialize the server's listening port
 */
void serverOnLine(void) {
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
	if ((rv = getaddrinfo(NULL, SERVER_PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}
	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL ; p = p->ai_next) {
		if ((server = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

	    /* set listening socket as non-blocking */
	    if (fcntl(server, F_SETFL, O_NONBLOCK) < 0 ) {
	    	perror("set non-blocking socket\n" );
			continue;
		}

		if (bind(server, p->ai_addr, p->ai_addrlen) == -1) {
			close(server);
			perror("listener: bind");
			continue;
		}
		break;
	}
	if (p == NULL ) {
		fprintf(stderr, "listener: failed to bind socket\n");
		exit(1);
	}
	freeaddrinfo(servinfo);

    /* wait for connection from client with a pending queue of size 5 */
	if (listen(server, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

}

/*
 * Let server go off-line
 */
void serverOffLine(void) {
	close(server);
}


/** 
 * readnf() - reading from a file descriptor but a bit smarter 
**/
int readnf (int fd, char *line)
{
    if (readline(fd, line, MAX_MSG) < 0)
        return ERROR; 
    return SUCCESS;
}
  
/**
 * readline() - read an entire line from a file descriptor until a newline.
 * functions returns the number of characters read but not including the
 * null character.
**/
int readline(int fd, char *str, int maxlen) 
{
  int n;           /* no. of chars */  
  int readcount;   /* no. characters read */
  char c;

  for (n = 1; n < maxlen; n++) {
    /* read 1 character at a time */
    readcount = read(fd, &c, 1); /* store result in readcount */
    if (readcount == 1) /* 1 char read? */
    {
      *str = c;      /* copy character to buffer */
      str++;         /* increment buffer index */         
      if (c == '\n') /* is it a newline character? */
         break;      /* then exit for loop */
    } 
    else if (readcount == 0) /* EOF */
    {
        break;
    }
    else 
      return (-1); /* error in read() */
  }
  *str='\0';       /* null-terminate the buffer */
  return (n);   /* return number of characters read */
} /* readline() */


typedef enum {
	INIT, TYPE, NOFIXFOUND, GPSFOUND, CHKSUM, NOFIXDOCODE, GPSDECODE
} State_t;

/**
 * ReadGPSPackage()
**/
int ReadGPSPackage(int fd, struct gps_package * gpkg) {
	int readcount; /* no. characters read */
	unsigned char buff[MAX_MSG];
	struct gps_package * marker = NULL;
	int foundType;
	CheckSum_t checksum;
	State_t state = INIT;

	readcount = (int)read(fd, buff, sizeof(buff));
	printf("readcound %d\n", readcount);
	if (readcount == -1)
		return -1;
	else if (readcount == 0)
		return -1;
	else {
		int i = 0;
		while( i < readcount ) {
			switch (state) {
			int n,rc;
			case INIT:
				printf("INIT 0x%X\n",buff[i]);

				if (buff[i] == DELIMITER_BYTE)
					state = TYPE;
				++i;
				if (i + 13 > readcount) {
					for (n=readcount; n<i+13; n++) {
						rc = (int)read(fd, &buff[n], 1);
						if (rc == -1)
							return -1;
					}
					readcount=i+13;
					printf("new readcount %d\n", readcount);
				}
				break;
			case TYPE:
				printf("TYPE 0x%X\n",buff[i]);
				if (buff[i] == NOFIX_BYTE) {
					state = NOFIXFOUND;
					foundType = NOFIXFOUND;
				}
				else if (buff[i] == GPS_BYTE) {
					state = GPSFOUND;
					foundType = GPSFOUND;
				}
				else
					state = INIT;
				++i;
				break;
			case NOFIXFOUND:
				state = INIT;
				break;
			case GPSFOUND:
				printf("GPSFOUND 0x%X\n",buff[i]);
				if ((i + (int)sizeof(struct gps_package)) < readcount) {
					marker = (struct gps_package *)(buff+i);
					state = CHKSUM;
					i += sizeof(struct gps_package);
				}
				else {
					break;
				}
				break;
			case CHKSUM:
				printf("CHKSUM 0x%X\n",buff[i]);
				checksum = (* (CheckSum_t *)(buff+i));
				checksum = ntohl(checksum);

				if (checksum == ChecksumCalculator((void *)marker, sizeof(struct gps_package))) {
					if (foundType == NOFIXFOUND)
						state = NOFIXDOCODE;
					else
						state = GPSDECODE;
					printf("received checksum %u\n", checksum);
					printf("marker %f, %f\n", marker->lat, marker->lon);
				}
				else
					state = INIT;

				break;
			case NOFIXDOCODE:
				state = INIT;
				break;
			case GPSDECODE:
				printf("GPSDECODE 0x%X\n",buff[i]);
				gpkg->lat = marker->lat;
				gpkg->lon = marker->lon;
				state = INIT;
				i += sizeof(CheckSum_t);
				break;
			default:
				fprintf(stderr, "State machine error\n");
				exit(-1);
			}

		}
		return readcount;
	}

} /* readline() */
