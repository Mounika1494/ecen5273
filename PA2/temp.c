#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>

int get_size(int file_desc)
{
  struct stat file_stat;
  if(fstat(file_desc,&file_stat)==-1)
  {
    printf("error in reading file stats\n");
    return -1;
  }
  return (int)file_stat.st_size;
}

void main()
{
int fd;
char filename[] = "ws.conf";
char buffer[4000];
char* found;
char* token;
int i = 0;
fd = open("ws.conf",O_RDONLY);
if(fd == -1)
{
printf("file not there\n");
}
read(fd,buffer,get_size(fd));
if((found = strstr(buffer,"DocumentRoot")) != NULL)
{
 token = strtok(found," \t");
 token = strtok(NULL," \t\n");
 printf("%s",token);
 close(fd);
}
}
