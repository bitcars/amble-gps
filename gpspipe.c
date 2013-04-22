/*
 * gpspipe
 *
 * a simple program to connect to a gpsd daemon and dump the received data
 * to stdout
 *
 * This will dump the raw NMEA from gpsd to stdout
 *      gpspipe -r
 *
 * This will dump the super-raw data (gps binary) from gpsd to stdout
 *      gpspipe -R
 *
 * This will dump the GPSD sentences from gpsd to stdout
 *      gpspipe -w
 *
 * This will dump the GPSD and the NMEA sentences from gpsd to stdout
 *      gpspipe -wr
 *
 * Original code by: Gary E. Miller <gem@rellim.com>.  Cleanup by ESR.
 *
 * This file is Copyright (c) 2010 by the GPSD project
 * BSD terms apply: see the file COPYING in the distribution root for details.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#ifndef S_SPLINT_S
#include <unistd.h>
#endif /* S_SPLINT_S */

#include "gpsd.h"
#include "gpsdclient.h"
#include "revision.h"
#include "global.h"

static struct gps_data_t gpsdata;
static void spinner(unsigned int, unsigned int);

/* NMEA-0183 standard baud rate */
#define BAUDRATE B4800

/* Serial port variables */
static struct termios oldtio, newtio;
static int fd_out = 1;		/* output initially goes to standard output */
static char serbuf[MAX_MSG];
static int debug;

#include <yajl/yajl_tree.h>

/*
 * Parse the json package
 * @params:
 * buffer	:	pointer to a null terminated string
 * data		:	pointer to a structure for GPS data
 */
static int parse_gps_json(const char * buffer, struct gps_package * data) {
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
    if (node != NULL)
    {
        const char * lat_path[] = { "lat", (const char *) 0 };
        const char * lon_path[] = { "lon", (const char *) 0 };
        yajl_val lat = yajl_tree_get(node, lat_path, yajl_t_number);
        yajl_val lon = yajl_tree_get(node, lon_path, yajl_t_number);
        if (lat) data->lat = YAJL_GET_DOUBLE(lat);
        else return ERROR;
        if (lon) data->lon = YAJL_GET_DOUBLE(lon);
        else return ERROR;
    }
    else
    	return ERROR;

    yajl_tree_free(node);

    return SUCCESS;
}

static bool SendGPSPackage(int fd, const void *buf, size_t n, uint8_t flag) {
	/* send gps package */
    uint8_t header = DELIMITER_BYTE;
	if (write(fd, &header, sizeof(uint8_t)) == -1) {
		fprintf(stderr, "gpspipe: Socket write Error, %s(%d)\n", strerror(errno), errno);
		return false;
	}
	if (write(fd, &flag, sizeof(uint8_t)) == -1) {
		fprintf(stderr, "gpspipe: Socket write Error, %s(%d)\n", strerror(errno), errno);
		return false;
	}
	char * data = (char *) buf;
	unsigned i;
	for (i = 0; i<n; i++) {
		if (write(fd, data++, 1) == -1) {
			fprintf(stderr, "gpspipe: Socket write Error, %s(%d)\n", strerror(errno), errno);
			return false;
		}
	}
	CheckSum_t checksum = ChecksumCalculator(buf, n);
	checksum = htonl(checksum);
	if (write(fd, &checksum, sizeof(CheckSum_t)) == -1) {
		fprintf(stderr, "gpspipe: Socket write Error, %s(%d)\n", strerror(errno), errno);
		return false;
	}

	return true;
}


static void open_serial(char *device)
/* open the serial port and set it up */
{
    /* 
     * Open modem device for reading and writing and not as controlling
     * tty.
     */
    if ((fd_out = open(device, O_RDWR | O_NOCTTY)) == -1) {
	fprintf(stderr, "gpspipe: error opening serial port\n");
	exit(1);
    }

    /* Save current serial port settings for later */
    if (tcgetattr(fd_out, &oldtio) != 0) {
	fprintf(stderr, "gpspipe: error reading serial port settings\n");
	exit(1);
    }

    /* Clear struct for new port settings. */
    /*@i@*/ bzero(&newtio, sizeof(newtio));

    /* make it raw */
    (void)cfmakeraw(&newtio);
    /* set speed */
    /*@i@*/ (void)cfsetospeed(&newtio, BAUDRATE);

    /* Clear the modem line and activate the settings for the port. */
    (void)tcflush(fd_out, TCIFLUSH);
    if (tcsetattr(fd_out, TCSANOW, &newtio) != 0) {
	(void)fprintf(stderr, "gpspipe: error configuring serial port\n");
	exit(1);
    }
}

