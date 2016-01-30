#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <pthread.h>

#include "common.h"
#include "config.h"
#include "forwarder.h"
#include "blacklist.h"
#include "hashtbl.h"
#include "badip.h"
#include "resolver.h"

#include "query_record.h"
extern int h_errno; //for resolver errors
extern int errno;


extern FILE * fd_log;
extern struct timeval timeout;
extern Blacklist *blacklist;
extern HASHTBL *badip;
extern Configuration config;

int sock_service, sock_tcpservice, sock_local_ns, sock_open_ns;

int randnum[MAX_QUERY_ID +1 ];
QR  history[MAX_QUERY_ID +1 ];

//int goodTTL[MAX2(LOCAL_RESOLVER, OPEN_RESOLVER)];
extern int goodTTL[];

char * servername[2];

int measure_ttl(int source, char *nameserver, int port, char *testname,  int *ttl )
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

    if ( (sock = CreateClientSocket(nameserver, SOCK_DGRAM, port, &ns_addr))<0){
        my_log(LOG_ERROR, "E: measure_tt CreateClientSocket error.\n");
        *ttl = -1;
        return -1;
    }

    struct timeval timeout = {15, 0};
    rcode = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    set =1;
    if( (rcode = setsockopt(sock, IPPROTO_IP, IP_RECVTTL, &set, sizeof(set)))<0) {
        my_log(LOG_ERROR, "E: measure_tt setsockopt error.\n");
        close(sock);
        *ttl = -1;
        return -1;
    }
    if( (queryLen = res_mkquery(QUERY, testname, ns_c_in, ns_t_a,
                 NULL, 0, NULL,(u_char *) query_buffer, sizeof (query_buffer))) <0) {
        my_log(LOG_ERROR, "E: measurement ttl error, unable to re_mkquery\n");
        close(sock);
        *ttl = -1;
        return -1;
    }


    memset(&handle, 0, sizeof(handle));
    if( ns_initparse(  query_buffer, queryLen, &handle) <0) {
        my_log(LOG_ERROR, "E: measure_ttl parse query error:%s responseLen=%d\n", strerror(errno), queryLen);
        close(sock);
        *ttl = -1;
        return -1;
    }
    id_query = ns_msg_id(handle);

    sendto(sock, query_buffer, queryLen, 0,
                (struct sockaddr *) &ns_addr, sizeof(struct sockaddr_in));

    //get answer and the TTL
    bzero(response_buffer, sizeof(response_buffer));
    responseLen = 0;
    if ( (responseLen = recvmsg_with_ttl_m(sock, (char *) response_buffer, NS_MAXMSG, ttl)  )<0 ) {
        my_log (LOG_ERROR, "E: measure_ttl receive data  error, Name server: %s port :%d \n", nameserver, port)
        close(sock);
        *ttl = -1;
        return -1;
    }
    //parse the response to handle,
    memset(&handle, 0, sizeof(handle));
    if( ns_initparse(  response_buffer, responseLen, &handle) <0) {
        my_log(LOG_ERROR, "E: measure_ttl parse response error:%s responseLen=%d\n", strerror(errno), responseLen);
        close(sock);
        *ttl = -1;
        return -1;
    }

    //get id, client_addr
    id_response=ns_msg_id(handle);
    flag = ns_msg_getflag(handle, ns_f_rcode);
    if (flag != ns_r_noerror){
        my_log(LOG_ERROR, "E: nameserver: %s returned rcode: %d for query %s(A)\n",
                nameserver, flag, testname );
    }
    ns_parserr(&handle, ns_s_qd,0, &rr);
    qname=ns_rr_name(rr);
    if (id_query != id_response) {
       my_log(LOG_ERROR, "E: meaurement got reply ID mismatch with query.\n");
    }

    if(ns_parserr(&handle, ns_s_an, 0, &rr)<0) {
        my_log(LOG_ERROR, "E: measure_ttl got no A record for name:%s from nameserver:%s\n",
                qname, nameserver);
    }
    close(sock);
    return 0;
}

