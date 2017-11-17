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
#include <fcntl.h>
#include <sys/mman.h>

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
int part_to_send[8];
packet_t packet;
uint8_t file_part_info[4][2];
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
  for(int i = 0;i<3;i++)
  {
  if(size_each[i]%512 != 0)
    size_each[i] += (512-size_each[i]%512);
  }
  size_each[3] = size - (size_each[0]+size_each[1]+size_each[2]);
  for (int i = 0;i<4;i++)
    printf("size of %d part is %d\n",i,size_each[i]);
}

//convert integer to asci
char* itoa(int num,char *str)
{
   if(str == NULL)
   return NULL;
   sprintf(str,"%d",num);
   printf("itoa:size of the file is %s\n",str);
   return str;
}


int send_file(char* filename)
{
  char *size_char =  malloc(10);
  char *tsize_char = malloc(10);
  int part = 0;
  uint64_t size;
  int packet_index = 0;
  char *spart = malloc(2);
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
  while(part<=3)
  {
    FILE *fp = fopen(itoa(part,spart),"w");
    while((fs_block_sz = fread(packet.data, sizeof(char), LENGTH, fs)) > 0)
    {
        fwrite(packet.data,sizeof(char),fs_block_sz,fp);
        size_sent = size_sent + fs_block_sz;
      //  printf("size of this part :%d and sent is %d\n",size_each[part],size_sent);
        if(size_sent >= size_each[part])
        {
          fprintf(stdout, "Part:%d completed\n",part );
          break;
        }
        //for(int i =0;i<1000000;i++);
        bzero(&packet,sizeof(packet));
     }
     part++;
     size_sent = 0;
     fclose(fp);
 }
 fclose(fs);
}

void send_part_file(int server)
{
  int start_index = 0;
  int end_index = 0;
  int socket_index = 0;
  if(server == 1)
  {
    printf("******sending to server1******\n");
    start_index = 0;
    end_index = 2;
    socket_index = 0;
  }
  if(server == 2)
  {
    printf("******sending to server2******\n");
    start_index = 2;
    end_index = 4;
    socket_index = 1;
  }
  if(server == 3)
  {
    printf("******sending to server3******\n");
    start_index = 4;
    end_index = 6;
    socket_index = 2;
  }
  if(server == 4)
  {
    printf("******sending to server4******\n");
    start_index = 6;
    end_index = 8;
    socket_index = 3;
  }
  int fs_block_sz;
  int size_sent = 0;
  char *spart = malloc(2);
  char *size_char =  malloc(10);
  for(int j = start_index;j<end_index;j++)
  {
  FILE *fp = fopen(itoa(part_to_send[j]-1,spart),"r");
  while((fs_block_sz = fread(packet.data, sizeof(char), LENGTH, fp)) > 0)
  {
    //printf("part_to_send[j] is %d\n",part_to_send[j]);
    packet.size_data = fs_block_sz;
    packet.index = part_to_send[j];
    strcpy(packet.filesize ,itoa(size_each[part_to_send[j]-1],size_char));
    strcpy(packet.partsize ,"22132");;
    fprintf(stdout,"total filesize:%s partsize:%s packet_index:%d size read:%d %lu\n"
        ,packet.partsize,packet.filesize,
        packet.index,packet.size_data,sizeof(packet));
    if(send(sockfd[socket_index], &packet,sizeof(packet), 0) < 0)
    {
        fprintf(stderr, "ERROR: Failed to send file (errno = %d)\n", errno);
        break;
    }
    size_sent = size_sent + fs_block_sz;
    printf("size of this part :%d and sent is %d\n",size_each[part_to_send[j]-1],size_sent);
    if(size_sent >= size_each[part_to_send[j]-1])
    {
      fprintf(stdout, "Part:%d completed\n",size_each[part_to_send[j]-1]);
      break;
    }
    for(int i =0;i<1000000;i++);
    bzero(&packet,sizeof(packet));
 }
 fclose(fp);
 size_sent = 0;
 printf("Ok %d.File from Client was Sent:size %d!\n",part_to_send[j],size_each[part_to_send[j]-1]);
 }
}

