#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BACKLOG 100000
#define CONNMAX 1000
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

typedef struct p3 {
  char user_name[30];
  char password[20];
}userinfo_t;
/***********commands******/
typedef enum
{
     GET = 1,
     PUT = 2,
     DELETE = 3,
     LIST_FILES = 4,
     EXIT = 5
}commands;
int nsockfd[1000],sockfd;
char* server_folder;


void error(const char *msg)
{
	perror(msg);
	exit(1);
}

//convert integer to asci
char* itoa(int num,char *str)
{
   if(str == NULL)
   return NULL;
   sprintf(str,"%d",num);
   printf("itoa: size of the file is %s\n",str);
   return str;
}

int recv_file(int sockfd,char* filename)
{
  struct stat st = {0};
	/*Receive File from Client */
  char *path = malloc(50);
  char *part_str = malloc(2);
  int first_time = 1;
  FILE* fr = NULL;
	uint64_t size =0;
	int part = 1;

		int fr_block_sz = 0;
		packet_t packet;
		int part_size = 0;
		int file_size = 0;
		//while((fr_block_sz = recv(nsockfd, revbuf, LENGTH, 0)) > 0)
    for(int i = 0;i<2;i++)
    {
    bzero(path,50);
    first_time = 1;
    size = 0;
    bzero(&packet,sizeof(packet));
		while((fr_block_sz = recv(sockfd, &packet, sizeof(packet), 0)) > 0)
		{
      part = packet.index;
      itoa(packet.index,part_str);
      printf("filename is %s\n",path);
      if(first_time == 1)
      {
        strcat(path,server_folder);
        if(stat(path,&st) == -1){
          mkdir(path,0700);
        }
        strcat(path,"/");
        strcat(path,"Mounika");
        if(stat(path,&st) == -1){
          mkdir(path,0700);
        }
        strcat(path,"/");
        strcat(path,".");
        strcat(path,filename);
        strcat(path,".");
        strcat(path,part_str);
        fr = fopen(path, "w");
     }
      first_time = 0;
      if(fr == NULL)
        printf("File %s Cannot be opened file on server.\n", filename);
			fprintf(stdout,"part size:%s index: %d size:%d\n",packet.filesize,packet.index,packet.size_data);
			int write_sz = fwrite(packet.data, sizeof(char),packet.size_data, fr);
			if(write_sz < packet.size_data)
				{
						error("File write failed on server.\n");
				}
			size = size + packet.size_data;
			printf("part:%d bytes recieved %lu\n",part,size);
			part_size = atoi(packet.filesize);
			if(size == part_size)
			//if(size == 693248 || size == 693248*2 )
			{
				printf("Done bytes recieved %lu\n",size);
				break;
			}
			bzero(&packet,sizeof(packet));
		}
		printf("%d.part completed size:%lu \n",part,size);
		bzero(&packet,sizeof(packet));
		if(fr_block_sz < 0)
			{
					if (errno == EAGAIN)
					{
								printf("recv() timed out.\n");
						}
						else
						{
								fprintf(stderr, "recv() failed due to errno = %d\n", errno);
								exit(1);
						}
				}
		printf("Ok received from client!\n");
		fclose(fr);
  }
}

/****************************************************************
*@Description: Check the user input if it is valid
*
*@param NULL
*
*@return enum command
***************************************************************/
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
//start server
void startServer(char *port)
{
	int sin_size;
	struct sockaddr_in addr_local; /* client addr */
	struct sockaddr_in addr_remote; /* server addr */
	/* Get the Socket file descriptor */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor. (errno = %d)\n", errno);
		exit(1);
	}
	else
		printf("[Server] Obtaining socket descriptor successfully.\n");

	/* Fill the client socket address struct */
	addr_local.sin_family = AF_INET; // Protocol Family
	addr_local.sin_port = htons(atoi(port)); // Port number
	addr_local.sin_addr.s_addr = INADDR_ANY; // AutoFill local address
	bzero(&(addr_local.sin_zero), 8); // Flush the rest of struct

	/* Bind a special Port */
	if( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to bind Port. (errno = %d)\n", errno);
		exit(1);
	}
	else
		printf("[Server] Binded tcp port %s in addr 127.0.0.1 sucessfully.\n",port);

	/* Listen remote connect/calling */
	if(listen(sockfd,BACKLOG) == -1)
	{
		fprintf(stderr, "ERROR: Failed to listen Port. (errno = %d)\n", errno);
		exit(1);
	}
	else
		printf ("[Server] Listening the port %s successfully.\n", port);

}

