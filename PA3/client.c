#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>

#define LENGTH 512

typedef struct p1 {
      uint8_t index;
			uint16_t size_data;
			char filesize[10];
      char partsize[10];
      char data[512+1];
      }packet_t;
typedef struct p2 {
     uint8_t name_size;
     char filename[255];
}fileinfo_t;
/***********commands******/
typedef enum
{
     GET = 1,
     PUT = 2,
     DELETE = 3,
     LIST_FILES = 4,
     EXIT = 5
}commands;


int sockfd[4];
int size_each[4];
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int get_filesize(char *filename)
{
      FILE* fp = NULL;
      size_t size = 0;
      char* size_str = malloc(5);
      fp = fopen(filename,"rb+");
      if(fp == NULL)
      {
      printf("file can't be opened\n");
      return 0;
      }
      //get its size
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      rewind (fp);
			fclose(fp);
      printf("Size of the file is %lu\n",size);
      return size;
}
int connect_server(int PORT,int n)
{
  struct sockaddr_in remote_addr;
	/* Get the Socket file descriptor */
	if ((sockfd[n] = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor! (errno = %d)\n",errno);
		exit(1);
	}

	/* Fill the socket address struct */
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &remote_addr.sin_addr);
	bzero(&(remote_addr.sin_zero), 8);

	/* Try to connect the remote */
	if (connect(sockfd[n], (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
	{
		fprintf(stderr, "ERROR: Failed to connect to the host! (errno = %d)\n",errno);
		exit(1);
	}
	else
		printf("[Client] Connected to server at port %d...ok!\n", PORT);
}

void split_file(int size)
{
  int split_size = 0;
  int extra = 0;
  extra = size%4;
  printf("extra is %d\n",extra);
  split_size = size/4;
  if(extra == 0)
  {
    size_each[0] = split_size;
    size_each[1] = split_size;
    size_each[2] = split_size;
    size_each[3] = split_size;
  }
  else if(extra == 1)
  {
    size_each[0] = split_size;
    size_each[1] = split_size;
    size_each[2] = split_size;
    size_each[3] = split_size+1;
  }
  else if(extra == 2)
  {
    size_each[0] = split_size;
    size_each[1] = split_size;
    size_each[2] = split_size + 1;
    size_each[3] = split_size + 1;
  }
  else if(extra == 3)
  {
    size_each[0] = split_size;
    size_each[1] = split_size + 1;
    size_each[2] = split_size + 1;
    size_each[3] = split_size + 1;
  }
  for (int i = 0;i<4;i++)
    printf("size of %d part is %d\n",i,size_each[i]);
}

//convert integer to asci
char* itoa(int num,char *str)
{
   if(str == NULL)
   return NULL;
   sprintf(str,"%d",num);
   printf("size of the file is %s",str);
   return str;
}
int send_file(char* filename)
{
  char *size_char =  malloc(10);
  char *tsize_char = malloc(10);
  int part = 0;
  uint64_t size;
  int packet_index = 0;
//  char* fs_name = "/home/netsys/ecen5273/PA3/apple.png";
  printf("[Client] Sending %s to the Server... ", filename);
  size = get_filesize(filename);
  split_file(size);
  FILE *fs = fopen(filename, "r");
  if(fs == NULL)
  {
    printf("ERROR: File %s not found.\n", filename);
    exit(1);
  }
  int fs_block_sz;
  int size_sent = 0;
  packet_t packet;
  while(part<=3)
  {
    while((fs_block_sz = fread(packet.data, sizeof(char), LENGTH, fs)) > 0)
    {
        strcpy(packet.partsize ,itoa(size,tsize_char));
        packet.index = part+1;
        packet.size_data = fs_block_sz;
        strcpy(packet.filesize ,itoa(size_each[part],size_char));
        //strcpy(packet.data,sdbuf);
        fprintf(stdout,"total filesize:%s partsize:%s packet_index:%d size read:%d %lu\n"
                ,packet.partsize,packet.filesize,packet.index,packet.size_data,sizeof(packet));
        //if(send(sockfd, sdbuf, fs_block_sz, 0) < 0)
        if(send(sockfd[part], &packet,sizeof(packet), 0) < 0)
        {
            fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", filename, errno);
            break;
        }
        //bzero(sdbuf, LENGTH);
        size_sent = size_sent + fs_block_sz;
        printf("size of this part :%d and sent is %d\n",size_each[part],size_sent);
        if(size_sent >= size_each[part])
        {
          fprintf(stdout, "Part:%d completed\n",part );
          break;
        }
        for(int i =0;i<1000000;i++);
        bzero(&packet,sizeof(packet));
     }
     part++;
     size_sent = 0;
     printf("Ok %d.File %s from Client was Sent!\n",part, filename);
 }
}
/****************************************************************
*@Description: Check the user input if it is valid
*
*@param NULL
*
*@return enum command
***************************************************************/
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
void send_fileinfo(char* filename,int n)
{
  fileinfo_t fileinfo;
  fileinfo.name_size = strlen(filename);
  strncpy(fileinfo.filename,filename,fileinfo.name_size);
  if (send(sockfd[n], &fileinfo,sizeof(fileinfo),0) == -1){
        perror("send");
        exit (1);
}
}

int main(int argc, char *argv[])
{
	/* Variable Declarations*/
  char revbuf[LENGTH];
	char PORT[4][6];
  char* filename = malloc(10);
  int option;
	//error handling
	if (argc != 5) {
			fprintf(stderr,"usage: client portno\n");
			exit(1);
	}
  strcpy(PORT[0],argv[1]);
  strcpy(PORT[1],argv[2]);
  strcpy(PORT[2],argv[3]);
  strcpy(PORT[3],argv[4]);
  connect_server(atoi(PORT[0]),0);
  connect_server(atoi(PORT[1]),1);
  connect_server(atoi(PORT[2]),2);
  connect_server(atoi(PORT[3]),3);
  while (1)
  {
    option = user_command();
    while(!option)
    {
      printf("Feature is not implemented.Select from available\n");
      option = user_command();
    }
    //Based on the option do required
    switch(option)
    {
      case PUT:
              for(int i =0;i<4;i++)
              {
              if (send(sockfd[i], "put",strlen("put"),0) == -1){
                  perror("send");
                  exit (1);
              }
              printf("Sent the put command\n");
              printf("Enter the file name\n");
              scanf("%s",filename);
              send_fileinfo(filename, i);
              }
              send_file(filename);


	/* Receive File from Server
	printf("[Client] Receiveing file from Server and saving it as final.txt...");
	char* fr_name = "/home/netsys/ecen5273/PA3/apple_ex.png";
	FILE *fr = fopen(fr_name, "a");
	if(fr == NULL)
		printf("File %s Cannot be opened.\n", fr_name);
	else
	{
		bzero(revbuf, LENGTH);
		int fr_block_sz = 0;
	    while((fr_block_sz = recv(sockfd[0], revbuf, LENGTH, 0)) > 0)
	    {
			int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
	        if(write_sz < fr_block_sz)
			{
	            error("File write failed.\n");
	        }
			bzero(revbuf, LENGTH);
			if (fr_block_sz == 0 || fr_block_sz != 512)
			{
				break;
			}
		}
		if(fr_block_sz < 0)
        {
			if (errno == EAGAIN)
			{
				printf("recv() timed out.\n");
			}
			else
			{
				fprintf(stderr, "recv() failed due to errno = %d\n", errno);
			}
		}
	    printf("Ok received from server!\n");
	    fclose(fr);
	}
	close (sockfd);
	printf("[Client] Connection lost.\n");
	return (0);*/
  }
 }
}
