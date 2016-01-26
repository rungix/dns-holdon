#include <stdio.h>
#include <stdlib.h>
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

#include "common.h"
#include "config.h"
#include "blacklist.h"
#include "hashtbl.h"
#include "badip.h"
#include "forwarder.h"

#define DEFAULT_CONFIG "/usr/local/dadder/dadder.config"


Configuration config;
Blacklist * blacklist;
HASHTBL * badip;
struct timeval timeout;
FILE * fd_log; 
FILE * fd_pid; 
int log_level;
char * config_file; 

int goodTTL[2];

void daemonize_init()
{
    int i;
    if(getppid()==1)  // parent pid ==1, the init process 
        return; /* already a daemon */
    i=fork();
    if (i<0)
        exit(1); /* fork error */
    if (i>0)
        exit(0); /* parent exits */
    /* child (daemon) continues */
    setsid(); /* obtain a new process group */

    for (i=getdtablesize();i>=0;--i)
        close(i); /* close all descriptors */
    i=open("/dev/null",O_RDWR);
    dup(i);
    dup(i); /* handle standart I/O */

    umask(027); /* set newly created file permissions */

    //chdir(WORK_DIR); /* change running directory */

    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);

    //the following signals have been captured in forwarder.c
    //signal(SIGHUP,signal_handler); /* catch hangup signal */
    //signal(SIGTERM,signal_handler); /* catch kill signal */
    //signal(SIGINT,signal_handler); /* catch kill signal */
}

void usage(char * self_name)
{
    fprintf(stderr,"%s [options...]\n", self_name);
    fprintf(stderr,"valid options:\n");
    fprintf(stderr," -c <configuration_file>: default /usr/local/dadder/dadder.conf;\n");
    fprintf(stderr," -d: daemonize, or run in background;\n");
    fprintf(stderr," -l <log_file>: file to log message; this value can also be configured in confguration file.\n");
    fprintf(stderr," -p <pid_file>: file to write pid; this value can also be configured in confguration file.\n");
    fprintf(stderr," -h: Help(this message).\n");
    return ;

}

int main(int argc, char* argv[])
{
    int oc;

    /*To parse the options. The options from command line will 
      overwrite those in configuration file(because -c <config_file> 
      is the first option)
    */
//   while( ( oc=getopt(argc, argv, "b:dg:hi:l:L:n:op:s:t:")) != -1 )

    while( ( oc=getopt(argc, argv, "c:dhl:p:")) != -1 )
    {
        switch (oc)
        {
            case 'c': 
                config_file=strdup(optarg);
                config.file_config = config_file;
                break;
            /*
            case 'b':  // blacklist file
                config.file_blacklist=strdup(optarg);   
                break;
            */
            case 'd': //daemonize 
                config.daemonize=TRUE;
                break;
            case 'h':
                usage(argv[0]);
                break; 
            /*
            case 'i':  //IP address file used by censors
                config.file_badip = strdup(optarg);
                break;
            */
            case 'l': //log file
                config.file_log = strdup(optarg);
                break;
            case 'p': //pid file
                config.file_pid = strdup(optarg);
                break;
            default:
                usage(argv[0]);
                break; 
        }// switch(oc)
    }//end of while (parse the options)

    if(config.file_config == NULL)
    {
        config.file_config = DEFAULT_CONFIG;
    }
    if( load_config(config.file_config, &config)<0 ) //The values can be overwrite by the following options
    {
        fprintf(stderr, "ERROR: unable to load configuration file: %s.\n", config.file_config);
        exit(-1);
    }

    if(config.daemonize) 
        daemonize_init();

    /*set default log level to ERROR(2) */
    log_level= (config.log_level == 0? LOG_ERROR: config.log_level);
/*
    if (config.open_resolver == NULL ) config.open_resolver = "8.8.8.8";
    if (config.open_resolver_port == 0 ) config.open_resolver_port = 53;
    if (config.local_resolver == NULL ) config.open_resolver = "127.0.0.1";
    if (config.local_resolver_port == 0 ) config.local_resolver_port = 53;
*/

    if( config.file_log == NULL)
    {
        fd_log = stderr;
    }
    else
    {
        fd_log = fopen(config.file_log, "a");
        if( fd_log ==NULL) 
        {
            fprintf(stderr, "ERROR: unable to open log file: %s to write(append).\n", config.file_log);
            exit(-1);
        }
    }   

    if(config.file_pid != NULL)
    {
        FILE * pid_fd=fopen(config.file_pid,"w"); // open again  
        if (pid_fd == NULL) 
        {
            my_log(LOG_ERROR, "E: Unable to open pid_file:%s to write\n", config.file_pid); 
            exit(-1);
        }
        fprintf(pid_fd, "%d" , getpid());
        fclose(pid_fd);
    }

    blacklist = blacklist_load(config.file_blacklist, 1);
    if( blacklist == NULL) 
    {
        my_log(LOG_ERROR, "E: unable to load Blacklist %s.\n", config.file_blacklist);
        exit(-1); 
    }
    //blacklist_dump(blacklist, config.file_blacklist);
    //printf("OK\n");
    //blacklist_release(blacklist);
    //exit(0);
    

    badip=badip_load(config.file_badip); 
    if(badip== NULL)
    {
        my_log(LOG_ERROR, "E: unable to load bad IP address %s.\n", config.file_badip);
        exit(-1); 
    }

    if( measure_ttl(OPEN_RESOLVER,  config.open_resolver, 
            config.open_resolver_port, "www.mit.edu", &goodTTL[OPEN_RESOLVER]) <0)
    {
        my_log(LOG_ERROR, "E: open_resolver test failed: %s\n", config.open_resolver);
        exit(-1);
    }
    if( measure_ttl(LOCAL_RESOLVER,  config.local_resolver,
            config.local_resolver_port,"www.mit.edu", &goodTTL[LOCAL_RESOLVER]) <0)
    {
        my_log(LOG_ERROR, "E: local_resolver test failed: %s\n", config.local_resolver);
        exit(-1);
    }

    my_log(LOG_MEASURE, "M: M_TTL: %d %d\n", goodTTL[OPEN_RESOLVER], goodTTL[LOCAL_RESOLVER]);

    pthread_t tid_measure_handler;
    int rcode=pthread_create(&tid_measure_handler,NULL, measure_handler, NULL );
    if( rcode !=0)
    {
        my_log(LOG_ERROR, "E: Cannot create thread for tcp_request_handler:rcode=%d in %s(%d).\n",
                rcode, __FILE__, __LINE__);
        exit(-1);
    }

    forwarder(config.service_port, config.open_resolver, config.open_resolver_port,
                    config.local_resolver, config.local_resolver_port, 1);

    //set_resolver_to_me()
    //create_report_service() 
   pthread_join( tid_measure_handler, NULL) ;
    return 0;
}

