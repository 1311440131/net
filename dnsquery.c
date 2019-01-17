//DNS Query Program on Linux
//Author : Silver Moon ([email protected])
//Dated : 29/4/2009

//Header Files
#include<stdio.h>	//printf
#include<string.h>	//strlen
#include<stdlib.h>	//malloc
#include<sys/socket.h>	//you know what this is for
#include<arpa/inet.h>	//inet_addr , inet_ntoa , ntohs etc
#include<netinet/in.h>
#include<unistd.h>	//getpid
#include"syswrapper.h"
#define MAX_CONNECT_QUEUE   1024

//List of DNS Servers registered on the system
char dns_servers[10][100];
int dns_server_count = 0;
//DNS result
char dns_result[100];
//Types of DNS resource records :)

#define T_A 1 //Ipv4 address
#define T_NS 2 //Nameserver
#define T_CNAME 5 // canonical name
#define T_SOA 6 /* start of authority zone */
#define T_PTR 12 /* domain name pointer */
#define T_MX 15 //Mail server

//Function Prototypes
void ngethostbyname (unsigned char* , int);
void ChangetoDnsNameFormat (unsigned char*,unsigned char*);
unsigned char* ReadName (unsigned char*,unsigned char*,int*);
void get_dns_servers();
int httpClient();
static int __Hello();

//DNS header structure
struct DNS_HEADER
{
	unsigned short id; // identification number

	unsigned char rd :1; // recursion desired
	unsigned char tc :1; // truncated message
	unsigned char aa :1; // authoritive answer
	unsigned char opcode :4; // purpose of message
	unsigned char qr :1; // query/response flag

	unsigned char rcode :4; // response code
	unsigned char cd :1; // checking disabled
	unsigned char ad :1; // authenticated data
	unsigned char z :1; // its z! reserved
	unsigned char ra :1; // recursion available

	unsigned short q_count; // number of question entries
	unsigned short ans_count; // number of answer entries
	unsigned short auth_count; // number of authority entries
	unsigned short add_count; // number of resource entries
};

//Constant sized fields of query structure
struct QUESTION
{
	unsigned short qtype;
	unsigned short qclass;
};

//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA
{
	unsigned short type;
	unsigned short _class;
	unsigned int ttl;
	unsigned short data_len;
};
#pragma pack(pop)

//Pointers to resource record contents
struct RES_RECORD
{
	unsigned char *name;
	struct R_DATA *resource;
	unsigned char *rdata;
};

//Structure of a Query
typedef struct
{
	unsigned char *name;
	struct QUESTION *ques;
} QUERY;

//int main( int argc , char *argv[])
int Gethostbyname()
{
	unsigned char hostname[100] = "github.com";
	dns_result[0]='1';
	dns_result[1]='9';
	dns_result[2]='2';
	dns_result[3]='.';
	dns_result[4]='3';
	dns_result[5]='0';
	dns_result[6]='.';
	dns_result[7]='2';
	dns_result[8]='5';
	dns_result[9]='5';
	dns_result[10]='.';
	dns_result[11]='1';
	dns_result[12]='1';
	dns_result[13]='3';
	httpClient();
	//Get the DNS servers from the resolv.conf file
	get_dns_servers();
	
	//Get the hostname from the terminal
	//printf("Enter Hostname to Lookup : ");
	//scanf("%s" , hostname);
	
	
	//Now get the ip of this hostname , A record
	ngethostbyname(hostname , T_A);
	
	return 0;
}

/*
 * Perform a DNS query by sending a packet
 * */
