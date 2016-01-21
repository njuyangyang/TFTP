#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "team13_struct.h"
#include <time.h>

#define MYPORT "5000" // the port users will be connecting to
#define MAXFILE 33554432

void senderror(int socket,short unsigned error_code,struct sockaddr_in* client_addr_ptr,socklen_t addr_len){
	struct error_packet errorPacket;
	short unsigned opcode;
	//printf("the errorcode is %d\n.",error_code);
	//error_code=htons(error_code);
	errorPacket.error_code=htons(error_code);
	opcode=5;
	opcode=htons(opcode);
	errorPacket.opcode=opcode;
	//printf("1:%s\n",errormsg[error_code]);
	strcpy(errorPacket.error_msg,errormsg[error_code]);
	//printf("2:%s\n",errormsg[error_code]);
	printf("send ERROR no.%d to the socket %d\n",error_code,socket);
	sendto(socket,&errorPacket,sizeof(errorPacket),0,(struct sockaddr*)client_addr_ptr,addr_len);
}


int senddata(node* listTemp,int resultACK,socklen_t addr_len){
	node* temp_ptr=listTemp;
	int lastsize,block_no;// put lastsize into node
	short unsigned opcode;
	struct data_packet dataPacket;		
		
	opcode=3;//packet opcode
	opcode=htons(opcode);
	dataPacket.opcode=opcode;

	


	/* lastsize=fread(&dataPacket.data,1,512,temp_ptr->fp_local); *///how many bytes have been read  //packet data

	if(!resultACK)//not a duplicate ACK
	{
		if(temp_ptr->lastsize<512){
			close(temp_ptr->clientfd);//close the fd 
			printf("complete the transmission to the socket %d\n",temp_ptr->clientfd);
			return temp_ptr->clientfd;//return the fd also as the boolean value of lastPacket
		}
		
		else{
			lastsize=fread(dataPacket.data,1,512,temp_ptr->fp_local);
			//printf("the packet size is %d\n",lastsize+4);
			//printf("%s\n",dataPacket.data);
			temp_ptr->lastsize=lastsize;//return lastsize to node
			block_no=htons(temp_ptr->block_number);//packet block_no
			dataPacket.block_number=block_no;
			printf("send block no.%d to the socket %d\n",temp_ptr->block_number,temp_ptr->clientfd);
			sendto(temp_ptr->clientfd,&dataPacket,lastsize+4,0,(struct sockaddr*)&(temp_ptr->client_addr),addr_len);//send a normal packet
			temp_ptr->block_number+=1;
			/* temp_ptr->position=ftell(temp_ptr->fp_local); */
		
			return 0;
		}
	}
	else{
	//resend the former packet
	fseek(temp_ptr->fp_local,-(temp_ptr->lastsize),1);//move the ptr to former block
	temp_ptr->block_number-=1;
	block_no=htons(temp_ptr->block_number);//packet block_no
	dataPacket.block_number=block_no;
	lastsize=fread(&dataPacket.data,1,512,temp_ptr->fp_local);
	temp_ptr->lastsize=lastsize;//return lastsize to node
	printf("RESEND block no.%d to the socket %d\n",temp_ptr->block_number,temp_ptr->clientfd);
	sendto(temp_ptr->clientfd,&dataPacket,lastsize+4,0,(struct sockaddr*)&(temp_ptr->client_addr),addr_len);//resend last packet smaller than 512 byte
	temp_ptr->block_number+=1;
	return 0;

	}
}



// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//campare two addresses, if same, return 0; otherwise return 1.
int addrCompare(struct sockaddr_in addr1,struct sockaddr_in addr2){
	if((addr1.sin_port==addr2.sin_port) 
	   && (addr1.sin_addr.s_addr==addr2.sin_addr.s_addr))
	   return 0;
	else
	   return 1;
}