#define NUM_TEST 3
void * measure_handler(void * arg)
{
    int i,j, ttl[2][NUM_TEST], counts[2]={0,0}, fails[2]={0,0}, sums[2]={0,0};

    //    pthread_detach (pthread_self());
    for (;;)
    {
        sleep(config.test_interval);
        for (i=0; i<2; i++){
            counts[i]=0;
            fails[i]=0;
            sums[i]=0;
        }
        for (i=0; i< NUM_TEST; i++){
            if( measure_ttl(OPEN_RESOLVER,  config.open_resolver,
                            config.open_resolver_port, config.testname, &ttl[0][i]) <0)
            {
                my_log(LOG_ERROR, "E: open_resolver test failed: %s\n", config.open_resolver);
                fails[0] ++;
            }
            if( measure_ttl(LOCAL_RESOLVER,  config.local_resolver,
                    config.local_resolver_port,config.testname, &ttl[1][i]) <0)
            {
                my_log(LOG_ERROR, "E: local_resolver test failed: %s\n", config.local_resolver);
                fails[1] ++;
            }
            //debug("TTL0:%d, TTL1:%d\n", ttl[0][i], ttl[1][i]);
            sleep(5);
        }//for (i=0
        for (i=0; i<2; i++){
            for(j=0; j< NUM_TEST; j ++){
                if( ttl[i][j] >=0) {
                    counts[i] ++;
                    sums[i] += ttl[i][j];
                }
            }
//BEGIN_LOCK
            if(counts[i])
                goodTTL[i] = sums[i] / counts[i];
//END_LOCK
        }
        my_log(LOG_MEASURE, "M: M_TTL: open: %d * %d,  local: %d * %d \n", \
                goodTTL[0], counts[0],  goodTTL[1],counts[1] );
    }
}
void signal_handler(int sig)
{
    switch(sig) {

     case SIGHUP:
        my_log(LOG_INFO, "I: Signal Hangup(SIGHUP), re-open logfile and dump blacklist.\n");
        if(config.file_log !=NULL)
        {
            fclose(fd_log);
            fd_log=fopen(config.file_log,"a");
        }

        blacklist_dump(blacklist, config.file_blacklist);
        //badip_dump(badip, config.file_badip);
       break;

     case SIGINT:
     case SIGTERM:
        my_log(LOG_INFO, "I: Signal terminate(SIGTERM), dump and bye.\n");
        blacklist_dump(blacklist, config.file_blacklist);
        blacklist_release(blacklist);
        //badip_dump(badip, config.file_badip);
        badip_release(badip);

        close(sock_service);
        close(sock_open_ns);
        close(sock_local_ns);
        if(config.file_log != NULL)
            fclose(fd_log);
        exit(0); // don't return to the select loop in forwarder()
        break;
    }
}

