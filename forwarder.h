#ifndef __FORWARDER_H_INCLUDE_GUARD__
#define __FORWARDER_H_INCLUDE_GUARD__

//#include "hashtbl.h"
#include "blacklist.h"
#include "badip.h"

#ifdef __BSD__
#include "bsd_missed.h"
#endif

typedef enum __reply_from {
    OPEN_RESOLVER = 0,
    LOCAL_RESOLVER = 1,
} Reply_From;

typedef enum __validate_result{
    VALIDATE_OK = 0,
    VALIDATE_BAD_IP = 1,
    VALIDATE_NULL_IP  = 2,
    VALIDATE_TTL_MISMATCH = 4,
    VALIDATE_SMALL_RTT = 8,
    VALIDATE_UNKNOWN =16,
    VALIDATE_ERROR = 32,
    VALIDATE_SERVFAIL = 64,
} Validate_Code;

typedef struct _tcp_arg {
    int sockfd;
    char server_ip[32];
    int server_port;
} TCP_ARG;


void signal_handler(int sig);

void * measure_handler(void * arg);
int measure_ttl(int source, char *nameserver, int port, char *testname,  int *ttl );

int forwarder(int local_port, char *open_resolver,int open_resolver_port, 
              char *local_resolver, int local_resolver_port, int openflag); 

#endif


