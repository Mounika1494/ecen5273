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


/****packet structure****/

typedef struct p1 {
      int index;
      uint8_t data[1024+1];
      }packet_t;

/***********commands******/
typedef enum
{
     GET = 1,
     PUT = 2,
     DELETE = 3,
     LIST_FILES = 4,
     EXIT = 5
}commands;

//Utility Functions
/**********************************************
*@Description: int to ascii
*
*@param: number,string to store
*
*@return converted string - success
*                  NULL - fail
**************************************************/
char* itoa(int num,char *str)
{
   if(str == NULL)
   return NULL;
   sprintf(str,"%d",num);
   return str;
}


/**********************************************************
*@Description: check if ack is recieved
*
*@param:int socket fd,
*        struct remote ip
*        char    filename - name of file to be transfered
*@return  1 -Success
*         0 - Fail
**********************************************************/
uint8_t check_ack(int sock,struct sockaddr_in remote,char *ack)
{
   if(!(strcmp(ack,"ACK")))
   {
   printf("ack is recieved\n");
   return 1;
   }
return 0;
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


/********************************************
*@Description: Decryption of packet
*
*@param packet to be decrypted
*
*@return NULL
*************************************************/
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


/********************************************************************************
*@Description: Compares 2 strings and if they are either substrings or equal strings
*
*@param strings to be compared
*
*@return 1 - success
         0 - Failure
*************************************************************************************/
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


/******************************************************************************
@Description: Recieve the file from server.For reliability 2 packets are transfered 
              2 packets recieved are compared.If equal then one is discarded.If different packets are              recieved based on the sequence number decide to buffer it or discard it
*
*
*@param  int socket fd,
*        struct remote ip
*        char    filename - name of file to be transfered
*@return 1 success
*        0 fail
********************************************************************************/
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

      
/*********************************************************************
*@Description: Recieve the size of file from Server
*
*@param
*      int sock
*      struct remote ip
*      char*  filename - name of file 
*@return
*      size success
*      0    failure
**********************************************************************/

size_t recv_fileinfo(int sock,struct sockaddr_in remote)
{
       int nbytes =0;
       unsigned int remote_length = 0;
       char* file_info = (char*)malloc(100*sizeof(char));
       size_t size = 0;
       bzero(file_info,100*sizeof(char));
       remote_length = sizeof(remote);
       nbytes = recvfrom(sock,file_info,100*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int *)&remote_length);
       size = atoi(file_info);
       printf("size is %lu\n",size);
       return size;
}


/*********************************************************************
*@Description: Send the size of file
*
*@param
*      int sock
*      struct remote ip
*      char*  filename - name of file 
*@return
*      size success
*      0    failure
**********************************************************************/
size_t send_fileinfo(int sock,struct sockaddr_in remote,char *filename)
{
      FILE* fp = NULL;
      size_t size = 0;
      char* file_info = (char *)malloc(100*sizeof(char));
      fp = fopen(filename,"rb+");
      if(fp == NULL)
      {
      printf("file can't be opened\n");
      sendto(sock,"0",1,0,(struct sockaddr *)&remote,sizeof(remote));
      return 0;
      }
      //get its size
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      rewind (fp);
      if(itoa(size,file_info)!= NULL)
      sendto(sock,file_info,strlen(file_info),0,(struct sockaddr *)&remote,sizeof(remote));
      return size;
}


/**************************************************************
*@DescriptionEncryption of packet  xor is the algorithm
*
*@param packer
*
*@return NULL
************************************************************/
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