void check_fileinfo(int sockfd,char* filename)
{
  int part[2];
  char *path = malloc(50);
  char *part_str = malloc(2);
  struct stat st = {0};
  int j = 0;
  char *send_data = malloc(3);
  for(int i =1;i<=4;i++)
  {
  bzero(path,50);
  strcat(path,server_folder);
  strcat(path,"/");
  strcat(path,"Mounika");
  strcat(path,"/");
  strcat(path,".");
  strcat(path,filename);
  strcat(path,".");
  strcat(path,itoa(i,part_str));
  printf("path is %s\n",path);
  if(access(path,F_OK) == -1){
  printf("%d file part not present\n",i);
  }
  else{
  part[j] = i;
  j++;
  }
  }
  fprintf(stdout,"*******check done*****\n");
  for(int k =0;k<2;k++)
  {
    fprintf(stdout,"%d\t",part[k]);
  }
  fprintf(stdout,"*******sent*****\n");
  itoa(part[0],part_str);
  strcat(send_data,part_str);
  itoa(part[1],part_str);
  strcat(send_data,part_str);
  fprintf(stdout,"%s",send_data);
  if(send(sockfd,send_data,3,0) == -1)
  {
    perror("error sending file part numbers");
    //exit(1);
  }
  //for(int j = 0;j<10000;j++);

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

void send_file(int sockfd,int part,char *filename)
{
  char* path = malloc(50);
  struct stat st = {0};
  int fs_block_sz = 0;
  char *size_char = malloc(10);
  char* part_str = malloc(2);
  packet_t packet;
  size_t size;
  int size_sent = 0;
  FILE* fp = NULL;

  itoa(part,part_str);
  strcat(path,server_folder);
  if(stat(path,&st) == -1){
   exit(1);
  }
  strcat(path,"/");
  strcat(path,"Mounika");
  if(stat(path,&st) == -1){
    exit(1);
  }
  strcat(path,"/");
  strcat(path,".");
  strcat(path,filename);
  strcat(path,".");
  strcat(path,part_str);
  size = get_filesize(path);
  fprintf(stdout, "part sending is %s\n",path );
  fp = fopen(path, "r");
  while((fs_block_sz = fread(packet.data, sizeof(char), LENGTH, fp)) > 0)
  {
    packet.index = part;
    packet.size_data = fs_block_sz;
    strcpy(packet.filesize ,itoa(size,size_char));
    fprintf(stdout,"total filesize:%s packet_index:%d size read:%d %lu\n"
        ,packet.filesize,packet.index,packet.size_data,sizeof(packet));
    if(send(sockfd,&packet,sizeof(packet), 0) < 0)
    {
        fprintf(stderr, "ERROR: Failed to send file (errno = %d)\n", errno);
        break;
    }
    size_sent = size_sent + fs_block_sz;
    printf("size of this part :%lu and sent is %d\n",size,size_sent);
    if(size_sent >= size)
    {
      fprintf(stdout, "Part:%lu completed\n",size);
      break;
    }
    for(int i =0;i<1000000;i++);
    bzero(&packet,sizeof(packet));
  }
  fclose(fp);
  size_sent = 0;
  fprintf(stdout, "Completed Sending the part: %d\n",part);
}

void recv_which_part(int sockfd,char *filename)
{
  int part;
  int rcvd = 0;
  rcvd=recv(sockfd,&part,sizeof(part), 0);
  if (rcvd<0)    // receive error
    fprintf(stdout,("recv() error\n"));
else if (rcvd==0)    // receive socket closed
    fprintf(stdout,"Client disconnected upexpectedly.\n");
  fprintf(stdout, "%d part is asked from client\n",part );
  send_file(sockfd,part,filename);
}

//get size of the file
int get_size(int file_desc)
{
  struct stat file_stat;
  if(fstat(file_desc, &file_stat) == -1)
  {
    printf("error in reading the file stats\n");
    return -1;
  }
  return (int)file_stat.st_size;
}

//get the information required from ws.conf file
char* get_info(char *search_string)
{
 int fd;
 char filename[] = "dfc.conf";
 char buffer[4000];
 char *found;
 char *token;
 fd = open("../dfc.conf",O_RDONLY);
 if(fd == -1)
 printf("unable to open configuration file\n");
 read(fd,buffer,get_size(fd));
 if((found = strstr(buffer,search_string)) != NULL)
 {
 token = strtok(found," \t\n");
 token = strtok(NULL," \t\n");
 }
 close(fd);
 printf("search_string is %s value is %s",search_string,token);
 return token;
}


void client_respond(int n)
{
    int rcvd = 0;
    int option = 0;
    int flag = 0;
    char* command = malloc(10);
    char* filename = malloc(20);
    char* size = malloc(7);
		fileinfo_t fileinfo;
    userinfo_t userinfo;
    while(1)
    {
      rcvd=recv(nsockfd[n],&userinfo,sizeof(userinfo), 0);
      printf("with username is %s and password is %s\n",userinfo.user_name,userinfo.password);
      if(strcmp(userinfo.user_name,get_info("Username")) == 0){
      if(strcmp(userinfo.password,get_info("Password")) == 0){
        printf("socket is %d\n",nsockfd[n]);
        send(nsockfd[n],"Ok",3,0);
        flag = 1;
      }
      }
    rcvd=recv(nsockfd[n],command, 10, 0);
    if (rcvd<0)    // receive error
      fprintf(stdout,("recv() error\n"));
    else if (rcvd==0)    // receive socket closed
      fprintf(stdout,"Client disconnected upexpectedly.\n");
    option  = command_decode(command);
    bzero(command,10);
    bzero(filename,20);
    switch(option)
    {
      case PUT:
                if(flag == 1)
                {
                rcvd=recv(nsockfd[n],&fileinfo,sizeof(fileinfo), 0);
                printf("with filename is %s\n",fileinfo.filename);
								strncpy(filename,fileinfo.filename,fileinfo.name_size);
                recv_file(nsockfd[n],filename);
                break;
                }
      case GET:
                if(flag ==1 )
                {
                rcvd = recv(nsockfd[n],&fileinfo,sizeof(fileinfo),0);
                printf("filename is %s\n",fileinfo.filename);
                strncpy(filename,fileinfo.filename,fileinfo.name_size);
                check_fileinfo(nsockfd[n],filename);
                printf("socket is %d\n",nsockfd[n]);
                recv_which_part(nsockfd[n],filename);
                break;
                }
    }
  }
}


int main (int argc, char *argv[])
{
	/* Defining Variables */
//	int sockfd;
	//int nsockfd;
	int sin_size;
//	struct sockaddr_in addr_local; /* client addr */
	struct sockaddr_in addr_remote; /* server addr */
//	char revbuf[LENGTH]; // Receiver buffer
	int option = 0;
	char PORT[6];
	int rcvd = 0;
	int slot = 0;
	char* filename = malloc(20);
  char* command =  malloc(10);
  server_folder = malloc(5);
	//error handling
	if (argc != 3) {
			fprintf(stderr,"usage: server portno\n");
			exit(1);
	}
	strcpy(PORT,argv[2]);
  strcpy(server_folder,argv[1]);
	// initialise all elements to -1: no client is der

	for (int i=0; i<CONNMAX; i++)
	 nsockfd[i]=-1;
	startServer(PORT);
	int success = 0;
	while(success == 0)
	{
		sin_size = sizeof(struct sockaddr_in);

		/* Wait a connection, and obtain a new socket file despriptor for single connection */
		if ((nsockfd[slot] = accept(sockfd, (struct sockaddr *)&addr_remote, &sin_size)) == -1)
		{
		    fprintf(stderr, "ERROR: Obtaining new Socket Despcritor. (errno = %d)\n", errno);
			exit(1);
		}
		else
			printf("[Server] Server has got connected from %s.\n", inet_ntoa(addr_remote.sin_addr));
			if (nsockfd[slot]<0)
  			printf ("accept() error");
  		else
  		{
  			if ( fork()==0 )
  			{
					printf("slot request:%d\n",slot);
  				client_respond(slot);
  				exit(0);
  			}
  		}

  		while (nsockfd[slot]!=-1) slot = (slot+1)%CONNMAX;
		}
		return 0;

		/* Call the Script */
		//system("cd ; chmod +x script.sh ; ./script.sh");

		/* Send File to Client */
		//if(!fork())
		//{
		  /*  char* fs_name = "/home/netsys/ecen5273/PA3/Server/apple_ex.png";
		    char sdbuf[LENGTH]; // Send buffer
		    printf("[Server] Sending %s to the Client...", fs_name);
		    FILE *fs = fopen(fs_name, "r");
		    if(fs == NULL)
		    {
		        fprintf(stderr, "ERROR: File %s not found on server. (errno = %d)\n", fs_name, errno);
				exit(1);
		    }

		    bzero(sdbuf, LENGTH);
		    int fs_block_sz;
		    while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs))>0)
		    {
		        if(send(nsockfd, sdbuf, fs_block_sz, 0) < 0)
		        {
		            fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
		            exit(1);
		        }
		        bzero(sdbuf, LENGTH);
		    }
		    printf("Ok sent to client!\n");
		    success = 1;
		    close(nsockfd);
		    printf("[Server] Connection with Client closed. Server will wait now...\n");
		    while(waitpid(-1, NULL, WNOHANG) > 0);*/
		//}
	//}
}