//if found a node whose filename and address is same as that of the new one,
//it is a duplicate RQQ, return 1; otherwise is a new RQQ;
int rqqCheck(node* head,char* file_name,struct sockaddr_in client_address){
	node* temp_ptr=head;
	while(temp_ptr!=NULL){
		if(!addrCompare(client_address,temp_ptr->client_addr) 
			&& (!strcmp(file_name,temp_ptr->filename))){
			return 1;
		}
		temp_ptr=temp_ptr->following;
	}
	return 0;
}

//find the corresponding information for the current connection
node* find(node* listHead, int sockfd){
	node* temp_ptr=listHead;
	while(temp_ptr!=NULL){
		if(temp_ptr->clientfd==sockfd){
			return temp_ptr;
		}
		temp_ptr=temp_ptr->following;
	}
	return NULL;
}

//check whether the ack is duplicate, if same as the block_no,
//if perfect, return 0; otherwise packet lost, return 1;
int ackCheck(node* temp_ptr,int block_number){
	if(temp_ptr->block_number-1==block_number){
		return 0;
	}
	else{
		return 1;
	}
}

void listDelete(node* temp_ptr,node** headptr_addr,node** tailptr_addr){
	//node *temp_ptr,*temp_front,*temp_tail;
	if((*headptr_addr)==(*tailptr_addr)){
		*headptr_addr=NULL;
		*tailptr_addr=NULL;
	}
	else{
		if(*headptr_addr==temp_ptr){
			//it is the first nod
			*headptr_addr=(*headptr_addr)->following;
			(*headptr_addr)->previous=NULL;
		}
		else{//it is the tail or in the body
			if(*tailptr_addr==temp_ptr){
				*tailptr_addr=(*tailptr_addr)->previous;
				(*tailptr_addr)->following=NULL;
			}
			else{//it is in the body
				temp_ptr->previous->following=temp_ptr->following;
				temp_ptr->following->previous=temp_ptr->previous;
			}
		}
	}

	free(temp_ptr);
}

struct timeval timeMinus(struct timeval time1, struct timeval time2){
	//this program execute minus
	struct timeval result;
	int timeTotal;
	timeTotal=1000000*(time1.tv_sec-time2.tv_sec)+time1.tv_usec-time2.tv_usec;
	result.tv_sec=timeTotal/1000000;
	result.tv_usec=timeTotal-1000000*result.tv_sec;
	return result;
}

