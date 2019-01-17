#include<stdio.h> 
#include <unistd.h> 
#include<fcntl.h> 
#include<netdb.h> 
#include<stdlib.h> 
#include "httpclient.h" 
#include<string.h> 
#include<sys/types.h> 
#include<sys/socket.h> 
#include <netinet/in.h> 

const char * http_s = "http://"; 

typedef struct{ 

char *url; 
int port; 
char *host; 
char *anyurl; 
int isip; 

}Hostinfo; 

int isip(const char * s){ 

         while( *s ){ 
                 if(isalpha(*s) && *s != '.') 
                  { 
                         return 0; 
                  }      
                  s++;   
         }        
         
         return 1; 
         
} 

int gethostinfobyurl(const char * url, char ** host, int *port, char ** anyurl) 
{ 

if(url == NULL || 0!=strncmp(url, http_s, strlen(http_s)) ){ 

printf("url is not legal, %s\n", url); 

return -1; 
} 


char * h_url = strstr(url,"//"); 

h_url = h_url + 2; 

*anyurl= strchr(h_url,'/'); 

printf("hurl: %s\n",h_url); 
if(anyurl != NULL){ 

char cc[1024]; 
char *pcc = cc; 
strcpy(cc,h_url); 
h_url = strsep(&pcc,"/"); 
printf("hhurl:%s\n",h_url); 
} 

if(NULL !=strchr(h_url,':')){ 

*host = strsep(&h_url,":"); 
char *cport = strsep(&h_url,":"); 
*port = atoi(cport);	
} 
else{ 

*host = h_url; 
} 
return 0; 
} 

void datas(int fp,Hostinfo *hinfo){ 

char request[1024]; 
bzero(&request,1024); 
//sprintf(request,"GET %s  HTTP/1.1\r\nHost:  61.135.169.125:80\r\nConnection:   Close\r\n\r\n","/"); 
sprintf(request,   "GET   %s   HTTP/1.1\r\nAccept:   */*\r\nAccept-Language:   zh-cn\r\n\ 
Host:  %s:%d\r\nConnection:   Close\r\n\r\n ", hinfo->anyurl,hinfo->host,hinfo->port); 

printf("\n\n"); 
printf("%s",request); 

printf("\n\n"); 

printf("request is %s\n", request); 
int send = write(fp, request,strlen(request)); 

if(send == -1){ 

printf("send error\n"); 
exit(1); 
} 


printf("send num: %d\n", send); 
int BUFFER_SIZE = 2048; 
char buffer[BUFFER_SIZE]; 
int length; 
bzero(&buffer, BUFFER_SIZE); 

        length = read(fp,buffer,BUFFER_SIZE); 
char * content = buffer; 


if(length <=0){ 

printf("error! read http return content!"); 

//return NULL; 
} 
char * head = strsep(&content,"\r\n"); 
printf("head:%s\n",head); 

strsep(&head," "); 

printf("1-->>head:%s\n",head); 
if(head == NULL){ 


printf("error! read http return content!"); 
return; 
} 


head = strsep(&head," "); 

printf("2-->>head:%s\n",head); 
int result = atoi(head); 

if(result == 200){ 
printf("success\n"); 

} 

/*while( length = read(fp,buffer,BUFFER_SIZE)){ 
if(length <=0){ 
return ; 
} 
bzero(&buffer,1024); 
} 

*/ 



} 
int connect_http(Hostinfo * hinfo) 
{ 
int cfd; 
struct sockaddr_in s_add; 
        struct sockaddr_in * sinp; 
struct addrinfo hint; 
struct addrinfo *ailist, *aip; 
int i = 0, err; 
char seraddr[128]; 

printf(" hinfo->host: %s\n", hinfo->host); 
  if(!(hinfo -> isip)){ 

  hint.ai_family = AF_INET; 
  hint.ai_socktype = SOCK_STREAM; 
  hint.ai_flags = AI_CANONNAME; 
  hint.ai_protocol = 0; 
  hint.ai_addrlen = 0; 
  hint.ai_addr = NULL; 
  hint.ai_canonname = NULL; 
  hint.ai_next = NULL; 
char name[1024]; 
strcpy(name, hinfo->host); 
      if ((err = getaddrinfo(name, NULL, &hint, &ailist)) != 0) { 
          printf("getaddrinfo error: %s host :%s\n", gai_strerror(err), hinfo->host); 
          exit(-1); 
       } 

for (aip = ailist; i==0 && aip != NULL; aip = aip->ai_next) { 
  
sinp = (struct sockaddr_in *)aip->ai_addr; 
//break; 
         i = i+1; 

if (inet_ntop(sinp->sin_family, &sinp->sin_addr, seraddr, INET_ADDRSTRLEN) != NULL) 
               { 
                    printf("\nserver address is %s\n", seraddr); 
               } 



  } 
} 
else{ 

strcpy(seraddr,hinfo->host); 
} 
cfd = socket(AF_INET,SOCK_STREAM,0); 
if(cfd == -1){ 
printf("socket error\n"); 
return -1;	
} 

struct timeval timeo = {3, 0}; 
socklen_t len = sizeof(timeo); 
setsockopt(cfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len); 
bzero(&s_add, sizeof(struct sockaddr_in)); 
s_add.sin_family=AF_INET;  
s_add.sin_addr.s_addr= inet_addr(seraddr);   
s_add.sin_port=htons(hinfo->port); 

if(-1 == connect(cfd,(struct sockaddr *)(&s_add), sizeof(struct sockaddr)))  
{  
    printf("connect fail !\r\n");  
    return -1;  
}  
  // int flags = fcntl(cfd, F_GETFL, 0);                       //设置异步 
       //fcntl(cfd, F_SETFL, flags | O_NONBLOCK);  
printf("connect ok !\r\n"); 
datas(cfd,hinfo); 
close(cfd); 
return 1; 

} 

char *  get(const char * url) 
{ 
char * host = (char *)malloc(200); 
char * curl = (char *)malloc(200); 
int port = 80; 
int *cport = &port; 
int result = gethostinfobyurl(url,&host,cport,&curl); 

if(result < 0){ 
return NULL; 
} 

printf("parase host:%s\n",host); 
Hostinfo *hinfo = (Hostinfo*)calloc(1,sizeof(Hostinfo)); 

hinfo->port = port; 
hinfo->host = host; 
hinfo->anyurl = curl; 
hinfo->isip = isip(hinfo->host); 
connect_http(hinfo);	
return NULL; 
} 


int main() 
{ 
const char * url = "http://61.135.169.105:80/"; 
/*	char * host = (char *)malloc(200); 
char * curl = (char *)malloc(200); 
int port = 80; 
int *cport = &port; 
gethostinfobyurl(url,&host,cport,&curl); 
printf("host:%s \t address: %s\n",host,curl); 
printf("port : %d\n",port); 
*/	
get(url); 
return 0; 

}