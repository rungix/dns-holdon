#ifndef __COMMON__H__
#define __COMMON__H__


#include <string.h>
#include <netinet/in.h>

#define MAX_LINE 1024
#define MAX_DNS_MSG 4096
#define MAX_TCP_CLIENTS 32 

#define ISSPACE(x)  ( (x == ' ' ) || (x =='\n')|| (x == '\r') || (x == '\t'))
#define MAX2(a,b)  a>b?a:b
                     

#define BOOL unsigned char
#define TRUE  1
#define FALSE 0
/*
#define my_log(fmt, ...) {print_time(fd_log); fprintf(fd_log, fmt, ##__VA_ARGS__) ;fflush(fd_log);} 
*/

#define LOG_NONE    0
#define LOG_ERROR   1
#define LOG_QUERY   2
#define LOG_FORWARD 4
#define LOG_REPLY   8
#define LOG_INFO    16
#define LOG_MEASURE 32 
#define LOG_DEBUG   64 

#define my_log(level, fmt, ...) {\
            if( level & log_level){print_time(fd_log); fprintf(fd_log, fmt, ##__VA_ARGS__) ;fflush(fd_log); }\
        }

#ifdef DEBUG
   #define debug(fmt, ...) {fprintf(fd_log, "Debug: "); fprintf(fd_log, fmt, ##__VA_ARGS__);fflush(fd_log); }   
   #define debug_point(msg){fprintf(fd_log,"SFSG:%s(%d):%s\n",__FILE__, __LINE__, msg);fflush(fd_log);} 
#else
   #define debug(fmt, ...)    
   #define debug_point(fmt, ...) 
#endif

char * strReverse(char *srcStr, char* dstStr);
char * strTrim(char * s);

ssize_t readn(int fd, void *vptr, size_t n);
ssize_t  writen(int fd, const void *vptr, size_t n);
void print_time (FILE * fd) ;
int CreateClientSocket(char * server_address,int protocol, int server_port, struct sockaddr_in * addr_struct);
int CreateServerSocket(int protocol, int port, struct sockaddr_in * addr, int openflag);
int initialize_random_number(int array[], int range);
int get_random(int array[], int range);
unsigned long getMillisecond();
int recvmsg_with_ttl(int sock, char * recv_buffer, int size , int *ttl);
int recvmsg_with_ttl_m(int sock, char * recv_buffer, int size , int *ttl);

extern int log_level;
extern FILE * fd_log;

#endif