void computeMd5sum(char *filename, char md5sum[100])  {
    //char md5sum[100];
    char systemmd5Cmd[100];
    strncpy(systemmd5Cmd, "md5sum ", sizeof("md5sum "));
    strncat(systemmd5Cmd, filename, strlen(filename));
    FILE *f = popen(systemmd5Cmd, "r");
    while (fgets(md5sum, 100, f) != NULL) {
	  strtok(md5sum,"  \t\n");
    printf( "%s %lu\n", md5sum, strlen(md5sum) );
    }
    pclose(f);
}

/*int recv_file(char* filename)
{
  struct stat st = {0};
	/*Receive File from Client */
/*  char *path = malloc(50);
  char *part_str = malloc(2);
  int first_time = 1;
  FILE* fr = NULL;
	uint64_t size =0;
	int part = 1;

	int fr_block_sz = 0;
	packet_t packet;
	int part_size = 0;
	int file_size = 0;

  while(slot<3)
  {
		bzero(&packet,sizeof(packet));
		while((fr_block_sz = recv(sockfd[slot], &packet, sizeof(packet), 0)) > 0)
		{
      part = packet.index;
      itoa(packet.index,part_str);
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
}*/

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

int decision_md5(char* filename)
{
  int md5sumInt,md5sumIndex = 0;
  char md5sum[100];
  computeMd5sum(filename, md5sum);
 	md5sumInt = md5sum[strlen(md5sum)-1] % 4;
 	md5sumIndex = (4-md5sumInt)%4;
  printf("md5sumIndex %d\n", md5sumIndex);
  if(md5sumIndex == 0)
 {
    part_to_send[0] = 1;
    part_to_send[1] = 2;
    part_to_send[2] = 2;
    part_to_send[3] = 3;
    part_to_send[4] = 3;
    part_to_send[5] = 4;
    part_to_send[6] = 4;
    part_to_send[7] = 1;
 }
  if(md5sumIndex == 1)
  {
    part_to_send[0] = 4;
    part_to_send[1] = 1;
    part_to_send[2] = 1;
    part_to_send[3] = 2;
    part_to_send[4] = 2;
    part_to_send[5] = 3;
    part_to_send[6] = 3;
    part_to_send[7] = 4;
  }
  if(md5sumIndex == 2)
  {
    part_to_send[0] = 3;
    part_to_send[1] = 4;
    part_to_send[2] = 4;
    part_to_send[3] = 1;
    part_to_send[4] = 1;
    part_to_send[5] = 2;
    part_to_send[6] = 2;
    part_to_send[7] = 3;
  }
  if(md5sumIndex == 3)
  {
    part_to_send[0] = 2;
    part_to_send[1] = 3;
    part_to_send[2] = 3;
    part_to_send[3] = 4;
    part_to_send[4] = 4;
    part_to_send[5] = 1;
    part_to_send[6] = 1;
    part_to_send[7] = 2;
  }
  printf("order\n");
  for(int i=0;i<8;i++)
  {
   printf("%d",part_to_send[i]);
  }
}
void recv_file(int psockfd)
{
  int fr_block_sz = 0;
  size_t part_size = 0;
  char *file_part =  malloc(3);
  int index = 0;
  FILE* fr = NULL;
  size_t size = 0;
  int first_time = 1;
    fprintf(stdout,"\n*****waiting for parts\n");
    while((fr_block_sz = recv(psockfd,&packet,sizeof(packet),0)) > 0)
    {
      index = packet.index;
      part_size = atoi(packet.filesize);
      if(first_time == 1)
      {
        fr = fopen(itoa(index,file_part),"w");
      }
      first_time = 0;
      if(fr == NULL)
        printf("File %s Cannot be opened file on server.\n", file_part);
      int write_sz = fwrite(packet.data, sizeof(char),packet.size_data, fr);
      if(write_sz < packet.size_data)
        {
            error("File write failed on server.\n");
        }
      size = size + packet.size_data;
      printf("part:%d size %lu bytes recieved %lu\n",index,part_size,size);
      part_size = atoi(packet.filesize);
      if(size == part_size)
      {
        printf("Done bytes recieved %lu\n",size);
        break;
      }
      bzero(&packet,sizeof(packet));
    }
    printf("%d part recieved from %d server\n",index,psockfd);
    fclose(fr);
}

