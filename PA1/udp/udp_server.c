#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 100

int main (int argc, char * argv[] )
{

        char *command = malloc(100*(sizeof(char)));
	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	//if ((sock = **** CALL SOCKET() HERE TO CREATE UDP SOCKET ****) < 0)
        if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		printf("unable to create socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);

	//waits for an incoming message
	bzero(buffer,sizeof(buffer));
	//nbytes = nbytes = **** CALL RECVFROM() HERE ****;
        while(1)
        {
        nbytes = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);

	printf("The client says %s\n", buffer);
        
	if(strstr(buffer,"get") != NULL)
        {
        command = strtok(buffer," ");
        printf("%s\n",command);
        if(command != NULL)
        {
        command = strtok(NULL," ");
        printf("%s",command);
        }
        command[4] = NULL;
        FILE* filein;
        char data[500];
        filein = fopen(command,"r");
        if(filein == NULL)
        {
        printf("file can't be opened\n");
        }  
        while(fgets(data,500,(FILE*)filein))
        {
        printf("%s",data);
        nbytes = sendto(sock,data,strlen(data),0,(struct sockaddr *)&remote,remote_length);
        bzero(data,sizeof(data));
        }
        fclose(filein);
        }
        }
	close(sock);
}

