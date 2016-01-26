#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
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
#include <pthread.h>
#include <arpa/nameser.h>
#include <ctype.h>

#include "common.h"

//extern FILE * fd_log;
ssize_t readn(int fd, void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);      /* return >= 0 */
} ///end readn 


// Write "n" bytes to a descriptor. 
ssize_t  writen(int fd, const void *vptr, size_t n)
{
    size_t      nleft;
    ssize_t     nwritten;
    const char  *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;       // and call write() again 
            else
                return(-1);         // error 
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
} // end writen 

void print_time (FILE * fd) //char * str_result)
{
    if(fd == NULL) 
        fd = stderr;
    time_t ct=time(NULL);
    time_t *tp=&ct;
    struct tm *tmp;
    tmp=localtime(tp);
    fprintf(fd, "%4d-%02d-%02d %02d-%02d-%02d ",
            1900+tmp->tm_year,1+tmp->tm_mon, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
}

char * strReverse(char *srcStr, char* dstStr)
{
    int i, length;
    if (srcStr == NULL || dstStr == NULL)
        return NULL;

    length = strlen(srcStr);
    for ( i =0; i < length; i++)
        dstStr[i] = srcStr[length - i - 1 ];
    dstStr[length] = '\0';

    return dstStr; 
}

char * strTrim(char * s){

    int head, tail, length,new_length, i;
    length=strlen(s); 

    for (head=0; head < length; head ++)
        if ( ! ISSPACE(s[head]) )
            break;

    for (tail=length-1; tail >head ; tail --)
        if ( ! ISSPACE(s[tail]) )
            break;

    new_length = tail - head +1;
    if (head != 0)
    {
        for (i=0; i< new_length; i++)
            s[i] = s[head+i];
    }
    s[new_length] = '\0';
    return s;  
     
}


int CreateClientSocket(char * server_address,int protocol, int server_port, struct sockaddr_in * addr_struct)
{
    int sockfd, rcode;

    sockfd = socket(AF_INET, protocol, 0);
    if(sockfd ==-1)
    {
        fprintf(stderr, "Error: CreateClientSocket: to %s error: %s(%d) in %s(%d).\n" , 
                server_address, strerror(errno), errno, __FILE__, __LINE__);
        return -1;
    }
    bzero(addr_struct, sizeof(struct sockaddr_in) ); //sizeof(addr_struct));
    //bzero(addr_struct, sizeof(*addr_struct));
    addr_struct->sin_family = AF_INET;
    addr_struct->sin_port = htons(server_port);
    rcode=inet_pton(AF_INET, server_address, &(addr_struct->sin_addr));
    if (rcode <=0)
    {
        fprintf(stderr, "Error: CreateClientSocket: inet_pton error:%s\n" , server_address);
        return -1;
    }
    return sockfd;
}

int CreateServerSocket(int protocol, int port, struct sockaddr_in * addr, int openflag)
{
    int socket_fd, rcode;
	socket_fd = socket(AF_INET, protocol, 0);
    if(socket_fd ==-1)
    {
        fprintf(stderr,"Error: CreateServerSocket: error: %s(%d) in %s(%d).\n" , 
                strerror(errno), errno, __FILE__, __LINE__);
        return -1;
    }
    bzero(addr, sizeof(struct sockaddr_in));
    addr->sin_family      = AF_INET;
    if(openflag) 
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    else
        addr->sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    addr->sin_port        = htons(port);

    rcode=bind(socket_fd, (struct sockaddr *) addr, sizeof(struct sockaddr_in));
    if(rcode ==-1)
    {
        fprintf(stderr, "Error: CreateServerSocket: bind on port %d error, errno=%d\n", port, errno);
        close(socket_fd);
        return -1;
    }
    if(protocol ==SOCK_STREAM)
    {
        rcode=listen(socket_fd, MAX_TCP_CLIENTS);
        if(rcode ==-1)
        {
            fprintf(stderr, "Error: listen on socket %d error, errno=%d in %s(%d).\n", 
                  socket_fd, errno, __FILE__, __LINE__);
            close(socket_fd);
            return -1;
        }
    }
    return socket_fd;
}


int recvmsg_with_ttl_m(int sock, char * recv_buffer, int size , int *ttl)
{
    extern FILE * fd_log;
    unsigned char ttl_val=0;
    struct sockaddr_in client_addr;
    int bytes_received;

    struct msghdr msg;
    struct cmsghdr cmsg;
    struct iovec   iov[1];

    struct cmsghdr  *cmptr;
    char buf[CMSG_SPACE(sizeof(ttl_val))]; //See /usr/include/sys/socket.h for the Macro define
    //int cmsg_size = sizeof(struct cmsghdr)+sizeof(ttl_val); 

    memset(&msg, 0, sizeof(msg));
    memset(&cmsg,0,sizeof(cmsg));
    memset(iov, 0, sizeof(iov));
    memset(&client_addr, 0,sizeof(client_addr));
    bytes_received = 0;


    msg.msg_name=&client_addr;
    msg.msg_namelen=sizeof(client_addr);
    iov[0].iov_base=recv_buffer; //response;
    iov[0].iov_len=size;
    msg.msg_iov=iov;
    msg.msg_iovlen=1;

    msg.msg_control = buf; // Assign buffer space for control header + header data/value
    msg.msg_controllen = sizeof(buf); //just initializing it
    msg.msg_flags = 0;

    bytes_received=recvmsg(sock,&msg,0);
    //debug("bytes_received from recvmsg:%d\n", bytes_received)
    if (bytes_received <0) //error strerr, perhaps timeout
    {
        my_log(LOG_ERROR, "E: recvmsg_with_ttl_m, recvmsg error:%d:%s\n", errno, strerror(errno));
        *ttl = -1;
        return  -1;
    }
/*
    if(bytes_received < sizeof(DNS_HEADER))
        continue; // Too short, drop/ignore it 
*/

    // GET TTL
    if (msg.msg_controllen < sizeof(struct cmsghdr) || (msg.msg_flags & MSG_CTRUNC) )
    {
        my_log(LOG_ERROR, "E: recvmsg_with_ttl_m, control information truncated\n");
    }
    ttl_val=0;
    for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL; cmptr = CMSG_NXTHDR(&msg, cmptr))
    {
        #ifdef __BSD__
           if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_RECVTTL && (cmptr->cmsg_len) )
        #else
           if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_TTL && (cmptr->cmsg_len) )
        #endif
        {
            memcpy(&ttl_val, CMSG_DATA(cmptr), sizeof(ttl_val));
            continue;
        }

    }
    /// end of Get TTL
    *ttl = ttl_val;
    return bytes_received;
}
int recvmsg_with_ttl(int sock, char * recv_buffer, int size , int *ttl)
{
    extern FILE * fd_log;
    unsigned char ttl_val=0;
    struct sockaddr_in client_addr;
    int bytes_received;

    struct msghdr msg;
    struct cmsghdr cmsg;
    struct iovec   iov[1];

    struct cmsghdr  *cmptr;
    char buf[CMSG_SPACE(sizeof(ttl_val))]; //See /usr/include/sys/socket.h for the Macro define
    //int cmsg_size = sizeof(struct cmsghdr)+sizeof(ttl_val); 

    memset(&msg, 0, sizeof(msg));
    memset(&cmsg,0,sizeof(cmsg));
    memset(iov, 0, sizeof(iov));
    memset(&client_addr, 0,sizeof(client_addr));
    bytes_received = 0;


    msg.msg_name=&client_addr;
    msg.msg_namelen=sizeof(client_addr);
    iov[0].iov_base=recv_buffer; //response;
    iov[0].iov_len=size;
    msg.msg_iov=iov;
    msg.msg_iovlen=1;

    msg.msg_control = buf; // Assign buffer space for control header + header data/value
    msg.msg_controllen = sizeof(buf); //just initializing it
    msg.msg_flags = 0;

    bytes_received=recvmsg(sock,&msg,0);
    //debug("bytes_received from recvmsg:%d\n", bytes_received)
    if (bytes_received <0) //error strerr, perhaps timeout
    {
        my_log(LOG_ERROR, "E: recvmsg error:%d:%s\n", errno, strerror(errno));
        *ttl = -1;
        return  -1;
    }
    //if () //error strerr
/*
    if(bytes_received < sizeof(DNS_HEADER))
        continue; // Too short, drop/ignore it 
*/

    // GET TTL
    if (msg.msg_controllen < sizeof(struct cmsghdr) || (msg.msg_flags & MSG_CTRUNC) )
    {
        my_log(LOG_ERROR, "E: recvmsg_with_ttl, control information truncated\n");
    }
    ttl_val=0;
    for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL; cmptr = CMSG_NXTHDR(&msg, cmptr))
    {
        #ifdef __BSD__
           if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_RECVTTL && (cmptr->cmsg_len) )
        #else
           if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_TTL && (cmptr->cmsg_len) )
        #endif
        {
            memcpy(&ttl_val, CMSG_DATA(cmptr), sizeof(ttl_val));
            continue;
        }

    }
    /// end of Get TTL
    *ttl = ttl_val;
    return bytes_received;
}