void recv_file_part()
{
  int fr_block_sz = 0;
  char *file_part =  malloc(3);
  for(int i = 0;i<4;i++)
  {
    fprintf(stdout,"*****waiting for parts\n");
    while((fr_block_sz = recv(sockfd[i],file_part,3,0)) > 0)
    {
      fprintf(stdout,"%s parts are present\n",file_part);
      file_part_info[i][0] = *file_part;
      file_part_info[i][1] = *(file_part+1);
      break;
    }
  }
  for(int i = 0;i<4;i++ )
  {
     for (int j = 0;j< 2; j++)
     {
      printf("%d\t",file_part_info[i][j]);
     }
     printf("\n" );
  }
}
void ask_file_part()
{
  printf("In sending the request for a file part\n");
  char *part = malloc(2);
  int found = 0;
  for(int j = 1;j<=4;j++)//part
  {
    found = 0;
    printf("searching for %d\n",j);
    for(int k = j-1;k<4;k++)//server
    {
      for(int i = 0;i<2;i++)//part in server
      {
        printf("%d\n",file_part_info[k][i]);
        if(file_part_info[k][i] == j+48)
        {
          if(send(sockfd[k],&j,sizeof(int),0)==-1)
          {
            perror("error sending file part numbers\n");
            exit(1);
          }
          printf("Requesting %d from %d server",j,k);
          found = 1;
          break;
        }
      }
      if (found == 1)
      {
        recv_file(sockfd[k]);
        break;
      }
    }
    fprintf(stdout,"*************recieved everything****************\n");
  }
}

void merge_files(char* filename)
{
  FILE *fs1, *fs2, *fs3, *fs4 ,*ft;
  char ch,read_buf[512+1];
  fs1 = fopen("1","r");
  fs2 = fopen("2","r");
  fs3 = fopen("3","r");
  fs4 = fopen("4","r");
  int fs_block_sz = 0;
  if( fs1 == NULL || fs2 == NULL ||fs3 == NULL || fs4 == NULL )
  {
     perror("Error ");
  }

  ft = fopen(filename,"w");

  if( ft == NULL )
  {
     perror("Error ");
  }

  while((fs_block_sz = fread(read_buf, sizeof(char), LENGTH, fs1)) > 0)
  {
     fwrite(read_buf, sizeof(char), LENGTH, ft);
     bzero(read_buf,512);
  }
  while((fs_block_sz = fread(read_buf, sizeof(char), LENGTH, fs2)) > 0)
  {
     fwrite(read_buf, sizeof(char), LENGTH, ft);
     bzero(read_buf,512);
  }
  while((fs_block_sz = fread(read_buf, sizeof(char), LENGTH, fs3)) > 0)
  {
     fwrite(read_buf, sizeof(char), LENGTH, ft);
     bzero(read_buf,512);
  }
  while((fs_block_sz = fread(read_buf, sizeof(char), LENGTH, fs4)) > 0)
  {
     fwrite(read_buf, sizeof(char), LENGTH, ft);
     bzero(read_buf,512);
  }

  printf("4 files were merged into %s file successfully.\n",filename);

  fclose(fs1);
  fclose(fs2);
  fclose(fs3);
  fclose(fs4);
  fclose(ft);
  system("rm -f 0 1 2 3 4");
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
              decision_md5(filename);
              send_file(filename);
              send_part_file(1);
              send_part_file(2);
              send_part_file(3);
              send_part_file(4);
              break;
     case GET:
              for(int i = 0;i<4;i++)
              {
              if (send(sockfd[i], "get",strlen("get"),0) == -1){
                  perror("recieve");
                  exit (1);
              }
              }
              printf("Sent the get command\n");
              printf("Enter the file name\n");
              scanf("%s",filename);
              for(int i = 0;i<4;i++)
              {
              send_fileinfo(filename, i);
              }
              recv_file_part();
              ask_file_part();
              merge_files(filename);
              break;



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