int process_query(int sock_service, /*int sock_local_ns,*/ struct sockaddr_in *local_ns_addr,
                /*int sock_open_ns,*/ struct sockaddr_in * open_ns_addr)
{
    struct sockaddr_in client_addr;
    socklen_t addrLen;
    u_char query_buffer[NS_MAXMSG];
    int queryLen, n, inBlacklist;
    ns_rr rr;
    char *name, reversedName[NS_MAXDNAME];
    char strAddr[64];
    u_int16_t id, new_id, new_txid;

    ns_msg handle;  /* handle for response message */
    memset(&handle, 0, sizeof(handle));

    memset(query_buffer, 0, sizeof(query_buffer));
    addrLen=sizeof(client_addr);
    queryLen = recvfrom(sock_service, query_buffer, NS_MAXMSG, 0,
                                       (struct sockaddr * )&client_addr, &addrLen);

    if( queryLen <0 )
    {
        my_log(LOG_ERROR, "E: in process_query, recvfrom:%s(%d)\n", strerror(errno), errno);
        return -1;
    }
    if (ns_initparse(query_buffer, queryLen, &handle) < 0)
    {
        my_log(LOG_ERROR, "E: parse query error on ns_initparse: %s, queryLen:%d\n", strerror(errno), queryLen);
        return -1;
    }
    if((n=ns_msg_count(handle, ns_s_qd))<=0)
    {
        my_log(LOG_ERROR, "E: Weird Packet:No Question? queryLen:%d\n", queryLen);
        return -1;
    }

    if ( ns_parserr(&handle, ns_s_qd,0, &rr) <0 )
    {
        my_log(LOG_ERROR, "E: Parse Question error.\n");
        return -1;
    }
    id =ns_msg_id(handle);
    name = ns_rr_name(rr);

    inet_ntop(AF_INET, &client_addr.sin_addr, strAddr,sizeof(strAddr));
    int srcPort = ntohs(client_addr.sin_port);
    my_log(LOG_QUERY, "Q: from: %s:%d name:%s id:%d len:%d\n", strAddr,srcPort,name,id,  queryLen  );

    new_id =get_random(randnum, MAX_QUERY_ID); // for transportation OK

    qr_add(history, id, new_id, name, &client_addr);
    new_txid = htons(new_id);
    memcpy(query_buffer, &new_txid,2); // modify the ID with new_id
    strReverse(name, reversedName);
    inBlacklist=blacklist_lookup(blacklist, reversedName);

    if ( inBlacklist)
    {
        sendto(sock_open_ns, query_buffer, queryLen, 0,
                (struct sockaddr *) open_ns_addr, sizeof(struct sockaddr_in));
        my_log(LOG_FORWARD, "F: to: %s name: %s id: %d\n", servername[OPEN_RESOLVER],name, new_id);
    }
    else
    {
        sendto(sock_local_ns, query_buffer, queryLen, 0,
                (struct sockaddr *) local_ns_addr, sizeof(struct sockaddr_in));
        my_log(LOG_FORWARD, "F: to: %s name: %s id: %d\n",servername[LOCAL_RESOLVER], name, new_id);
    }

    return 0;
}

#define TTL_THRESHOLD 3
//#define BAD_TTL(ttl)  abs(ttl - goodTTL[source] ) > TTL_THRESHOLD
#define BAD_TTL(ttl)  abs(ttl - goodTTL[source] ) > config.ttl_threshold

