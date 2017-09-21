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
#include <dirent.h>
#include <stdint.h>

/* You will have to modify the program below */

#define MAXBUFSIZE 30000
typedef enum 
{
  GET = 1,
  PUT = 2,
  DELETE = 3,
  LIST_FILES = 4,
  EXIT = 5
}commands;

char* itoa(int num,char *str)
{
   if(str == NULL)
   return NULL;
   sprintf(str,"%d",num);
   return str;
}

uint8_t check_ack(int sock,struct sockaddr_in remote,char *ack)
{
if(!(strcmp(ack,"ACK")))
{
printf("ack is recieved\n");
}

}
uint8_t command_decode(char *command)
{
      uint8_t cmd_recieved = 0;
      if(!(strcmp(command,"get")))
      {
      printf("client wants to get a file\n");
      cmd_recieved = GET;
      }
      if(!(strcmp(command,"put")))
      {
      printf("client wants to put a file\n");
      cmd_recieved = PUT;
      }
      if(!(strcmp(command,"delete")))
      {
      printf("client wants to delete a file\n");
      cmd_recieved = DELETE;
      }
     if(!(strcmp(command,"ls")))
      {
      printf("client wants list of files\n");
      cmd_recieved = LIST_FILES;
      }
     if(!(strcmp(command,"exit")))
     {
      printf("client wants to stop the server\n");
      cmd_recieved = EXIT;
     }
    return cmd_recieved;
}
size_t send_fileinfo(int sock,struct sockaddr_in remote,char *filename)
{
      FILE* fp = NULL;
      size_t size = 0;
      char* file_info = (char *)malloc(100*sizeof(char));
      fp = fopen(filename,"rb+");
      if(fp == NULL)
      {
      printf("file can't be opened\n");
      } 
      //get its size
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      rewind (fp);
      printf("size of file is %lu\n",size);
      if(itoa(size,file_info)!= NULL)
      printf("size in string is %s\n",file_info);
      //forming packet
      //strncpy(file_info,"00000000",8);
      //strcat(file_info,size);
      sendto(sock,file_info,strlen(file_info),0,(struct sockaddr *)&remote,sizeof(remote));
      return size;
  
}

int send_file(int sock,struct sockaddr_in remote,char *filename,size_t size)
{
      FILE* filein = NULL;
      int nbytes = 0;
      int read_bytes = 0;
      int packet_size = 1000;
      int nmemb = 0;
      char *data = malloc(packet_size * sizeof(char));
      if(data == NULL)
      {
      printf("could'nt allocate memory\n");
      }
      filein = fopen(filename,"rb+");
      if(filein == NULL)
      {
      printf("file can't be opened\n");
      }
      
      while(fread(data,1,packet_size,filein))
      {
      //printf("number of bytes read is %d\n",read_bytes);
      nbytes = sendto(sock,data,packet_size*sizeof(char),0,(struct sockaddr *)&remote,sizeof(remote));
      if(nbytes == 0)
      return 0;
      read_bytes=read_bytes+nbytes;
      printf("number of bytes sent is %d\n",read_bytes);
      for(int i=0;i<1000000;i++);
      }
      fclose(filein);
      return 1;

}

char* buffer_remove_index(char* file_info)
{
      char *index = malloc(8*sizeof(char));
      strncpy(file_info,index,8);
      return(file_info+8);
}

int main (int argc, char * argv[] )
{
        char *filename = malloc(100*(sizeof(char)));
        char *file_info = malloc(100*sizeof(char));
        char *command = malloc(10*(sizeof(char)));
        char *ack = malloc(10*sizeof(char));
        size_t size;
        uint8_t option = 0;
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

	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


        if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		printf("unable to create socket");
	}

	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);
        while(1)
        {
	//waits for an incoming message
        nbytes = recvfrom(sock,command,10*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
        printf("client says %s\n",command);
        option = command_decode(command);
        switch(option)
         {
           case GET: 
                    recvfrom(sock,filename,100*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
                    printf("file name is %s\n",filename);
                    size = send_fileinfo(sock,remote,filename);
                    if(send_file(sock,remote,filename,size))
                    {
                    printf("sent the file\n");
                    nbytes = recvfrom(sock,ack,10*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int*)&remote_length);
                    if(nbytes != 0)
                    check_ack(sock,remote,ack);
                    }
                    break;
         }
                    
      }  
        close(sock);
}


