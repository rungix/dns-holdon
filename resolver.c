#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "common.h"
#include "resolver.h"


DNSResolver * resolver_new(char * ipstr, int port_udp, int port_tcp)
{
    DNSResolver *rsv;
    int sock;
    struct sockaddr_in ns_addr;
    int ttl, rtt;

    if (ipstr == NULL)
        return NULL; 
    if ((rsv= malloc(sizeof(DNSResolver))) ==NULL)
        return NULL;

    strncpy(rsv->ipstr, ipstr, LEN_NAME);

    if( inet_pton(AF_INET, ipstr, &rsv->ipaddr) != 1) 
    {
        free(rsv);
        return NULL;
    }
    if ( port_udp !=0 )
        rsv->port_udp = port_udp;
    else
        rsv->port_udp = 53;
    if ( port_tcp !=0 )
        rsv->port_tcp = port_tcp;
    else
        rsv->port_tcp = 53;

    return rsv;
}

/* To test 
    -TTL?
    -RTT?
    -recursive or not?
    -DNSSEC OK?
    -tcp open ?
    -software and version ?
    -forwarder?
*/
/*
int resolver_test(DNSResolver* rsv)
{
    
    //make a query with good name, and send it to the server;
    char *qname; 
    int queryLen, responseLen, id_query, id_response;
    u_char query_buffer[NS_MAXMSG], response_buffer[NS_MAXMSG];
    ns_msg handle;
    ns_rr rr;
    struct sockaddr_in ns_addr;
    int sock, rcode, flag;
    unsigned int set;

    if (rsv == NULL )
        return -1;
    
    if ( (sock = CreateClientSocket(rsv->ipstr, SOCK_DGRAM, rsv->port_udp, &ns_addr))<0){
        my_log("ERROR: measure_tt CreateClientSocket error.\n");
        return -1;
    }

    struct timeval timeout = {5, 0};
    rcode = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    set =1;
    if( (rcode = setsockopt(sock, IPPROTO_IP, IP_RECVTTL, &set, sizeof(set)))<0) {
        my_log("ERROR: measure_tt setsockopt error.\n");
        close(sock);
        *ttl = -1;
        return -1;
    } 
    if( (queryLen = res_mkquery(QUERY, testname, ns_c_in, ns_t_a,
                 NULL, 0, NULL,(u_char *) query_buffer, sizeof (query_buffer))) <0) {
        my_log("ERROR: measurement ttl error, unable to re_mkquery\n");
        close(sock);
        *ttl = -1;
        return -1;
    }


    memset(&handle, 0, sizeof(handle));
    if( ns_initparse(  query_buffer, queryLen, &handle) <0) {
        my_log("ERROR: measure_ttl parse query error:%s responseLen=%d\n", strerror(errno), queryLen);
        close(sock);
        *ttl = -1;
        return -1;
    }
    id_query = ns_msg_id(handle);

    sendto(sock, query_buffer, queryLen, 0,
                (struct sockaddr *) &ns_addr, sizeof(struct sockaddr_in));
    
    //debug("measure_ttl sent query :%s to %s, waiting for reply...\n", testname, nameserver);
    //get answer and the TTL
    bzero(response_buffer, sizeof(response_buffer));
    responseLen = 0;
    if ( (responseLen = recvmsg_with_ttl_m(sock, (char *) response_buffer, NS_MAXMSG, ttl)  )<0 ) {
        my_log ("ERROR: measure_ttl receive data  error.\n")
        close(sock);
        *ttl = -1;
        return -1;
    }
    //parse the response to handle, 
    memset(&handle, 0, sizeof(handle));
    if( ns_initparse(  response_buffer, responseLen, &handle) <0) {
        my_log("ERROR: measure_ttl parse response error:%s responseLen=%d\n", strerror(errno), responseLen);
        close(sock);
        *ttl = -1;
        return -1;
    }

    //get id, client_addr
    id_response=ns_msg_id(handle);
    flag = ns_msg_getflag(handle, ns_f_rcode);
    if (flag != ns_r_noerror){
        my_log("ERROR: nameserver: %s returned rcode: %d for query %s(A)\n", 
                nameserver, flag, testname );
    }
    ns_parserr(&handle, ns_s_qd,0, &rr);
    qname=ns_rr_name(rr);
    if (id_query != id_response) {
       my_log("ERROR: ID mismatch.\n"); 
    }

    if(ns_parserr(&handle, ns_s_an, 0, &rr)<0) {
        my_log("ERROR: measure_ttl got no A record for name:%s from nameserver:%s\n",
                qname, nameserver);
    }
    close(sock);
    return 0;
}

*/

FILE * fd_log ;

void main(int argc, char * argv[])
{
    int i;
    DNSResolver *rsv[2];
 
    fd_log = stderr;
    rsv[0] = resolver_new("8.8.8.8", 0, 0);
    rsv[1] = resolver_new("202.112.57.5", 5335, 53) ;
    
    for (i=0; i<2; i++)
    {
        printf("resolver %d: ip: %s, port_udp:%d\n",i, rsv[i]->ipstr, rsv[i]->port_udp);
    }
}