int main(void){
	fd_set master;    //the field to be supervised
	fd_set read_fds;  //the temp field to superviesd by select()
	int fdmax;       //record the max fd used in select()
	int listener;   //the file discriptor for listener
	int newfd;       //the file discriptor for new socket
	int rv;         //result of establishing IP address for getaddrinfo
	int i;          //index in for loop
	int yes=1;      //as a parameter to reuse port
	int nbytes;     //the numbers of bytes received
	int resultCheck; //check the received message is expected
	int lastPacket;   //flag to indicate whether it is the last packet
	int lastSize;
	int client_portno;
	char client_port[5];

	FILE *fp=NULL;
	int filesize;

	//packet info
	short unsigned opcode;
	short unsigned block_number;
	short unsigned error_code;
	char  msg_body[512];
	char filename[512];

	//address info
	struct addrinfo hints, *servinfo, *clientinfo,*p;
	struct sockaddr_in client_addr;

	//paraments for list to record all clients information
	node* listHead=NULL;
	node* listEnd=NULL;
	node* listTemp=NULL;

	//container for different kinds of data packets
	data_packet dataPacket;
	ack_packet ackPacket;
	error_packet errorPacket;
	generic_packet rqqPacket;

	//time used in select(); and record the time transmission for timeout
	struct timeval tv,start_time,end_time,interval_time;

	//temp container for data received or prepared to send
	char buf[516];
	socklen_t addr_len;
	char remoterIP[INET6_ADDRSTRLEN];

	//use time()
	time_t rawtime;
	struct tm * timeinfo;


	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	//seed to generate random port number
	srand(time(NULL));
	
	//
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}


	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((listener = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
			close(listener);
			perror("listener: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);
	printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof client_addr;

	FD_SET(listener,&master);
	fdmax=listener;

	//initial to a large time period
	tv.tv_sec=10;
  	tv.tv_usec=0;

	for(;;){
		read_fds=master;

		if(select(fdmax+1,&read_fds,NULL,NULL,&tv)==-1){
			perror("\nselect\n");
			exit(4);
		}
		
		//if(!FD_ISSET(listener,&read_fds))
			//printf("listener missiong\n");


		for(i=0;i<=fdmax;i++){
			if(FD_ISSET(i,&read_fds)){
				//we got one connect
				time ( &rawtime );
  				timeinfo = localtime ( &rawtime );
  				printf ( "\n%s =======================\n", asctime (timeinfo));
  				if(i==listener){
  					//it is a new connection
  					//printf("get a new connection\n");

  					memset(buf,'\0',sizeof buf);

  					if((nbytes=recvfrom(listener,buf,516,0,(struct sockaddr *)&client_addr,&addr_len))==-1){
  						//get a wrong packet,do noting
  						printf("get a wrong message!\n");
  					}
  					else{
  						//get a complete message
  						//printf("get a complete message\n");
  						memcpy(&opcode,buf,sizeof(opcode));

  						opcode=ntohs(opcode);
  						//printf("the opcode is %d\n",opcode);
  						if(opcode!=1){
  							//this port is for RQQ only
  							printf("this port is for RQQ only\n");
  						}
  						else{
  							//this is a RQQ expected
  							memset(filename,'\0',sizeof filename);
  							//memcpy(filename,buf+2,512);
  							strcpy(filename,buf+2);
  							//printf("the file name is %s\n",filename);

  							//check duplication,if duplicate return 1, pass return 0;
  							resultCheck=rqqCheck(listHead,filename,client_addr);
  							//printf("the resultCheck is %d\n",resultCheck);
  							if(resultCheck==1){
  								printf("it is a duplicate rqq\n");
  							}
  							else{
  								//create a socket
								client_portno = rand();
								client_portno=client_portno % 1000+5000;
  								memset(client_port,'\0',sizeof client_port);
  								sprintf(client_port,"%d",client_portno);
  								//printf("the client_port is %s\n",client_port);
  								if ((rv = getaddrinfo(NULL, client_port, &hints, &clientinfo)) != 0) {
									fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
									return 1;
								}


								// loop through all the results and bind to the first we can
								for(p = clientinfo; p != NULL; p = p->ai_next) {
									if ((newfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
										perror("listener: socket");
										continue;
									}

									setsockopt(newfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));

									if (bind(newfd, p->ai_addr, p->ai_addrlen) == -1) {
										close(newfd);
										perror("newfd: bind");
										continue;
									}
									break;
								}	

								if (p == NULL) {
									fprintf(stderr, "newfd: failed to bind socket\n");
									return 2;
								}

								freeaddrinfo(clientinfo);

								printf("set new socket %d successfull\n",newfd);

  								if((fp=fopen(filename,"rb"))!=NULL){
  									FILE *fp1=fp;
  									fseek(fp1,0L,2);
  									filesize=ftell(fp1);
  									//fclose(fp1);
  									printf("the file length is %d\n",filesize);
  									if(filesize > MAXFILE){
  										//send an error, file too large
  										senderror(newfd,ILLEGALOPERR,&client_addr,addr_len);
  										close(newfd);
  									}
  									else{
  										//printf("start to creat node\n");
  										//create a node in the list
  										listTemp=(node*)malloc(sizeof(node));
  										listTemp->clientfd=newfd;
  										listTemp->fp_local=fopen(filename,"rb");
  										listTemp->block_number=1;
  										listTemp->filesize=filesize;
  										listTemp->lastsize=512;
  										memset(listTemp->filename,'\0',sizeof filename);
  										strcpy(listTemp->filename,filename);
  										memcpy(&(listTemp->client_addr),&(client_addr),sizeof(client_addr));
  										gettimeofday(&(listTemp->sendtime),0);
  										listTemp->previous=NULL;
  										listTemp->following=NULL;

  										//add the node to the list
  										if(listHead==NULL){
  											listHead=listTemp;
  											listEnd=listTemp;
  										}
  										else{
  											listEnd->following=listTemp;
  											listTemp->previous=listEnd;
  											listEnd=listTemp;
  										}

  										//printf("insert information successfully\n");
  									
  										//form a DATA to send
  										senddata(listTemp,0,addr_len);
  										printf("sent first data successfully\n");



  										
  										FD_SET(newfd,&master);
  										if(newfd>fdmax){
  											fdmax=newfd;
  										}
  									}
  								}
  								else{  									
  									//form a ERROR to send
  									//printf("I can before senderror\n");
  									senderror(newfd,NOTFOUNDERR,&client_addr,addr_len);
  									//printf("I can behind senderror\n");

  									
  									close(newfd);
  								}
  							}
  						}//end of complete RQQ
  					}//end of recerive a complete packet
  				}//end of the message is from listener
  				else{
  					//message from existing connection
  					memset(buf,'\0',sizeof buf);

  					if((nbytes=recvfrom(i,buf,516,0,(struct sockaddr *)&client_addr,&addr_len))==-1){
  						//get a wrong packet,do noting
  						printf("get a wrong message!\n");
  					}
  					else{
  						//get a complete packet
  						//printf("we can be here\n");
  						listTemp=find(listHead,i);
  						memcpy(&opcode,buf,sizeof(opcode));
  						opcode=ntohs(opcode);
  						if(opcode!=4){
  							//this port is for ACK only
  							printf("this port is for ACK only\n");
  						}
  						else{
  							//this is a ACK
  							memcpy(&block_number,buf+2,sizeof(block_number));
  							block_number=ntohs(block_number);

  							//check if the ACK is duplicate, if duplicate, return 1, else return 0;
  							resultCheck=ackCheck(listTemp,block_number);
  							gettimeofday(&(listTemp->sendtime),0);
  							//send DATA packet properly according to the resultCheck; 
  							//if ack for last packet, close the connection, 
  							//return socketfd,otherwise return 0
  							lastPacket=senddata(listTemp,resultCheck,addr_len);

  							if(lastPacket){
  								//last packet has been sent
  								listDelete(listTemp,&listHead,&listEnd);
  								FD_CLR(i,&master);
  							}
  							//else{
  								//listTemp->block_no++;
  							//}
  						}//end for ACK
  					}//end for complete packet
  				}//end for existing connection
			}//end of FD_ISSET
  		}//end of small for loop

  		gettimeofday(&end_time,0);

  		//sent timeout equal to 5s
  		tv.tv_sec=5;
  		tv.tv_usec=0;
  		

  		for(listTemp=listHead;listTemp!=NULL;listTemp=listTemp->following){
  			interval_time=timeMinus(end_time,listTemp->sendtime);
  			//timeout=5s=5000000us
  			//if time interval >timeout
  			if(interval_time.tv_sec * 1000000+interval_time.tv_usec > 5000000){
  				gettimeofday(&(listTemp->sendtime),0);
  				//resend
  				senddata(listTemp,1,addr_len);

  			}
  			else{
  				//record the minimal time
  				if(interval_time.tv_sec * 1000000+interval_time.tv_usec < tv.tv_sec * 1000000+tv.tv_usec){
  					tv.tv_sec=interval_time.tv_sec;
  					tv.tv_usec=interval_time.tv_usec;
  				}
  			}
  		}//end of the second small loop
	}//and of big for loop
}
