void ngethostbyname(unsigned char *host , int query_type)
{
	unsigned char buf[65536],*qname,*reader;
	int i , j , stop , s;

	struct sockaddr_in a;

	struct RES_RECORD answers[20],auth[20],addit[20]; //the replies from the DNS server
	struct sockaddr_in dest;

	struct DNS_HEADER *dns = NULL;
	struct QUESTION *qinfo = NULL;

	printf("Resolving %s" , host);

	s = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP); //UDP packet for DNS queries

	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr.s_addr = inet_addr(dns_servers[0]); //dns servers

	//Set the DNS structure to standard queries
	dns = (struct DNS_HEADER *)&buf;

	dns->id = (unsigned short) htons(getpid());
	dns->qr = 0; //This is a query
	dns->opcode = 0; //This is a standard query
	dns->aa = 0; //Not Authoritative
	dns->tc = 0; //This message is not truncated
	dns->rd = 1; //Recursion Desired
	dns->ra = 0; //Recursion not available! hey we dont have it (lol)
	dns->z = 0;
	dns->ad = 0;
	dns->cd = 0;
	dns->rcode = 0;
	dns->q_count = htons(1); //we have only 1 question
	dns->ans_count = 0;
	dns->auth_count = 0;
	dns->add_count = 0;

	//point to the query portion
	qname =(unsigned char*)&buf[sizeof(struct DNS_HEADER)];

	ChangetoDnsNameFormat(qname , host);
	qinfo =(struct QUESTION*)&buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; //fill it

	qinfo->qtype = htons( query_type ); //type of the query , A , MX , CNAME , NS etc
	qinfo->qclass = htons(1); //its internet (lol)

	printf("\nSending Packet...");
	if( sendto(s,(char*)buf,sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION),0,(struct sockaddr*)&dest,sizeof(dest)) < 0)
	{
		perror("sendto failed");
	}
	printf("Done");
	
	//Receive the answer
	i = sizeof dest;
	printf("\nReceiving answer...");
	if(recvfrom (s,(char*)buf , 65536 , 0 , (struct sockaddr*)&dest , (socklen_t*)&i ) < 0)
	{
		perror("recvfrom failed");
	}
	printf("Done");

	dns = (struct DNS_HEADER*) buf;

	//move ahead of the dns header and the query field
	reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname)+1) + sizeof(struct QUESTION)];

	printf("\nThe response contains : ");
	printf("\n %d Questions.",ntohs(dns->q_count));
	printf("\n %d Answers.",ntohs(dns->ans_count));
	printf("\n %d Authoritative Servers.",ntohs(dns->auth_count));
	printf("\n %d Additional records.\n\n",ntohs(dns->add_count));

	//Start reading answers
	stop=0;

	for(i=0;i<ntohs(dns->ans_count);i++)
	{
		answers[i].name=ReadName(reader,buf,&stop);
		reader = reader + stop;

		answers[i].resource = (struct R_DATA*)(reader);
		reader = reader + sizeof(struct R_DATA);

		if(ntohs(answers[i].resource->type) == 1) //if its an ipv4 address
		{
			answers[i].rdata = (unsigned char*)malloc(ntohs(answers[i].resource->data_len));

			for(j=0 ; j<ntohs(answers[i].resource->data_len) ; j++)
			{
				answers[i].rdata[j]=reader[j];
			}

			answers[i].rdata[ntohs(answers[i].resource->data_len)] = '\0';

			reader = reader + ntohs(answers[i].resource->data_len);
		}
		else
		{
			answers[i].rdata = ReadName(reader,buf,&stop);
			reader = reader + stop;
		}
	}

	//read authorities
	for(i=0;i<ntohs(dns->auth_count);i++)
	{
		auth[i].name=ReadName(reader,buf,&stop);
		reader+=stop;

		auth[i].resource=(struct R_DATA*)(reader);
		reader+=sizeof(struct R_DATA);

		auth[i].rdata=ReadName(reader,buf,&stop);
		reader+=stop;
	}

	//read additional
	for(i=0;i<ntohs(dns->add_count);i++)
	{
		addit[i].name=ReadName(reader,buf,&stop);
		reader+=stop;

		addit[i].resource=(struct R_DATA*)(reader);
		reader+=sizeof(struct R_DATA);

		if(ntohs(addit[i].resource->type)==1)
		{
			addit[i].rdata = (unsigned char*)malloc(ntohs(addit[i].resource->data_len));
			for(j=0;j<ntohs(addit[i].resource->data_len);j++)
			addit[i].rdata[j]=reader[j];

			addit[i].rdata[ntohs(addit[i].resource->data_len)]='\0';
			reader+=ntohs(addit[i].resource->data_len);
		}
		else
		{
			addit[i].rdata=ReadName(reader,buf,&stop);
			reader+=stop;
		}
	}

	//print answers
	printf("\nAnswer Records : %d \n" , ntohs(dns->ans_count) );
	for(i=0 ; i < ntohs(dns->ans_count) ; i++)
	{
		printf("Name : %s ",answers[i].name);

		if( ntohs(answers[i].resource->type) == T_A) //IPv4 address
		{
			long *p;
			p=(long*)answers[i].rdata;
			a.sin_addr.s_addr=(*p); //working without ntohl
			printf("has IPv4 address : %s\n",inet_ntoa(a.sin_addr));//answer addr

			/* add http here */
			
			strcpy(dns_result, inet_ntoa(a.sin_addr));
			httpClient();
		}
		
		if(ntohs(answers[i].resource->type)==5) 
		{
			//Canonical name for an alias
			printf("has alias name : %s",answers[i].rdata);
		}

		printf("\n");
	}

	//print authorities
	printf("\nAuthoritive Records : %d \n" , ntohs(dns->auth_count) );
	for( i=0 ; i < ntohs(dns->auth_count) ; i++)
	{
		
		printf("Name : %s ",auth[i].name);
		if(ntohs(auth[i].resource->type)==2)
		{
			printf("has nameserver : %s",auth[i].rdata);
		}
		printf("\n");
	}

	//print additional resource records
	printf("\nAdditional Records : %d \n" , ntohs(dns->add_count) );
	for(i=0; i < ntohs(dns->add_count) ; i++)
	{
		printf("Name : %s ",addit[i].name);
		if(ntohs(addit[i].resource->type)==1)
		{
			long *p;
			p=(long*)addit[i].rdata;
			a.sin_addr.s_addr=(*p);
			printf("has IPv4 address : %s",inet_ntoa(a.sin_addr));
		}
		printf("\n");
	}
	return;
}

