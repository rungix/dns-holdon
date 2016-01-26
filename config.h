#ifndef __CONFIG_H__
#define __CONFIG_H__

#define BOOL unsigned char
#define TRUE  1
#define FALSE 0

#define default_config "/usr/local/dadder/config.txt"
#define default_log "/usr/local/dadder/log.txt"
#define default_pid "/usr/local/dadder/pid.txt"

typedef struct
{
    char * file_config;
    char * file_blacklist;
    char * file_blacklist_dump;
    char * file_badip;
    int    service_port;
    int    tcpservice_port;
    char * local_resolver;
    int    local_resolver_port;
    char * open_resolver;
    int    open_resolver_port;
    int    ttl_threshold;
    BOOL   daemonize ;
    BOOL   fail_forward;
    /* Log */
    char * file_log;
    int    log_level;
    char * file_pid;
    /*Test section*/
    char * testname;
    int    test_interval; // How many seconds to sleep between two tests, default 300

} Configuration;

int load_config( char * file_config, Configuration * config );

#endif
