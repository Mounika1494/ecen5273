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

typedef struct p1 {
      int index;
      uint8_t data[1024+1];
}packet_t;


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

void decryption(packet_t* packet)
{
      char* key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      int j = 0;
      int packet_size = 1024;
      for(int i = 0;i<packet_size;i++)
      {

         *(packet->data + i) = *(packet->data+i) ^ *(key+j);
         j++;
         if(j==25)
         j=0;
      }
}

void encryption(packet_t* packet)
{
      char* key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
      int j = 0;
      int packet_size = 1024;
      for(int i = 0;i<packet_size;i++)
      {

         *(packet->data + i) = *(packet->data+i) ^ *(key+j);
         j++;
         if(j==25)
         j=0;
      }
}

int my_strcmp(char *str1,char *str2,int size)
{
     int i=0;
     while(i<size)
     {
     if(*(str1+i) == *(str2+i))
     i++;
     else
     {
     printf("condition fails at %d\n",i);
     return 0;
     break;
     }
}
printf("number of char compared %d\n",i);
return 1;
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
      //encryption(data);
      //printf("number of bytes read is %d\n",read_bytes);
      while(i<2)
      {
      nbytes = sendto(sock,data,packet_size*sizeof(char),0,(struct sockaddr *)&remote,sizeof(remote));
      if(nbytes == packet_size)
      i++;
      }
      read_bytes=read_bytes+nbytes;
      printf("number of bytes sent is %d\n",read_bytes);
      bzero(data,packet_size);
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
      int packet_size = 1024;
      int packet_count = 0;
      int packet_index = 0;
      int result = 0;
      packet_t packet1;
      packet_t packet2;
      char *file = malloc(size*(sizeof(char)));
      bzero(&(packet1.data),packet_size);
      fp = fopen(file_name,"wb+");
      printf("size is %lu\n",size);
      if(size%packet_size==0)
      packet_count = size/packet_size;
      else
      packet_count = size/packet_size+1;
      printf("packet_count is %d\n",packet_count);
      //recieve the data from server
      while(packet_index < packet_count)
      {
      recvfrom(sock,&packet1,sizeof(packet1),0,(struct sockaddr *)&remote,(unsigned int*)&remote_length);
      recvfrom(sock,&packet2,sizeof(packet2),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
      decryption(&packet1);
      decryption(&packet2);
      printf("packet index is %d,%d\n",packet1.index,packet2.index);
      printf("string size is %d,%d\n",strlen(&packet1.data),strlen(&packet2.data));
      if(packet_count == 1)
      {
      if(fwrite(&(packet1.data),1,size,fp)<0)
      {
      printf("error writing file\n");
      return 0;
      }
      fclose(fp);
      return 1;
      }
      if(packet1.index == packet2.index)
      {
      printf("both packets are same\n");
      if(fwrite(&(packet1.data),1,packet_size,fp)<0)
      {
      printf("error writing file\n");
      return 0;
      }
      packet_index++;
      }
      else if(packet1.index=packet2.index+1)
      {
      if(fwrite(&(packet1.data),1,packet_size,fp)<0)
      {
      printf("error writing file\n");
      return 0;
      }
      packet_index++;
      if(fwrite(&(packet2.data),1,packet_size,fp)<0)
      {
      printf("error writing file\n");
      return 0;
      }
      packet_index++;
      }
      bzero(&(packet2.data),packet_size);
      bzero(&(packet1.data),packet_size);
      printf("data read is %d\n",packet_index*packet_size);
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
        bzero(command,10);
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
                  {
                  printf("file not present\n");
                  printf("Do ls to see the list of files\n");
                  }
                  else
                  {
                  printf("deleted successfully\n");
                  sendto(sock,"ACK",strlen("ACK"),0,(struct sockaddr *)&remote,sizeof(remote));
                  }
                  bzero(filename,100);
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
   close(sock);  
}


