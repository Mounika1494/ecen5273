#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <sys/stat.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>
#include<errno.h>
#include <openssl/md5.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
//Basic idea is refered from godlytalias.blogspot
int timeout = 0;
void error(char* msg)
{
perror(msg);
exit(0);
}

int get_size(int file_desc)
{
  struct stat file_stat;
  if(fstat(file_desc,&file_stat) == -1)
  {
    printf("Error in reading file stats\n");
    return -1;
  }
  return (int)file_stat.st_size;
}

void getcachefiletime(char* name,char time_created[1000])
{
  struct stat attr;
  stat(name,&attr);
  sprintf(time_created,"%s",ctime(&attr.st_mtime));
}

char *computeMD5(const char *str,int length)
{
  int n;
  MD5_CTX c;
  unsigned char digest[16];
  char *out = (char*)malloc(33);
  MD5_Init(&c);
  while(length > 0)
  {
    if(length >512){
      MD5_Update(&c,str,512);
    }else{
      MD5_Update(&c,str,length);
    }
    length -= 512;
    str += 512;
  }
  MD5_Final(digest,&c);
  for(n = 0;n<16;n++){
    snprintf(&(out[n*2]),32,"%02x",(unsigned int)digest[n]);
  }
  return out;
}

int cache_present(char* cache_name)
{
  struct stat st = {0};
  FILE* fp;
  char str[1000];
  int file_time;
  int present_time;
  if(stat("Cache",&st) == -1){
    mkdir("Cache",0700);
  }
  sprintf(str,"Cache/%s",cache_name);
  fp = fopen(str,"r");
  char time_created[1000];
  if(fp == NULL)
  {
    return 0;
  }
  else
  {
       getcachefiletime(str,time_created);
       char *hr, *min,*sec;

       hr = strtok(time_created,":") ;

       min = strtok(NULL,":") ;

       sec = strtok(NULL,":") ;
       sec = strtok(sec, " ");

       hr = strtok(hr," ") ;
       hr = strtok(NULL," ");
       hr = strtok(NULL," ");
       hr = strtok(NULL," ");

       file_time = atoi(hr)*3600 + atoi(min)*60 + atoi(sec);

       time_t current_time;
       time(&current_time);
       bzero(time_created,sizeof(time_created));
       sprintf(time_created,"%s", ctime(&current_time));

       printf("Present Time: %s\n", time_created);

       hr = strtok(time_created,":") ;

       min = strtok(NULL,":") ;

       sec = strtok(NULL,":") ;
       sec = strtok(sec, " ");

       hr = strtok(hr," ") ;
       hr = strtok(NULL," ");
       hr = strtok(NULL," ");
       hr = strtok(NULL," ");

       present_time = atoi(hr)*3600 + atoi(min)*60 + atoi(sec);

       printf("Present time %d\n", present_time);
       printf("File time %d \n", file_time);
       printf("Difference is %d\n", present_time - file_time);
       if (present_time - file_time > timeout)
       {
           printf("TIMEOUT EXPIRED\n");
           return 0;
       }

       else
       {
         fclose(fp);
         return 1;
       }
    }
}

int is_blocked(char* host_name,char* host_ip)
{
  int fd;
  int found = 0;
  char buffer[10000];
  char filename[] = "blocked_websites.txt";
  fd = open(filename,O_RDONLY);
  if(fd == -1)
  {
    printf("There are no blocked websites\n");
    return 0;
  }
  else
  {
    read(fd,buffer,get_size(fd));
    if((found = strstr(buffer,host_name) != NULL) || (found = strstr(buffer,host_ip) != NULL))
    {
      printf("blocked website %s:%s", host_name,host_ip);
      close(fd);
      return 1;
    }

    else{
      close(fd);
      return 0;
    }

  }

}

void link_prefetch(char *filename,int sockfd)
{
  printf("prefetching links\n");
  char buf_to_send[1000000];
  char filepath[9999];
  int i,openFile;
  char *md5;
  FILE *fp;
  char http_url[9999];
  char * ret;
  char *retVal;
  char path[9999];
  char prefetch_req[9999];
  char buffer[9999];
  int readFile,n;
  char md5sum[9999];
  char *new_ptr;


  bzero(http_url,sizeof(http_url));
  bzero(path,sizeof(path));
  bzero(buffer,sizeof(buffer));
  bzero(buf_to_send,sizeof(buf_to_send));
  bzero(filepath,sizeof(filepath));
  bzero(prefetch_req,sizeof(prefetch_req));
  bzero(md5sum,sizeof(md5sum));
  openFile=open(filename,O_RDONLY);
  if(openFile==-1){
      printf("\n error in reading the file");
  }

  // reading file
  readFile = read(openFile,buf_to_send, sizeof buf_to_send);
  if (readFile < 0)
  {
      printf("FILE reading error\n");
  }

  // md5sum
  if((ret=strstr(buf_to_send,"href=\"http://"))!= NULL){
      while((ret=strstr(ret,"href=\"http://")) ){
          ret = ret+13;
          i=0;
          while(*ret!='"')
              {
                  http_url[i] = *ret;
                  ret++;
                  i++;
              }
          new_ptr=ret;
          http_url[i]='\0';

          strcpy(md5sum, http_url);
          md5 = computeMD5(md5sum,strlen(md5sum));


          printf("\n md5 sum of the link:%s",md5);
          sprintf(filepath,"Cache/%s.html",md5);
          printf("filepath: %s",filepath);
          retVal = strstr(http_url,"/");
          if ( retVal == NULL){
            continue;
          }
          if(retVal!=NULL){
              strcpy(path,retVal);
          }

          *retVal='\0';
          ret =new_ptr +1;

          printf("\nGET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",path,http_url);
          sprintf(prefetch_req,"GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",path,http_url);

          n = send(sockfd,prefetch_req,strlen(prefetch_req),0);
          printf("\n sending request");

          fp=fopen(filepath,"w");
          do{
              printf("\n getting written into file");
              n=recv(sockfd,buffer,500,0);
              fwrite(buffer,1,n,fp);
          }while(n>0);
        }
      }
}