static void usage(void)
{
    (void)fprintf(stderr,
		  "Usage: gpspipe [OPTIONS] [server[:port[:device]]]\n\n"
		  "-d Run as a daemon.\n" 
		  "-o [file] Write output to file.\n"
		  "-h Show this help.\n" 
		  "-r Dump raw NMEA.\n"
		  "-R Dump super-raw mode (GPS binary).\n"
		  "-w Dump gpsd native data.\n"
		  "-l Sleep for ten seconds before connecting to gpsd.\n"
		  "-t Time stamp the data.\n"
		  "-T [format] set the timestamp format (strftime(3)-like; implies '-t')\n"
		  "-s [serial dev] emulate a 4800bps NMEA GPS on serial port (use with '-r').\n"
		  "-n [count] exit after count packets.\n"
		  "-v Print a little spinner.\n"
		  "-p Include profiling info in the JSON.\n"
		  "-V Print version and exit.\n\n"
		  "You must specify one, or more, of -r, -R, or -w\n"
		  "You must use -o if you use -d.\n");
}

/*@ -compdestroy @*/
int main(int argc, char **argv) {
	char buf[4096]; /* Yiding: buffer to put the gps package from gpsd */
	bool timestamp = false; /* Yiding: set the time stamp */
	char *format = "%c";
	char tmstr[200]; /* Yiding: time string */
	bool daemonize = false; /* Yiding: allow the client to run as a daemon */
	bool binary = false;
	bool sleepy = false;
	bool new_line = true;
	bool raw = false;
	bool watch = false;
	bool profile = false;
	bool usesocket = false;
	bool connectionAlive = false;
	long count = -1;
	int option;
	unsigned int vflag = 0, l = 0;
	FILE *fp;
	unsigned int flags;
	fd_set fds;
	struct hostent * host = NULL;
	struct sockaddr_in local_addr, serv_addr;
	int client = -1;
	int rc = -1;

	struct fixsource_t source;
	char *serialport = NULL;
	char *outfile = NULL;
	char *serverName = NULL;
	struct gps_package gpsPackage;

	/*@-branchstate@*/
	flags = WATCH_ENABLE;
	while ((option = getopt(argc, argv, "?dD:lhrRwtT:vVn:s:o:pS:")) != -1) {
		switch (option) {
		case 'S':
			usesocket = true;
			serverName = optarg;
			break;
		case 'D':
			debug = atoi(optarg);
#ifdef CLIENTDEBUG_ENABLE
			gps_enable_debug(debug, stderr);
#endif /* CLIENTDEBUG_ENABLE */
			break;
		case 'n':
			count = strtol(optarg, 0, 0);
			break;
		case 'r':
			raw = true;
			/*
			 * Yes, -r invokes NMEA mode rather than proper raw mode.
			 * This emulates the behavior under the old protocol.
			 */
			flags |= WATCH_NMEA;
			break;
		case 'R':
			flags |= WATCH_RAW;
			binary = true;
			break;
		case 'd':
			daemonize = true;
			break;
		case 'l':
			sleepy = true;
			break;
		case 't':
			timestamp = true;
			break;
		case 'T':
			timestamp = true;
			format = optarg;
			break;
		case 'v':
			vflag++;
			break;
		case 'w':
			flags |= WATCH_JSON;
			watch = true;
			break;
		case 'p':
			profile = true;
			break;
		case 'V':
			(void) fprintf(stderr, "%s: %s (revision %s)\n", argv[0], VERSION,
					REVISION);
			exit(0);
		case 's':
			serialport = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case '?':
		case 'h':
		default:
			usage();
			exit(1);
		}
	}
	/*@+branchstate@*/

	/* Grok the server, port, and device. */
	if (optind < argc) {
		gpsd_source_spec(argv[optind], &source);
	} else
		gpsd_source_spec(NULL, &source);

	if (serialport != NULL && !raw) {
		(void) fprintf(stderr, "gpspipe: use of '-s' requires '-r'.\n");
		exit(1);
	}

	if (outfile == NULL && daemonize) {
		(void) fprintf(stderr, "gpspipe: use of '-d' requires '-f'.\n");
		exit(1);
	}

	if (!raw && !watch && !binary) {
		(void) fprintf(stderr,
				"gpspipe: one of '-R', '-r', or '-w' is required.\n");
		exit(1);
	}

	/* check if server address is reachable */
	if (usesocket) {
		/* get host address from specified server name */
		host = gethostbyname(serverName);

		if (host == NULL ) {
			printf("%s: unknown host '%s'\n", argv[0], serverName);
			exit(-1);
		}

		/* now fill in sockaddr_in for remote address */
		serv_addr.sin_family = host->h_addrtype;
		/* get first address in host, copy to serv_addr */
		memcpy((char *) &serv_addr.sin_addr.s_addr, host->h_addr_list[0],
				host->h_length);
		serv_addr.sin_port = htons(SERVER_PORT);
		memset(serv_addr.sin_zero, 0, 8);

		/* create local stream socket */
		client = socket(PF_INET, SOCK_STREAM, 0);
		if (client < 0) {
			perror("cannot open socket ");
			exit(-1);
		}

		/* bind local socket to any port number */
		local_addr.sin_family = AF_INET;
		local_addr.sin_addr.s_addr = htonl(INADDR_ANY );
		local_addr.sin_port = htons(0);
		memset(local_addr.sin_zero, 0, 8);

		rc = bind(client, (struct sockaddr *) &local_addr, sizeof(local_addr));

		if (rc < 0) {
			printf("%s: cannot bind port TCP %u\n", argv[0], SERVER_PORT);
			perror("error ");
			exit(1);
		}
	}

	/* Daemonize if the user requested it. */
	/*@ -unrecog @*/
	if (daemonize)
		if (daemon(0, 0) != 0)
			(void) fprintf(stderr, "gpspipe: demonization failed: %s\n",
					strerror(errno));
	/*@ +unrecog @*/

	/* Sleep for ten seconds if the user requested it. */
	if (sleepy)
		(void) sleep(10);

	/* Open the output file if the user requested it.  If the user
	 * requested '-R', we use the 'b' flag in fopen() to "do the right
	 * thing" in non-linux/unix OSes. */
	if (outfile == NULL ) {
		fp = stdout;
	} else {
		if (binary)
			fp = fopen(outfile, "wb");
		else
			fp = fopen(outfile, "w");

		if (fp == NULL ) {
			(void) fprintf(stderr, "gpspipe: unable to open output file:  %s\n",
					outfile);
			exit(1);
		}
	}

	/* Open the serial port and set it up. */
	if (serialport)
		open_serial(serialport);

	/*@ -nullpass -onlytrans @*/
	if (gps_open(source.server, source.port, &gpsdata) != 0) {
		(void) fprintf(stderr,
				"gpspipe: could not connect to gpsd %s:%s, %s(%d)\n",
				source.server, source.port, strerror(errno), errno);
		exit(1);
	}
	/*@ +nullpass +onlytrans @*/

	if (profile)
		flags |= WATCH_TIMING;
	if (source.device != NULL )
		flags |= WATCH_DEVICE;
	(void) gps_stream(&gpsdata, flags, source.device);

	if ((isatty(STDERR_FILENO) == 0) || daemonize)
		vflag = 0;

	for (;;) {
		int r = 0;
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 100000;
		FD_ZERO(&fds);
		FD_SET(gpsdata.gps_fd, &fds);
		errno = 0;
		r = select(gpsdata.gps_fd + 1, &fds, NULL, NULL, &tv);
		if (r == -1 && errno != EINTR) {
			(void) fprintf(stderr, "gpspipe: select error %s(%d)\n",
					strerror(errno), errno);
			exit(1);
		} else if (r == 0)
			continue;

		if (vflag)
			spinner(vflag, l++);

		/* reading directly from the socket avoids decode overhead */errno = 0;
		r = (int) read(gpsdata.gps_fd, buf, sizeof(buf));
		if (r > 0) {
			int i = 0;
			int j = 0;
			for (i = 0; i < r; i++) {
				char c = buf[i];
				if (j < (int) (sizeof(serbuf) - 1)) {
					serbuf[j++] = c;
				}
				if (new_line && timestamp) {
					time_t now = time(NULL );

					struct tm *tmp_now = localtime(&now);
					(void) strftime(tmstr, sizeof(tmstr), format, tmp_now);
					new_line = 0;
					if (fprintf(fp, "%.24s :", tmstr) <= 0) {
						(void) fprintf(stderr, "gpspipe: write error, %s(%d)\n",
								strerror(errno), errno);
						exit(1);
					}
				}
				if (fputc(c, fp) == EOF) {
					fprintf(stderr, "gpspipe: Write Error, %s(%d)\n",
							strerror(errno), errno);
					exit(1);
				}

				if (c == '\n') {
					/* We have received a complete package */
					if (usesocket) {

						/* connect to server */
						if (!connectionAlive) {
							rc = connect(client, (struct sockaddr *) &serv_addr,
									sizeof(serv_addr));
							if (rc < 0) {
								perror("cannot connect ");
								connectionAlive = false;
							} else {
								connectionAlive = true;
							}
						}

						if (connectionAlive) {
							/* parse the package with json */
							/* null end the string */
							if (j < (int) (sizeof(serbuf) - 1)) {
								serbuf[j] = '\0';

								rc = parse_gps_json(serbuf, &gpsPackage);
								if (rc == SUCCESS) {
									connectionAlive = SendGPSPackage(client,
											&gpsPackage,
											(size_t) sizeof(struct gps_package),
											GPS_BYTE);
								} else {
									/* send no data */
									//SendGPSPackage(client, &gpsPackage, (size_t) sizeof(struct gps_package));
								}

								if (!connectionAlive) {
									printf("What the heck? The server is messed up?\n");
									/* create local stream socket */
									client = socket(PF_INET, SOCK_STREAM, 0);
									if (client < 0) {
										perror("cannot open socket ");
										exit(-1);
									}

									/* bind local socket to any port number */
									local_addr.sin_family = AF_INET;
									local_addr.sin_addr.s_addr = htonl(
											INADDR_ANY );
									local_addr.sin_port = htons(0);
									memset(local_addr.sin_zero, 0, 8);

									rc = bind(client,
											(struct sockaddr *) &local_addr,
											sizeof(local_addr));

									if (rc < 0) {
										printf("%s: cannot bind port TCP %u\n",
												argv[0], SERVER_PORT);
										perror("error ");
										exit(1);
									}
								}
							} else {
								fprintf(stderr, "gpspipe: buffer overflow");
							}
						}
					}

					if (serialport != NULL ) {
						if (write(fd_out, serbuf, (size_t) j) == -1) {
							fprintf(stderr,
									"gpspipe: Serial port write Error, %s(%d)\n",
									strerror(errno), errno);
							exit(1);
						}
					}

					j = 0;

					new_line = true;
					/* flush after every good line */
					if (fflush(fp)) {
						(void) fprintf(stderr,
								"gpspipe: fflush Error, %s(%d)\n",
								strerror(errno), errno);
						exit(1);
					}
					if (count > 0) {
						if (0 >= --count) {
							/* completed count */
							exit(0);
						}
					}
				} /* c == '\n' */
			} /* for i */
		} else {
			if (r == -1) {
				if (errno == EAGAIN)
					continue;
				else
					(void) fprintf(stderr, "gpspipe: read error %s(%d)\n",
							strerror(errno), errno);
				exit(1);
			} else {
				exit(0);
			}
		}
	} /* infinite loop */

#ifdef __UNUSED__
	if (serialport != NULL) {
		/* Restore the old serial port settings. */
		if (tcsetattr(fd_out, TCSANOW, &oldtio) != 0) {
			(void)fprintf(stderr, "Error restoring serial port settings\n");
			exit(1);
		}
	}
#endif /* __UNUSED__ */

	exit(0);
}

/*@ +compdestroy @*/

static void spinner(unsigned int v, unsigned int num)
{
    char *spin = "|/-\\";

    (void)fprintf(stderr, "\010%c", spin[(num / (1 << (v - 1))) % 4]);
    (void)fflush(stderr);
    return;
}
