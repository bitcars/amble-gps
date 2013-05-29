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
#include <stdarg.h>

#include "server.h"

#define SUCCESS 0
#define ERROR   1

#define DEBUG 0
static int Printf (const char * format, ...)
{
    va_list args;
    va_start(args, format);
    int ret = 0;

	if (DEBUG)
		ret = vprintf(format, args);

    va_end(args);

	return ret;
}

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


void outputKML(struct gps_package * gps, clientId cid)
{
	FILE * fpKML;
    char kmlfile[50];
    sprintf(kmlfile, "client-%d.kml", cid);
	fpKML = fopen(kmlfile, "w");

	int range, tilt, speed;
	speed = (int)(gps->speed * 2.2369356f);
	if (speed >= 10) {
		range = ((speed / 100) * 350) + 650;
		tilt = ((speed / 120) * 43) + 30;
	} else {
		range = 200;
		tilt = 30;
	}

	fprintf(fpKML, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(fpKML, "<kml xmlns=\"http://earth.google.com/kml/2.0\">\n");
	fprintf(fpKML, "  <Placemark>\n");
	fprintf(fpKML, "    <name>%d mph</name>\n", speed);
	fprintf(fpKML, "    <description>AmbleTour ClientId %d</description>\n", cid);
	fprintf(fpKML, "    <LookAt>\n");
	fprintf(fpKML, "      <longitude>%f</longitude>\n", gps->lon);
	fprintf(fpKML, "      <latitude>%f</latitude>\n", gps->lat);
	fprintf(fpKML, "      <range>%d</range>\n", range);
	fprintf(fpKML, "      <tilt>%d</tilt>\n", tilt);
	fprintf(fpKML, "      <heading>%f</heading>\n", gps->heading);
	fprintf(fpKML, "    </LookAt>\n");
	fprintf(fpKML, "    <Point>\n");
	fprintf(fpKML, "      <coordinates>%f,%f,%f</coordinates>\n", gps->lon, gps->lat, gps->alt);
	fprintf(fpKML, "    </Point>\n");
	fprintf(fpKML, "  </Placemark>\n");
	fprintf(fpKML, "</kml>\n");

	fflush(fpKML);
	fclose(fpKML);
}

/**
 * Thread handler for incoming connections...
**/
void handler(AmbleClientInfo * clientInfo) {
    int client_local;   /* keep a local copy of the client's socket descriptor */
    FILE * fp;
    struct gps_package gps;

    char outfile[50];
    sprintf(outfile, "client-%d.txt", clientInfo->cid);


    fp = fopen(outfile, "w");

    client_local = clientInfo->remotefd; /* store client socket descriptor */
    
    while(ReadGPSPackage(client_local, &gps)!= -1) {
    	outputKML(&gps, clientInfo->cid);
        fprintf(fp, "%f, %f\n", gps.lat, gps.lon);
		fflush(fp);
    }
    
    fclose(fp);
    serverHangup(clientInfo);
    return;
} /* handler() */

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
	int on=1;
	for (p = servinfo; p != NULL ; p = p->ai_next) {
		if ((server = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
            if (setsockopt( server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
perror("set enable address reuse");
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
	INIT, TYPE, NOFIXFOUND, GPSFOUND
} State_t;

/*                     GPS DATA              DELIMITER         TYPE         Offset */
#define BORDER (sizeof(struct gps_package) + sizeof(uint8_t) + sizeof(uint8_t) - 1)

/**
 * ReadGPSPackage()
**/
int ReadGPSPackage(int fd, struct gps_package * gpkg) {
	int readcount; /* no. characters read */
	unsigned char buff[MAX_MSG];
	struct gps_package * marker = NULL;
	int foundType;
	State_t state = INIT;
	uint8_t ack = 0x23;

	readcount = (int)read(fd, buff, sizeof(buff));
	Printf("readcound %d\n", readcount);
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
				Printf("INIT 0x%X\n",buff[i]);

				if (buff[i] == DELIMITER_BYTE)
					state = TYPE;
				++i;
				if (i+BORDER > readcount) {
					for (n=readcount; n<i+BORDER; n++) {
						rc = (int)read(fd, &buff[n], 1);
						if (rc == -1)
							return -1;
					}
					readcount=i+BORDER;
					/* send back an ack message */
					if (write(fd, "I got your message",18) == -1) {
						perror("write");
						return -1;
					}
					Printf("new readcount %d\n", readcount);
				}
				break;
			case TYPE:
				Printf("TYPE 0x%02X\n",buff[i]);
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
				Printf("GPSFOUND i=%d\n",i);
				if (foundType == GPSFOUND) {
					if ((i + (int)sizeof(struct gps_package)) <= readcount) {
						marker = (struct gps_package *)(buff+i);
						Printf("marker %f, %f\n", marker->lat, marker->lon);
						*gpkg = *marker;
						state = INIT;
						i += sizeof(struct gps_package);
					}
					else {
						printf("Fatal: GPS Package Parsing Error\n");
						return -1;
					}
				}
				else if (foundType == NOFIXFOUND){
					state = INIT;
					i = readcount;
				}
				else
					return -1;
				break;
			default:
				fprintf(stderr, "State machine error\n");
				exit(-1);
			}

		}
		return readcount;
	}

} /* readline() */