int main(int argc,char* argv[])
{
pid_t pid;
int result = 0;
char *md5sum;
struct sockaddr_in addr_in,client_addr,server_addr;
struct hostent* host;
int sockfd,newsockfd;
FILE *fp;
char cache_name[100];

if(argc<3)
error("./proxy <port_no> timeout");
timeout = atoi(argv[2]);

printf("\n****PROXY SERVER STARTED*****\n");

bzero((char*)&server_addr,sizeof(server_addr));
bzero((char*)&client_addr, sizeof(client_addr));

server_addr.sin_family=AF_INET;
server_addr.sin_port=htons(atoi(argv[1]));
server_addr.sin_addr.s_addr=INADDR_ANY;


sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
if(sockfd<0)
error("Problem in initializing socket");

if(bind(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
error("Error on binding");

listen(sockfd,50);
int client_len=sizeof(client_addr);

accepting:

newsockfd=accept(sockfd,(struct sockaddr*)&client_addr,&client_len);

if(newsockfd<0)
error("Problem in accepting connection");

pid=fork();
if(pid==0)
{
struct sockaddr_in host_addr;
int flag=0,newsockfd1,n,port=0,i,sockfd1;
char buffer[510],request_method[300],request_type[300],http_ver[10];
char path[510];
char buftosend[9999];
char filenamepath[1000];
char* url=NULL;
int ret = 0;
bzero((char*)buffer,500);
recv(newsockfd,buffer,500,0);

sscanf(buffer,"%s %s %s",request_method,request_type,http_ver);

if(((strncmp(request_method,"GET",3)==0))&&((strncmp(http_ver,"HTTP/1.1",8)==0)||(strncmp(http_ver,"HTTP/1.0",8)==0))&&(strncmp(request_type,"http://",7)==0))
{
strcpy(request_method,request_type);
strcpy(path,request_type);
md5sum = computeMD5(path,strlen(path));
printf("md5sum is %s\n",md5sum);
sprintf(cache_name,"%s.html",md5sum);
result = cache_present(cache_name);
if(result == 0)
{
  printf("Not in cache connecting to server\n");
  sprintf(filenamepath,"Cache/%s",cache_name);
  fp = fopen(filenamepath,"w+");
  flag=0;
  for(i=7;i<strlen(request_type);i++)
  {
    if(request_type[i]==':')
    {
    flag=1;
    break;
    }
  }
  url=strtok(request_type,"//");
  if(flag==0)
  {
  port=80;
  url=strtok(NULL,"/");
  }
  else
  {
  url=strtok(NULL,":");
  }

  sprintf(request_type,"%s",url);
  printf("host = %s",request_type);
  host=gethostbyname(request_type);
  printf("\nhost->h_addr:%s,host->h_length:%d\n",(char *)host->h_addr,host->h_length);

  if(flag==1)
  {
  url=strtok(NULL,"/");
  port=atoi(url);
  }
  strcat(request_method,"^]");
  url=strtok(request_method,"//");
  url=strtok(NULL,"/");
  if(url!=NULL)
  url=strtok(NULL,"^]");
  printf("\npath: %s\nPort: %d\n",url,port);


  bzero((char*)&host_addr,sizeof(host_addr));
  host_addr.sin_port=htons(port);
  host_addr.sin_family=AF_INET;
  bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);

  sockfd1=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  newsockfd1=connect(sockfd1,(struct sockaddr*)&host_addr,sizeof(struct sockaddr));
  sprintf(buffer,"\nConnected to %s  IP - %s\n",request_type,inet_ntoa(host_addr.sin_addr));
  ret = is_blocked(request_type,inet_ntoa(host_addr.sin_addr));
  if(ret == 1)
  {
    char blocked[100] = "<html><body><H1>Error 403 Forbidden </H1></body></html>";
    send(newsockfd,blocked,strlen(blocked),0);
  }
  else{
  if(newsockfd1<0)
  error("Error in connecting to remote server");
  //printf("\n%s\n",buffer);
  bzero((char*)buffer,sizeof(buffer));

  //Creating url to send to host
  if(url!=NULL)
  sprintf(buffer,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",url,http_ver,request_type);
  else
  sprintf(buffer,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",http_ver,request_type);

  //sending request to host server
  n=send(sockfd1,buffer,strlen(buffer),0);
  if(n<0)
  error("Error writing to socket");
  else{
  do
  {
    bzero((char*)buffer,500);
    n=recv(sockfd1,buffer,500,0);
    if(!(n<=0))
    {
    send(newsockfd,buffer,n,0);
    fwrite(buffer,1,n,fp);
    }
  }while(n>0);
  }
  fclose(fp);
  }
  link_prefetch(filenamepath,sockfd1);
}
else
{
  char fname_path[1000];
  sprintf(fname_path,"Cache/%s",cache_name);
  printf("present in cache\n");
  fp = fopen(fname_path,"r");
  do {
    n = fread(buftosend,1,9999,fp);
    send(newsockfd,buftosend,n,0);
    printf("Sent\n");
  }while(n > 0);
  fclose(fp);
}
}
else
{
  char invalid_method[200] = "<html><body><H1>Error 400 Bad Request: Invalid method </H1></body></html>";
  send(newsockfd,invalid_method,strlen(invalid_method),0);
}
close(sockfd1);
close(newsockfd);
close(sockfd);
_exit(0);
}
else
{
close(newsockfd);
goto accepting;
}
return 0;
}
