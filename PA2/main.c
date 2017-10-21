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

#define CONNMAX 1000
#define BYTES 1024


int listenfd, clients[CONNMAX];
void error(char *);
void startServer(char *);
void respond(int,char *);

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

char* strrev(char *str)
{
    char *p1,*p2;
    if(! str || ! *str)
     return str;
    for(p1=str,p2=str+strlen(str)-1; p2>p1; ++p1,--p2)
    {
    *p1 ^= *p2;
    *p2 ^= *p1;
    *p1 ^= *p2;
    }
  return str;
}

char* get_info(char *search_string)
{
 printf("\nIn get info\n");
 int fd;
 char filename[] = "ws.conf";
 char buffer[4000];
 char *found;
 char *token;
 fd = open("ws.conf",O_RDONLY);
 if(fd == -1)
 printf("unable to open configuration file\n");
 read(fd,buffer,get_size(fd));
 if((found = strstr(buffer,search_string)) != NULL)
 {
 token = strtok(found," \t\n");
 token = strtok(NULL," \t\n");
 }
 close(fd);
 return token;
}

char* itoa(int num,char *str)
{
   if(str == NULL)
   return NULL;
   sprintf(str,"%d",num);
   return str;
}

void send_client(int socket_desc,char *msg)
{
   printf("%s\n",msg);
   int length = strlen(msg);
   if(send(socket_desc,msg,length,0) == -1)
   {
     printf("sending failed\n");
   }
}

int main(int argc, char* argv[])
{
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
        char *ROOT;
	char* PORT = malloc(6);
	ROOT = malloc(30);
	int slot=0;
        strcpy(ROOT,get_info("DocumentRoot"));
        strcpy(PORT,get_info("Listen"));
	printf("Server started at port no.%s with root directory is %s done",PORT,ROOT); 
	// Setting all elements to -1: signifies there is no client connected
	int i;
	for (i=0; i<CONNMAX; i++)
		clients[i]=-1;
	startServer(PORT);

	// ACCEPT connections
	while (1)
	{
		addrlen = sizeof(clientaddr);
		clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

		if (clients[slot]<0)
			error ("accept() error");
		else
		{
			if ( fork()==0 )
			{
				respond(slot,ROOT);
				exit(0);
			}
		}

		while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;
	}

	return 0;
}

//start server
void startServer(char *port)
{
	struct addrinfo hints, *res, *p;

	// getaddrinfo for host
	memset (&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo( NULL, port, &hints, &res) != 0)
	{
		perror ("getaddrinfo() error");
		exit(1);
	}
	// socket and bind
	for (p = res; p!=NULL; p=p->ai_next)
	{
		listenfd = socket (p->ai_family, p->ai_socktype, 0);
		if (listenfd == -1) continue;
		if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
	}
	if (p==NULL)
	{
		perror ("socket() or bind()");
		exit(1);
	}

	freeaddrinfo(res);

	// listen for incoming connections
	if ( listen (listenfd, 1000000) != 0 )
	{
		perror("listen() error");
		exit(1);
	}
}

char* get_file_format(char* file_info,char *file_type)
{
   char filename[] = "ws.conf";
   char buffer[4000];
   char *found;
   char *token;
   char *search,*parse;
   int fd;
   fd = open("ws.conf",O_RDONLY);
   printf("file type to be searched is %s\n",file_type);
   if(fd == -1)
     printf("unable to open configuration file\n");
   read(fd,buffer,get_size(fd));
   if((found = strstr(buffer,file_info)) != NULL)
   {
    parse = strstr(found,file_type);
    token = strtok(parse," \t\n");
    token = strtok(NULL," \t\n");
   }
   close(fd);
   return token;
}

//client connection
void respond(int n,char* ROOT)
{
	char mesg[99999], *reqline[3], data_to_send[BYTES], path[99999], header[99999];
	int rcvd, fd, bytes_read;
        char size[7];
        char *default_page = malloc(15);
        char *file_name = malloc(30);
        char *format_type = malloc(5);
        char *token = malloc(20);
        char *file_type = malloc(20);
        char *content_type = malloc(10);
	memset( (void*)mesg, (int)'\0', 99999 );

	rcvd=recv(clients[n], mesg, 99999, 0);

	if (rcvd<0)    // receive error
		fprintf(stderr,("recv() error\n"));
	else if (rcvd==0)    // receive socket closed
		fprintf(stderr,"Client disconnected upexpectedly.\n");
	else    // message received
	{
		printf("%s", mesg);
		reqline[0] = strtok (mesg, " \t\n");
		if ( strncmp(reqline[0], "GET\0", 4)==0 )
		{
			reqline[1] = strtok (NULL, " \t");
			reqline[2] = strtok (NULL, " \t\n");
			if ( strncmp( reqline[2], "HTTP/1.0", 8)!=0 && strncmp( reqline[2], "HTTP/1.1", 8)!=0 )
			{
				write(clients[n], "HTTP/1.0 400 Bad Request\n", 25);
			}
			else
			{
				if ( strncmp(reqline[1], "/\0", 2)==0 )
                                   {
                                        strcpy(default_page,"/");
                                        strcat(default_page,get_info("DirectoryIndex"));
                                        strcpy(reqline[1],default_page);
                                   }
                                   
                                strcpy(file_name,reqline[1]);
				strncpy(path, ROOT+1,strlen(ROOT)-2);
				strcpy(&path[strlen(ROOT)-2], reqline[1]);
				printf("file: %s\n", path);
                                strcpy(format_type,path);
                                format_type = strrev(format_type);
                                printf("%s",format_type);
                                token = strtok(format_type,".");
                                strcpy(file_type,".");
                                strcat(file_type,strrev(token));
                                content_type = get_file_format("#Content-Type which the server handles",file_type);
                               
		                if ( (fd=open(path, O_RDONLY))!=-1 )    //FILE FOUND
				{
                                        strcpy(header,"HTTP/1.0 200 Document Follows\n");
                                        itoa(get_size(fd),size);
                                        printf("size is %s\n",size);
                                        strcat(header,"Content-Size :");
                                        strcat(header,size);
                                        strcat(header,"\n");
                                        strcat(header,"Content-Type : ");
                                        strcat(header,content_type);
                                        strcat(header,"\n\n");
                                        send_client(clients[n],header);
					while ( (bytes_read=read(fd, data_to_send, BYTES))>0 )
						write (clients[n], data_to_send, bytes_read);
				}
				else    write(clients[n], "HTTP/1.0 404 Not Found\n", 23); //FILE NOT FOUND
			}
		}
	}

	//Closing SOCKET
	shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
	close(clients[n]);
	clients[n]=-1;
}
