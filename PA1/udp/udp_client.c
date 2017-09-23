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
#include <stdint.h>

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

uint8_t user_command()
{
     char *user_input = malloc(25*(sizeof(char)));
     uint8_t op_selected = 0; 
     //commands available
     printf("select the command\n\n");
     printf("******************************\n");
     printf("get \n\n");
     printf("put \n\n");
     printf("delete \n\n");
     printf("ls\n\n");
     printf("exit\n\n");
     printf("*********************************\n");
     scanf("%s",user_input);
     
     if(!(strcmp(user_input,"get")))
     {
      op_selected = GET;
     }
     if(!(strcmp(user_input,"put")))
     {
      op_selected = PUT;
     }
     if(!(strcmp(user_input,"delete")))
     {
      op_selected = DELETE;
     }
     if(!(strcmp(user_input,"ls")))
     {
      op_selected = LIST_FILES;
     }
     if(!(strcmp(user_input,"exit")))
     {
      op_selected = EXIT;
     }
     return op_selected;

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
      int i=0;
      //printf("number of bytes read is %d\n",read_bytes);
      while(i<2)
      {
      nbytes = sendto(sock,data,packet_size*sizeof(char),0,(struct sockaddr *)&remote,sizeof(remote));
      if(nbytes == packet_size)
      i++;
      }
      read_bytes=read_bytes+nbytes;
      printf("number of bytes sent is %d\n",read_bytes);
      for(int i=0;i<100000;i++);
      }
      fclose(filein);
      return 1;

}



int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	//char buffer[MAXBUFSIZE];
        size_t size = 0;
        char *filename = malloc(25*(sizeof(char)));
	struct sockaddr_in remote;              //"Internet socket address structure"
        unsigned int remote_length;
        struct sockaddr_in from_addr;
        uint8_t option = 0;
        char *ack = malloc(10*sizeof(char));
        
	

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		printf("unable to create socket");
	}
        remote_length = sizeof(remote);
        printf("connected to the server\n");
        
        //get userinput
         option = user_command();
         while(!option)
         {
           printf("Feature is not implemented.Select from available\n");
           option = user_command();
         }
         
         switch(option)
         {
           case GET: 
                     sendto(sock,"get",strlen("get"),0,(struct sockaddr *)&remote,sizeof(remote));
                     printf("Enter the file name\n");
                     scanf("%s",filename);
                     sendto(sock,filename,strlen(filename),0,(struct sockaddr *)&remote,sizeof(remote));
                     size = recv_fileinfo(sock,remote);
                     if(recv_file(sock,remote,filename,size))
                     {
                     sendto(sock,"ACK",strlen("ACK"),0,(struct sockaddr *)&remote,sizeof(remote));
                     printf("successfully recieved the file\n");
                     }
                     bzero(filename,25);
                     break;
           case PUT:
                     sendto(sock,"put",strlen("put"),0,(struct sockaddr *)&remote,sizeof(remote));
                     printf("Enter the file name\n");
                     scanf("%s",filename);
                     sendto(sock,filename,strlen(filename),0,(struct sockaddr *)&remote,sizeof(remote));
                     size = send_fileinfo(sock,remote,filename);
                     if(send_file(sock,remote,filename,size))
                     {
                     printf("sent the file\n");
                     nbytes = recvfrom(sock,ack,10*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int*)&remote_length);
                     if(nbytes !=0)
                     check_ack(sock,remote,ack);
                     bzero(filename,100);
                     bzero(ack,10);
                     }
                     break;
           
           case LIST_FILES:
 
                     filename = "files.txt";
                     FILE *list = NULL;
                     char *list_buffer =malloc(60*sizeof(char));
                     sendto(sock,"ls",strlen("ls"),0,(struct sockaddr *)&remote,sizeof(remote));
                     size = recv_fileinfo(sock,remote);
                     if(recv_file(sock,remote,filename,size))
                     {
                     sendto(sock,"ACK",strlen("ACK"),0,(struct sockaddr *)&remote,sizeof(remote));
                     printf("successfully recieved the file\n");
                     list = fopen(filename,"r");
                     while(fread(list_buffer,1,size,list))
                     {
                     printf("List of files in buffer are ...\n");
                     printf("******************************************\n");
                     printf("%s",list_buffer);
                     printf("******************************************\n");
                     }
                     }
                     //bzero(filename,9);
                     //bzero(list_buffer,60);
                     break;
           
           case DELETE:
                    sendto(sock,"delete",strlen("delete"),0,(struct sockaddr *)&remote,sizeof(remote));
                    printf("Enter the filename\n");
                    scanf("%s",filename);
                    sendto(sock,filename,strlen(filename),0,(struct sockaddr *)&remote,sizeof(remote));
                    nbytes = recvfrom(sock,ack,10*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int*)&remote_length);
                    if(nbytes != 0)
                    check_ack(sock,remote,ack);
                    bzero(filename,100);
                    bzero(ack,10);
                    break;
           
           case EXIT:
                   sendto(sock,"exit",strlen("exit"),0,(struct sockaddr *)&remote,sizeof(remote));
                   break;
                   
        }
        	close(sock);
}