/*
 * 
 * */
u_char* ReadName(unsigned char* reader,unsigned char* buffer,int* count)
{
	unsigned char *name;
	unsigned int p=0,jumped=0,offset;
	int i , j;

	*count = 1;
	name = (unsigned char*)malloc(256);

	name[0]='\0';

	//read the names in 3www6google3com format
	while(*reader!=0)
	{
		if(*reader>=192)
		{
			offset = (*reader)*256 + *(reader+1) - 49152; //49152 = 11000000 00000000 ;)
			reader = buffer + offset - 1;
			jumped = 1; //we have jumped to another location so counting wont go up!
		}
		else
		{
			name[p++]=*reader;
		}

		reader = reader+1;

		if(jumped==0)
		{
			*count = *count + 1; //if we havent jumped to another location then we can count up
		}
	}

	name[p]='\0'; //string complete
	if(jumped==1)
	{
		*count = *count + 1; //number of steps we actually moved forward in the packet
	}

	//now convert 3www6google3com0 to www.google.com
	for(i=0;i<(int)strlen((const char*)name);i++) 
	{
		p=name[i];
		for(j=0;j<(int)p;j++) 
		{
			name[i]=name[i+1];
			i=i+1;
		}
		name[i]='.';
	}
	name[i-1]='\0'; //remove the last dot
	return name;
}

/*
 * Get the DNS servers from /etc/resolv.conf file on Linux
 * */
void get_dns_servers()
{
	FILE *fp;
	char line[200] , *p;
	if((fp = fopen("/etc/resolv.conf" , "r")) == NULL)
	{
		printf("Failed opening /etc/resolv.conf file \n");
	}
	
	while(fgets(line , 200 , fp))
	{
        printf("%s",line);
		if(line[0] == '#')
		{
			continue;
		}
		if(strncmp(line , "nameserver" , 10) == 0)
		{
			p = strtok(line , " ");
			p = strtok(NULL , " ");
			
			//p now is the dns ip :)
			printf("dns ip:%s",p);
		}
	}
    	
	strcpy(dns_servers[0] , "10.0.2.3");
	strcpy(dns_servers[1] , "208.67.220.220");
}

