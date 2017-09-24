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
      printf("command is %s",command);
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
      sendto(sock,file_info,strlen(file_info),0,(struct sockaddr *)&remote,sizeof(remote));
      return size;
  
}

char *decryption(char *data,int packet_size)
{
      char* key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      int j = 0;
      for(int i = 0;i<packet_size;i++)
      {

        *(data + i) = *(data+i) ^ *(key+j);
         j++;
         if(j==25)
         j=0;
      }
      return data;

}

char *encryption(char *data,int packet_size)
{
      char* key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      int j = 0;
      for(int i = 0;i<packet_size;i++)
      {

        *(data + i) = *(data+i) ^ *(key+j);
         j++;
         if(j==25)
         j=0;
      }
      return data;

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
      return 0;
      }
      
      while(fread(data,1,packet_size,filein))
      {
      int i=0;
      encryption(data,packet_size);
      //printf("number of bytes read is %d\n",read_bytes);
      while(i<2)
      {
      nbytes = sendto(sock,data,packet_size*sizeof(char),0,(struct sockaddr *)&remote,sizeof(remote));
      if(nbytes == packet_size)
      i++;
      }
      read_bytes=read_bytes+nbytes;
      printf("number of bytes sent is %d\n",read_bytes);
      for(int i=0;i<150000;i++);
      }
      fclose(filein);
      return 1;

}

int recv_file(int sock,struct sockaddr_in remote,char *file_name,size_t size)
{
      unsigned int remote_length = 0;
      remote_length = sizeof(remote);
      FILE *fp = NULL;
      int packet_size = 1000;
      int nmemb = 0;
      int nbytes = 0;
      size_t data_read = 0;
      int result = 0;
      char *file = malloc(size*(sizeof(char)));
      char *buffer = NULL;
      char *buffer1 = malloc(packet_size*(sizeof(char)));
      char *buffer2 = malloc(packet_size*(sizeof(char)));
      bzero(buffer1,packet_size*(sizeof(char)));
      bzero(buffer2,packet_size*(sizeof(char)));
      fp = fopen(file_name,"w+");
      printf("size is %lu\n",size);
      //recieve the data from server
      while(data_read<size)
      {
      recvfrom(sock,buffer1,packet_size*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int*)&remote_length);
      recvfrom(sock,buffer2,packet_size*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
      result = strcmp(buffer1,buffer2);
      if(result >= 0)
      {
      printf("both packets are recieved\n");
      buffer = buffer1;
      bzero(buffer2,packet_size*(sizeof(char)));
      }
      else if(result < 0)
      {
      buffer = buffer2;
      bzero(buffer1,packet_size*(sizeof(char)));
      }
      decryption(buffer,packet_size);
      memcpy(file+data_read,buffer,packet_size);
      bzero(buffer,packet_size*sizeof(char));
      data_read=data_read+packet_size;
      printf("bytes recieved is %d\n",data_read);
      }
      printf("size of file is %d\n",strlen(file));
      if(fp == NULL)
      return 0;
      if(fwrite(file,1,size,fp)<0)
      {
      printf("error writing file\n");
      return 0;
      }
      printf("file close return value is %d\n",fclose(fp));
      return 1;
}

size_t recv_fileinfo(int sock,struct sockaddr_in remote)
{
       int nbytes =0;
       unsigned int remote_length = 0;
       char* file_info = (char*)malloc(100*sizeof(char));
       size_t size = 0;
       bzero(file_info,100*sizeof(char));
       remote_length = sizeof(remote);
       nbytes = recvfrom(sock,file_info,100*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
       printf("file_info buffer has %s\n",file_info);
       size = atoi(file_info);
       printf("size is %lu\n",size);
       return size;
}

int main (int argc, char * argv[] )
{
        char *filename = malloc(100*(sizeof(char)));
        char *file_info = malloc(100*sizeof(char));
        char *command = malloc(10*(sizeof(char)));
        char *ack = malloc(10*sizeof(char));
        size_t size;
        int return_value;
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
        if(nbytes != 0)
        {
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
                    bzero(filename,100);
                    }
                    break;
          case PUT:
                   recvfrom(sock,filename,100*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
                   printf("filename is %s\n",filename);
                   size = recv_fileinfo(sock,remote);
                   if(recv_file(sock,remote,filename,size))
                   {
                   printf("recieved the file\n");
                   sendto(sock,"ACK",strlen("ACK"),0,(struct sockaddr *)&remote,sizeof(remote));
                   }
                   bzero(filename,100);
                   break;
          
          case LIST_FILES:
                   system("ls > files.txt");
                   size = send_fileinfo(sock,remote,"files.txt");
                   if(send_file(sock,remote,"files.txt",size))
                   {
                   printf("sending the list of files\n");
                   nbytes = recvfrom(sock,ack,10*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int*)&remote_length);
                   if(nbytes != 0)
                   check_ack(sock,remote,ack);
                   }
                   break;
         
          case DELETE:
                  recvfrom(sock,filename,100*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
                  printf("client wants to delete %s\n",filename);
                  char *delete_command =  malloc(105*sizeof(char));
                  strcpy(delete_command,"rm -f ");
                  strncpy(delete_command+6,filename,strlen(filename));
                  printf("command executing is %s\n",delete_command);
                  if(system(delete_command) == -1)
                  printf("error in deletind file\n");
                  else
                  {
                  printf("deleted successfully\n");
                  sendto(sock,"ACK",strlen("ACK"),0,(struct sockaddr *)&remote,sizeof(remote));
                  }
                  break;
                  
         case EXIT:
                  printf("closing the server\n");
                  return_value = close(sock);
                  if(return_value == 0)
                  printf("connection closed\n");
                  return 0;   
                  break;                   
         default:
                  break;
                 
                           
          }
      
         }// bzero(command,10);              
      }  
}