int validate(ns_msg * handle, Reply_From source, QR * pHistory, int ttl, char * name,  char *ip)
{
   //for GFW only
    int count_an, count_ns, count_ad; //,  count_qd;
    ns_rr       rr;
    const u_char  *rdata;
    char *rname;
    u_int16_t   resFlag, rtype;
    int result ;

    my_log(LOG_DEBUG, "D: expect goodTTL[0]=%d, goodTTL[1]=%d, threshold:%d, to validate:%d\n",
		goodTTL[0], goodTTL[1], config.ttl_threshold, ttl);
    result = VALIDATE_OK; //0
    if (BAD_TTL(ttl) ) {
        my_log(LOG_INFO, "I: TTL mismatch, expect: %d got: %d\n", goodTTL[source], ttl);
        result  = result | VALIDATE_TTL_MISMATCH;
        //continue validate, because the bad ip is helpful
    }

    if( handle == NULL)
        return  result | VALIDATE_ERROR;


    resFlag =  ns_msg_getflag(*handle, ns_f_rcode);
               //ns_msg_get_flag(ns_msg handle, ns_flag flag)
    //count_qd = ns_msg_count(*handle, ns_s_qd);
    count_an = ns_msg_count(*handle, ns_s_an);
    count_ns = ns_msg_count(*handle, ns_s_ns);
    count_ad = ns_msg_count(*handle, ns_s_ar);

    if( ns_parserr(handle, ns_s_qd,0, &rr)<0)
        result = result | VALIDATE_ERROR;


   //debug("Response code:%d, question:%d, answer:%d, name server:%d, additional:%d, From :%d\n",
    //        resFlag, count_qd, count_an, count_ns, count_ad, source);
   if(  ( count_an == 0) && (count_ns == 0) && (count_ad ==0)
      && ( resFlag == ns_r_noerror) && (ns_rr_type(rr) == ns_t_a)  )
    {
        //debug("Found NULL IP .\n");
        return  result | VALIDATE_NULL_IP;
    }

   /* If local resolver return server fail...
    */
   if(  resFlag == ns_r_servfail )
    {
        //my_log(LOG_DEBUG, "D: Resolver:%d returns with SERVFAIL\n", source );
        my_log(LOG_DEBUG, "D: Resolver:%s returns with SERVFAIL\n", servername[source] );
        return  result | VALIDATE_SERVFAIL;
    }


    if ( count_an >= 1)
    {
        if( ns_parserr(handle, ns_s_an,0, &rr)<0)
            return  result | VALIDATE_ERROR;

        rtype = ns_rr_type(rr);

        if (rtype == ns_t_a )
        {
            rdata = ns_rr_rdata(rr);
            rname = ns_rr_name(rr);
            char strIP[64];
            in_addr_t addr ;
            memcpy(&addr, rdata, sizeof(in_addr_t));
            inet_ntop(AF_INET, &addr, strIP, sizeof(strIP));
            strcpy(name, rname);
            strcpy(ip, strIP);
            //debug("Got IP:%s for name: %s\n", strIP, rname);
            if( in_badip(badip, addr))
            {
                my_log(LOG_INFO, "I: BADIP: name:%s IP: %s\n", rname, strIP);
                return result | VALIDATE_BAD_IP;
            }
            if (result & VALIDATE_TTL_MISMATCH)
            {
                my_log(LOG_INFO, "I: BADIP_NEW: name:%s IP: %s\n", rname, strIP);
                return result | VALIDATE_BAD_IP;
            }
        }
    }
    return result;
}
int process_reply(Reply_From source, int sock, struct sockaddr_in * open_ns_addr)
{
    u_char response_buffer[NS_MAXMSG], query_buffer[NS_MAXMSG];
    char *name , query_name[NS_MAXDNAME], answer_ip[64];
    int  responseLen, found, rcode;
    ns_msg handle;
    ns_rr rr;
    u_int16_t new_id, old_id;
    int ttl;

    bzero(response_buffer, sizeof(response_buffer));
    responseLen = 0;

    responseLen = recvmsg_with_ttl(sock, (char *) response_buffer, NS_MAXMSG, &ttl);

    if( responseLen < 0){
        my_log(LOG_ERROR, "E: receive data from %s resolver error, responseLen=%d.\n", servername[source], responseLen);
        return -1;
    }
    my_log(LOG_REPLY, "R: from:%s len:%d ttl:%d\n", servername[source], responseLen, ttl);

    //parse the response to handle,
    memset(&handle, 0, sizeof(handle));
    if( ns_initparse( response_buffer, responseLen, &handle) <0)
    {
        //Because GFW will inject repy with 0 answer, 0 authority and 0 additional, but noerror
        //This kind of packet will fail to pass ns_initparse
        my_log(LOG_INFO, "I: ns_initparse parse error, maybe injection: %s, responseLen=%d\n", \
                strerror(errno), responseLen);
        return -1;
    }

    //get id, client_addr
    new_id=ns_msg_id(handle);
    ns_parserr(&handle, ns_s_qd,0, &rr);
    name=ns_rr_name(rr);
    //lookup from history
    QR * pt = qr_lookup(history, new_id, name, &found);
    if ((pt == NULL )|| (found ==0))
    {
        my_log(LOG_ERROR, "E: qr_lookup not found\n"); //no query for this reply? drop it
        return -1;
    }
    if(found && pt)
    {
        pt->time_reply = getMillisecond();
        pt->num_answers ++;
    }

    memset(query_name, 0, sizeof(query_name));
    memset(answer_ip, 0, sizeof(answer_ip));
    rcode = validate(&handle, source, pt, ttl, query_name, answer_ip );
    //debug("Validate return :%d\n", rcode);
    if (rcode == VALIDATE_OK) // good reply
    {

        //replace the id in the response with the old_id from the cache
        old_id=htons(pt->old_id);
        memcpy(response_buffer, &old_id,2); // modify the ID with new_id

        //sendto the response_buffer to the client_addr
        char strAddr[64];
        inet_ntop(AF_INET, &pt->client_addr.sin_addr, strAddr,sizeof(strAddr));
        sendto(sock_service, response_buffer , responseLen, 0,
            (struct sockaddr *) &pt->client_addr, sizeof(struct sockaddr_in));
        my_log(LOG_FORWARD, "F: to_client: %s name: %s,  answer_ip: %s, responseLen:%d\n",
               strAddr, name,  answer_ip, responseLen);
        return 0;
    }
    else //rcode >0  // bad reply, the name was  polluted already
    {
        //my_log("INFO: bad or null ip, or TTL mismatch rcode=%d, DROPPED.\n", rcode);
        if( (source == LOCAL_RESOLVER) && (rcode & VALIDATE_BAD_IP)  )
        {
            //send_query_to_open_resolver()
            //Now, construct a query from the response:
            //ns_parserr(&handle, ns_s_qd,0, &rr);
            //name=ns_rr_name(rr);
            //u_int16_t renew_id = get_random(randnum, MAX_QUERY_ID);
            char reversedName[NS_MAXDNAME];
            strReverse(query_name, reversedName);
            blacklist_add(blacklist, reversedName );
            my_log(LOG_INFO, "I: add_new_name_to_blacklist: %s\n", query_name);

            int queryLen = res_mkquery(QUERY, name, ns_rr_class(rr), ns_rr_type(rr),
                            NULL, 0, NULL, query_buffer, sizeof (query_buffer));
            if (queryLen < 0)
            {
                my_log(LOG_ERROR, "E: res_mkquerry error\n");
            }
            u_int16_t txid = htons(new_id);
            memcpy(query_buffer, &txid, 2);
            sendto(sock_open_ns, query_buffer, queryLen, 0,
                (struct sockaddr *) open_ns_addr, sizeof(struct sockaddr_in));
            my_log(LOG_FORWARD, "F: to: %s name: %s\n",servername[OPEN_RESOLVER],name);
        }
        else if ( (source == OPEN_RESOLVER) && (rcode & VALIDATE_BAD_IP) )
        {
            //Add the bad IP to badib list
            my_log(LOG_INFO, "I: got_badip_from_open: Name: %s IP: %s\n", query_name, answer_ip );
	    /*
            if( badip_add(badip, answer_ip)<0)
            {
                my_log(LOG_ERROR, "E: badip_add error while add %s\n",answer_ip);
            }
            badip_dump(badip, config.file_badip);
	    */
        }
    }
    return 0;
}