/*
 * This will convert www.google.com to 3www6google3com 
 * got it :)
 * */
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host) 
{
	int lock = 0 , i;
	strcat((char*)host,".");
	
	for(i = 0 ; i < strlen((char*)host) ; i++) 
	{
		if(host[i]=='.') 
		{
			*dns++ = i-lock;
			for(;lock<i;lock++) 
			{
				*dns++=host[lock];
			}
			lock++; //or lock=i+1;
		}
	}
	*dns++='\0';
}


//here add http to github



int httpClient() 
{

        int     sSocket;
        struct sockaddr_in stSvrAddrIn; /* 服务器端地址 */
	struct sockaddr_in clientAddr;/*client addr*/
        char        sndBuf[1024] = {0};
        char        rcvBuf[2048] = {0};
        char       *pRcv         = rcvBuf;
        int         num          = 0;
        int         nRet         ;
	char	*dst_ip;
	int	ip_lenth;
//DNS_ip is dst_ip	
	ip_lenth = strlen(dns_result);
	dst_ip = (char *)malloc(ip_lenth*sizeof(char));
	dst_ip = dns_result;
        /* HTTP 消息构造开始,这是程序的关键之处 */
/*
        sprintf(sndBuf, "GET / HTTP/1.1\n");
        strcat(sndBuf, "Host: www.163.com\r\n");
	strcat(sndBuf, "User-Agent: curl/7.16.3 libcurl/7.16.3 OpenSSL/0.9.7l zlib/1.2.3\r\n");
	strcat(sndBuf, "Accept-Language: en, mi\r\n");
*/
	sprintf(sndBuf,"GET / HTTP/1.1\r\n");
	strcat(sndBuf,"Accept:html/text*/*\r\n");
	strcat(sndBuf,"Accept-language:zh-ch\r\n");
	strcat(sndBuf,"Accept-Encoding:gzip,deflate\r\n");

//	strcat(sndBuf,"Host: 180.97.33.107:80\r\n");
//	strcat(sndBuf,"Host: 13.250.177.223\r\n");
	strcat(sndBuf,"Host: ");
	strcat(sndBuf,dst_ip);
	strcat(sndBuf,"\r\n");
	strcat(sndBuf,"User-Agent:client<1.0>\r\n");
	strcat(sndBuf,"Connection:Close\r\n");
	strcat(sndBuf,"\r\n");
	printf("sndBuf = %s\n",sndBuf);

        /* HTTP 消息构造结束 */
        /* socket DLL初始化 */
     
     stSvrAddrIn.sin_family      = AF_INET;
     stSvrAddrIn.sin_port        = htons(80);
     stSvrAddrIn.sin_addr.s_addr = inet_addr(dst_ip);//www.baidu.com 180.97.33.107

     clientAddr.sin_family = AF_INET;
     clientAddr.sin_port = htons(8080);
     clientAddr.sin_addr.s_addr = inet_addr("10.0.2.15");//10.0.2.15 MenuOS


     sSocket = socket(AF_INET, SOCK_STREAM, 0);

     bind(sSocket, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
          /* 连接 */
	__Hello();//fake http
     nRet = connect(sSocket, (struct sockaddr *)&stSvrAddrIn, sizeof(stSvrAddrIn));
        if (nRet<0)
        {
               printf("connect fail!/n");
               return -1;
        }
               /* 发送HTTP请求消息 */

        send(sSocket, (char*)sndBuf, sizeof(sndBuf), 0);
          /* 接收HTTP响应消息 */
        while(1)
        {
               num = recv(sSocket, pRcv, 2048, 0);
                 pRcv += num;
              if((0 == num) || (-1 == num))
               {
                      break ;
               }
	
        }
               /* 打印响应消息 */
        printf("%s/n", rcvBuf);
          return 0; 
} 

static int __Hello(int argc, char *argv[])
{
	char szBuf[MAX_BUF_LEN] = "\0";
	char szMsg[MAX_BUF_LEN] = "hello\0";
	OpenRemoteService();
	SendMsg(szMsg);
	RecvMsg(szBuf);
	CloseRemoteService();
	return 0;
}