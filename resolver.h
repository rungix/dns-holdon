#ifndef __RESOLVER_TEST_H_INCLUDE_GUARD__
#define __RESOLVER_TEST_H_INCLUDE_GUARD__

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define LEN_NAME 64 

/*
typedef enum _boolean{
    =0, 
    = 1
} BOOL;

*/

typedef enum _dns_software{
    dns_BIND = 1,
    dns_UNBOUND =2,
    dns_OTHER =0,
} DNSSoftware;

typedef enum _country_code{
    cc_us = 0,
    cc_cn = 1,
    cc_jp = 2,
    cc_de = 3,
    cc_other =255
} CountryCode;
     
typedef struct _resolver{
    char name[LEN_NAME];
    in_addr_t ipaddr;
    int port_tcp;
    int port_udp;
    int rtt;
    int rtt_dev;
    int ttl;
    BOOL ipv6;
    BOOL dnssec_ok;
    BOOL forwarder;
    unsigned long time_lasttest;
    BOOL open; 
    DNSSoftware software;
    char version[LEN_NAME]; 
    int censored;
    char * key;
    char * algorithm;
    char * session_key;

    CountryCode country;
} DNSResolver;



DNSResolver * resolver_new(char * name, char *ip, int port);
int resolver_test(DNSResolver *resolver, char *testname, char *rrtype); 

#endif 
