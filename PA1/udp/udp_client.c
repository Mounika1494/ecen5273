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
#include <errno.h>

#define MAXBUFSIZE 30000

/* You will have to modify the program below */

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
        char *user_input = malloc(25*(sizeof(char)));
	struct sockaddr_in remote;              //"Internet socket address structure"

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet 
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		printf("unable to create socket");
	}
        printf("connected to the server\n");
        printf("select the command\n\n");
        printf("******************************\n");
        printf("get [file_name]\n\n");
        printf("put [file_name]\n\n");
        printf("delete [file_name]\n\n");
        printf("ls\n\n");
        printf("exit\n\n");
        printf("*********************************\n");
        fgets(user_input,30,stdin);
        printf("user_input at address %d has %c\n",user_input,*user_input);
        nbytes = sendto(sock,user_input,strlen(user_input),0,(struct sockaddr *)&remote,sizeof(remote));
             
                
	/******************
	  sendto() sends immediately.  
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
        unsigned int remote_length;
        remote_length = sizeof(remote);	
	//nbytes = **** CALL SENDTO() HERE ****;
        // nbytes = sendto(sock,command,strlen(command),0,(struct sockaddr*)&remote,sizeof(remote));
	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	int addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));
	//nbytes = **** CALL RECVFROM() HERE ****; 
        FILE *fp; 
        fp = fopen("foo1_client.txt","w+");
        if(fp == NULL)
        printf("file can't be opened\n");
        while(nbytes!=NULL)
        {
        nbytes = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
        printf("%d\n",nbytes);
	printf("%s", buffer);
        if(fwrite(buffer,1,sizeof(buffer),fp)<0)
        {
        printf("error writing file\n");
        }
        bzero(buffer,sizeof(buffer));
        }
        fclose(fp);
	close(sock);

}

