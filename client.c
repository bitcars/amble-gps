#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h>
#include <stdlib.h>

#define SERVER_PORT 1600
#define MAX_MSG 256

/* main() */
int main (int argc, char *argv[]) {

    int client;  /* client socket */
    int rc;   
    struct sockaddr_in local_addr, serv_addr;
    struct hostent * host;
    char message[6] = {'/','T','I','M','E','\n'};
    char date[25];
    
    if(argc < 2) {
        printf("usage: %s <server>\n",argv[0]);
        exit(-1);
    }

    /* get host address from specified server name */
    host = gethostbyname(argv[1]);
  
    if (host == NULL) 
    {
        printf("%s: unknown host '%s'\n",argv[0],argv[1]);
        exit(-1);
    }

    /* now fill in sockaddr_in for remote address */
    serv_addr.sin_family = host->h_addrtype;
    /* get first address in host, copy to serv_addr */
    memcpy((char *) &serv_addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);
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
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(0);
    memset(local_addr.sin_zero, 0, 8);

    rc = bind(client, (struct sockaddr *) &local_addr, sizeof(local_addr));

    if (rc < 0) 
    {
        printf("%s: cannot bind port TCP %u\n",argv[0],SERVER_PORT);
        perror("error ");
        exit(1);
    }
    
    /* connect to server */
    rc = connect(client, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (rc < 0) 
    {
        perror("cannot connect ");
        exit(1);
    }

    /* now send /TIME */
    rc = send(client, message, strlen(message) + 1, 0);
 
    if (rc < 0) 
    {
        perror("cannot send data "); 
        close(client);
        exit(-1);
    }
  
    /* we're expecting 25 chars from server, */
        read(client,date,25);

    printf(date);
    
    close(client);	
    return 0;
  
} /* main() */