//You must be sure that the size of array is large enough, 
//that is equal or greater than range
int initialize_random_number(int array[], int range)
{
    int i,tmp,r;
    for (i=0; i<range; i++)
        array[i]=i+1;
    srand ( time(NULL) );
    //for(i=range-1; i>0; i--)
    for(i=range-1; i>0; i--)
    {
        r = rand() % i;
        tmp = array[r];
        array[r] = array[i];
        array[i] = tmp;
    }
    return 0;
}
//generate a random and not repeatable number
int get_random(int array[], int range)
{
    static int pointer=0;
    int r1, r2, top, bottom, result;

    result = array[pointer];
//    return array[pointer];

    //swap 2 elements in the other half of array
    if (pointer < range/2) 
    {
        top=range;
        bottom=range/2 ;
    }
    else
    {
        top=range/2 ;
        bottom=0;
    } 
    r1= rand() % (top - bottom) +bottom; 
    r2= rand() % (top - bottom) +bottom; 
    int tmp=array[r1];
    array[r1]=array[r2];
    array[r2]=tmp;
    
    //printf("pointer=%d, r1=%d , r2=%d number=%d\n", pointer, r1, r2, result);

    pointer = (pointer +1 ) % range;
    return result;

}

//Get Time in millisecond
unsigned long getMillisecond()
{
    struct timeval tv;
    if(gettimeofday(&tv,NULL) != 0)
        return 0;
    return (tv.tv_sec*1000) + (tv.tv_usec / 1000);
}

/*
#define MAXID 15

FILE * fd_log;
void main( int argc , char * argv[])
{
    int rand[MAXID+1], num=MAXID, i, j;
    fd_log=stderr;
    initialize_random_number(rand, num+1);
    for(i =0; i< num; i++) 
    {
        printf("Random[%d]: %d\n", i, rand[i]);
    }
    printf("===\n");
    for(i =0; i<3; i++)  
    {
        for ( j=0 ; j<num; j++) 
        {
            printf(" %d\n",  get_random(rand, num));
        }
        printf("\n");
    }
}
*/