//int process_tcp_query(int sockfd, char * server_ip, int server_port )
void * tcp_request_handler(void * tcp_arg)
{
    TCP_ARG * arg;
    char server_ip[64];
    int  sockfd, server_port;
    unsigned short length,len_request,len_reply, n;
    int rcode;
    char request_buffer[NS_MAXMSG];
    char reply_buffer[NS_MAXMSG];
    ns_msg handle;
    //NS_MSG request, reply;

    struct sockaddr_in server_addr;
    int socket_query;

    arg =  (TCP_ARG*) tcp_arg;
    sockfd = arg->sockfd;
    strcpy(server_ip, arg->server_ip);
    server_port = arg->server_port;


    free(tcp_arg);
    pthread_detach (pthread_self());

    len_request=0;
    memset(request_buffer,0,sizeof(request_buffer));

    /* DNS query over tcp has 2 bytes before the real DNS Message */
    length=readn(sockfd, request_buffer, 2);
    memcpy(&len_request, request_buffer,2);
    len_request=ntohs(len_request);

    length=readn(sockfd, 2+request_buffer,len_request);

    my_log(LOG_QUERY, "Q: tcp_query, claimed length: %d: real length: %d bytes.\n",len_request, length);

    if(ns_initparse((u_char *)(request_buffer + 2), length, & handle) <0)
    {
        my_log(LOG_ERROR, "E: parse query over tcp.\n");
        close(sockfd);
        return NULL;
    }

    //Now query DNS server via TCP
    socket_query=CreateClientSocket(server_ip, SOCK_STREAM, server_port, &server_addr);
    if(socket_query < 0)
    {
        my_log(LOG_ERROR, "E: CreateClientSocket in %s(%d)\n",__FILE__, __LINE__);
        close(sockfd);
        return NULL;
    }

    rcode=connect(socket_query, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(rcode<0)
    {
        my_log(LOG_ERROR, "E: connect in %s(%d): %s(%d).\n",
                __FILE__, __LINE__, strerror(errno), errno);
        close(sockfd);
        return NULL;
    }
    n=writen(socket_query, request_buffer,len_request+2);
    //debug("write %d bytes\n", n);
    if( n<0)
    {
        my_log(LOG_ERROR, "E: write to DNS server over tcp: %s(%d): %s(%d).\n",
                 __FILE__, __LINE__, strerror(errno), errno);
        close(sockfd);
        return NULL;
    }


    n=readn(socket_query,reply_buffer,2);
    if( n<0)
    {
        my_log(LOG_ERROR, "E: read from DNS server over tcp: %s(%d): %s(%d).\n",
                __FILE__, __LINE__, strerror(errno), errno);
        close(sockfd);
        return NULL;
    }

    memcpy(&len_reply, reply_buffer,2);

    len_reply=ntohs(len_reply);

    length=NS_MAXMSG;
    n=readn(socket_query,2+reply_buffer,len_reply);

    close(socket_query);
    n=writen(sockfd, reply_buffer, len_reply+2);
    my_log(LOG_FORWARD, "F: tcp_client: bytes: %d\n", n);

    close(sockfd);
    return NULL;
}


int forwarder(int service_port, char *open_resolver,int open_resolver_port,
            char *local_resolver, int local_resolver_port, int openflag)
{
    struct sockaddr_in service_addr, tcpservice_addr, local_ns_addr , open_ns_addr;
    fd_set read_fds;
    int cnt_selected, max_fd, rcode;

    //history = qr_new(MAX_QUERY_ID); //65535
    //initialize_random_number(randnum, MAX_QUERY_ID +1 );
    initialize_random_number(randnum, MAX_QUERY_ID  );

    (void) res_init(  );

    servername[OPEN_RESOLVER]=open_resolver;
    servername[LOCAL_RESOLVER]=local_resolver;

    sock_service=CreateServerSocket(SOCK_DGRAM, service_port,&service_addr, openflag);
    if(sock_service < 0)
    {
        my_log(LOG_ERROR, "E: unable to create Server Socket for forwarder service.\n");
        return -1;
    }

    if(config.tcpservice_port >0)
    {
        sock_tcpservice=CreateServerSocket(SOCK_STREAM, config.tcpservice_port, &tcpservice_addr, openflag);
        if(sock_tcpservice < 0)
        {
            my_log(LOG_ERROR, "E: unable to create TCP Socket for forwarder service.\n");
            return -1;
        }
    }

    sock_local_ns=CreateClientSocket(local_resolver, SOCK_DGRAM, local_resolver_port,&local_ns_addr);
    if(sock_local_ns < 0)
    {
        my_log(LOG_ERROR, "E: unable to create Client Socket to  local resovler.\n");
        return -1;
    }

    sock_open_ns=CreateClientSocket(open_resolver, SOCK_DGRAM, open_resolver_port,&open_ns_addr);
    if(sock_open_ns < 0)
    {
        my_log(LOG_ERROR, "E: unable to create Client Socket to  open resovler.\n");
        return -1;
    }

    //Set IP_RECVTTL to access  IP TTL (via recvmsg)
    //I don't know if Microsoft Windows supports this option or not.
    unsigned int set=1;
    rcode = setsockopt(sock_open_ns, IPPROTO_IP, IP_RECVTTL, &set, sizeof(set));
    rcode = setsockopt(sock_local_ns, IPPROTO_IP, IP_RECVTTL, &set, sizeof(set));
    if(rcode<0)
    {
        my_log(LOG_ERROR, "E: setsockopt: IP_RECVTTL is not supported. errno:%d:  %s \n", errno, strerror(errno));
        close(sock_service);
        if(config.tcpservice_port >0) close(sock_tcpservice);
        close(sock_local_ns);
        close(sock_open_ns);
        return -1;
    }

    max_fd= MAX2(sock_service, sock_local_ns);
    max_fd= MAX2(max_fd, sock_open_ns) +1;

    signal(SIGTERM,signal_handler); // catch kill Terminate signal
    signal(SIGINT,signal_handler); // catch kill Interrupt signal
    signal(SIGHUP,signal_handler); // catch kill Interrupt signal

    //Loop
    for (;;)
    {
        FD_ZERO(&read_fds);
        FD_SET(sock_service,&read_fds);
        if(config.tcpservice_port >0 && sock_tcpservice>0)
            FD_SET(sock_tcpservice,&read_fds);
        FD_SET(sock_local_ns,&read_fds);
        FD_SET(sock_open_ns,&read_fds);

        timeout.tv_sec= 60; // seconds
        timeout.tv_usec=0;


        cnt_selected = select(max_fd,&read_fds,NULL,NULL,&timeout); // Wait for a request or a reply
        if(cnt_selected < 0) // some interrupt happened, such as Ctrl-C.
        {
            //debug("Interrupted, break...\n");
            continue;
        }
        if (cnt_selected == 0) // No query or reply arrive in timeout value
        {
            blacklist_dump(blacklist, config.file_blacklist);
            badip_dump(badip, config.file_badip);
            continue;
        }
        if (FD_ISSET(sock_service, &read_fds)) //Got a query from service port
        {
           //debug("Got a request from client.\n");
           //process_query(sock_service, sock_local_ns,&local_ns_addr, sock_open_ns, &open_ns_addr);
           process_query(sock_service, &local_ns_addr, &open_ns_addr);
        }
        if( sock_tcpservice>0 && FD_ISSET(sock_tcpservice, &read_fds)) //Got a query from service port
        {
            unsigned int length;
            TCP_ARG * tcp_arg;
            struct sockaddr_in client_addr_tcp;
            pthread_t tid_tcp_handler;

            length=sizeof(client_addr_tcp);
            tcp_arg = malloc(sizeof(TCP_ARG));
            // tcp_arg will be released/freed in threads tcp_request_handler
            if(tcp_arg == NULL)
            {
                my_log(LOG_ERROR, "E: malloc eror for TCP_ARG \n");
            }
            else
            {
                tcp_arg->sockfd = accept( sock_tcpservice,
                                  (struct sockaddr *) &client_addr_tcp, &length );
                strcpy(tcp_arg->server_ip, config.open_resolver);
                tcp_arg->server_port = config.open_resolver_port;
                if(tcp_arg->sockfd == -1)
                {
                    my_log(LOG_ERROR, "E: accept socket_tcp :%d error. errno=%d at %s(%d).",
                            tcp_arg->sockfd, errno, __FILE__, __LINE__);
                }
                else
                {   //Generate a thread to handle this request
                    rcode=pthread_create(&tid_tcp_handler,NULL, tcp_request_handler, (void*) tcp_arg);
                    if( rcode !=0)
                    {
                        my_log(LOG_ERROR, "E: Cannot create thread for tcp_request_handler:rcode=%d in %s(%d).\n",
                                rcode, __FILE__, __LINE__);
                        close(tcp_arg->sockfd);
                    }
                }
            }
        }// END of if sock_tcpservice>0
        if (FD_ISSET(sock_local_ns, &read_fds)) //Got a reply from local resolver
        {
           //debug("Got a reply from local resolver.\n");
           process_reply(LOCAL_RESOLVER, sock_local_ns, &open_ns_addr);

        }
        if (FD_ISSET(sock_open_ns, &read_fds)) //Got a reply from open resolver
        {
           //debug("Got a reply from open resolver.\n");
           process_reply(OPEN_RESOLVER, sock_open_ns, &open_ns_addr);

        }

    } //end of for loop

    return 0;
}
