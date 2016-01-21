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
#include <time.h>

#ifndef _TFTP_H
#define _TFTP_H

enum packet_type {RRQ=1, WRQ, DATA, ACK, ERROR};

enum filemode {NETASCII, OCTET, MAIL};
static char *filemode[] = {
    "netascii",
    "octet",
    "mail"
};

enum error_codes {NOTDEFERR, NOTFOUNDERR, ACCESSERR, DISKFULLERR, 
        		  ILLEGALOPERR, UNKNOWNIDERR, FILEEXISTSERR, 
		          UNKNOWNUSERERR};

static char *errormsg[] = {
    "Not defined, see error message",
    "File not found",
    "Access violation",
    "Disk full or allocation exceeded",
    "Illegal TFTP operation",
    "Unknown transfer ID",
    "File already exists",
    "No such user"
};

typedef
    struct generic_packet {
        short unsigned opcode;
        char info[512];
    } generic_packet;

typedef
    struct data_packet {
        short unsigned opcode;  /* 3 */
        short unsigned block_number;
        char data[512];
    } data_packet;

typedef
    struct ack_packet {
        short unsigned opcode;  /* 4 */
        short unsigned block_number;
    } ack_packet;

typedef
    struct error_packet {
        short unsigned opcode;  /* 5 */
        short unsigned error_code;
        char error_msg[512];
    } error_packet;

typedef
    struct information_node{
        int clientfd;
        FILE* fp_local;
        int block_number;
        int filesize;
        int lastsize;
        char filename[512];
        struct sockaddr_in client_addr;
        struct timeval sendtime;
        struct information_node* previous;
        struct information_node* following;
    }node;

#endif