/******************************************************************************
*@Description sends the packet to the remote and delay as the rx write will be slow
*             For reliability each packet is sent twice an ack can also be implemented
*             But UDP doesn't work like that and efficiency will be high in this 
*             implementation
*
*@param  
*        int socket fd,
*        struct remote ip
*        char    filename - name of file to be transfered
* unsigned long  size
*@return
*         error  0
*         success 1
****************************************************************************/
int send_file(int sock,struct sockaddr_in remote,char *filename,size_t size)
{
      FILE* filein = NULL;
      int nbytes = 0;
      int read_bytes = 0;
      int packet_size = 1024;
      int packet_index  = 1;
      char *data = malloc(packet_size * sizeof(char));
      char *buffer = malloc(packet_size * sizeof(char));
      char *encrypted_data = malloc(packet_size * sizeof(char));
      packet_t packet;
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
      bzero(&(packet.data),packet_size);
      while(fread(&(packet.data),1,packet_size,filein))
      {
      int i=0;
      packet.index = packet_index;
      encryption(&packet);
      while(i<2)
      {
      sendto(sock,&packet,sizeof(packet),0,(struct sockaddr *)&remote,sizeof(remote));
      i++;
      }
      for(int i=0;i<100000;i++);
      packet_index++;
      bzero(&(packet.data),packet_size);
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
        char *files_list =  malloc(25*(sizeof(char)));
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
        while(1)
        {
        //get userinput
         option = user_command();
         while(!option)
         {
           printf("Feature is not implemented.Select from available\n");
           option = user_command();
         }
         //Based on the option do required
         switch(option)
         {
           case GET: 
                     sendto(sock,"get",strlen("get"),0,(struct sockaddr *)&remote,sizeof(remote));
                     printf("Enter the file name\n");
                     scanf("%s",filename);
                     sendto(sock,filename,strlen(filename),0,(struct sockaddr *)&remote,sizeof(remote));
                     size = recv_fileinfo(sock,remote);
                     if(size == 0)
                      {
                       bzero(filename,100);
                       printf("do ls to see the names of files\n");
                       break;
                      }
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
                     if(size == 0)
                     {
                     bzero(filename,100);
                     printf("check the file name\n");
                     break;
                     }
                     if(send_file(sock,remote,filename,size))
                     {
                     printf("sent the file\n");
                     recvfrom(sock,ack,10*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int*)&remote_length);
                     if(!check_ack(sock,remote,ack))
                     printf("could'nt connect to server\n");
                     bzero(filename,100);
                     bzero(ack,10);
                     }
                     else
                     printf("check the file name\n");
                     break;
           
           case DELETE:
                    sendto(sock,"delete",strlen("delete"),0,(struct sockaddr *)&remote,sizeof(remote));
                    printf("Enter the filename\n");
                    scanf("%s",filename);
                    sendto(sock,filename,strlen(filename),0,(struct sockaddr *)&remote,sizeof(remote));
                    nbytes = recvfrom(sock,ack,10*sizeof(char),0,(struct sockaddr *)&remote,(unsigned int*)&remote_length);
                    if(nbytes != 0)
                    if (!check_ack(sock,remote,ack))
                    printf("could'nt delete file do ls\n");
                    bzero(filename,100);
                    bzero(ack,10);
                    break;
            
         case LIST_FILES:

                     files_list = "files.txt";
                     FILE *list = NULL;
                     char *list_buffer =malloc(60*sizeof(char));
                     sendto(sock,"ls",strlen("ls"),0,(struct sockaddr *)&remote,sizeof(remote));
                     size = recv_fileinfo(sock,remote);
                     if(size==0)
                     {
                     printf("empty server\n");
                     break;
                     }
                     if(recv_file(sock,remote,files_list,size))
                     {
                     sendto(sock,"ACK",strlen("ACK"),0,(struct sockaddr *)&remote,sizeof(remote));
                     printf("successfully recieved the file\n");
                     list = fopen(files_list,"r");
                     while(fread(list_buffer,1,size,list))
                     {
                     printf("List of files in buffer are ...\n");
                     printf("******************************************\n");
                     printf("%s",list_buffer);
                     printf("******************************************\n");
                     }
                     }
                     fclose(list);
                     break;

           case EXIT:
                   sendto(sock,"exit",strlen("exit"),0,(struct sockaddr *)&remote,sizeof(remote));
                   printf("closing along server\n");
                   close(sock);
                   return 0;
                   break;
                   
        }
     }
   close(sock);
}

