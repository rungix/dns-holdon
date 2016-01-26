#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ini.h"
#include "config.h"
#include "common.h"

extern FILE * fd_log;

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    Configuration* pconfig = (Configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("main", "file_blacklist")) 
    {
        pconfig->file_blacklist = strdup(value); 
    } 
    else if (MATCH("main", "file_blacklist_dump")) 
    {
        pconfig->file_blacklist_dump = strdup(value); 
    } 
    else if (MATCH("main", "file_badip")) 
    {
        pconfig->file_badip = strdup(value);
    } 
    else if (MATCH("main", "file_log")) 
    {
        if(pconfig->file_log == NULL)
           pconfig->file_log = strdup(value);
    } 
    else if (MATCH("main", "file_pid")) 
    {
        if(pconfig->file_pid == NULL)
           pconfig->file_pid = strdup(value);
    } 
    else if (MATCH("main", "service_port")) 
    {
        pconfig->service_port = atoi(value);
    } 
    else if (MATCH("main", "tcpservice_port")) 
    {
        pconfig->tcpservice_port = atoi(value);
    } 
    else if (MATCH("main", "open_resolver")) 
    {
        pconfig->open_resolver = strdup(value);
    } 
    else if (MATCH("main", "local_resolver")) 
    {
        pconfig->local_resolver = strdup(value);
    } 
    else if (MATCH("main", "open_resolver_port")) 
    {
        pconfig->open_resolver_port = atoi(value);
    } 
    else if (MATCH("main", "local_resolver_port")) 
    {
        pconfig->local_resolver_port = atoi(value);
    } 
    else if (MATCH("main", "ttl_threshold")) 
    {
        pconfig->ttl_threshold = atoi(value);
    } 
    else if (MATCH("main", "daemonize")) 
    {
        if (!strcmp(value, "yes") )
            pconfig->daemonize = TRUE;
        else if ( (!strcmp(value , "no")) && (pconfig->daemonize == 0)  )
            pconfig->daemonize = FALSE;
        else 
        {
            fprintf(stderr, "Warning: invalid value:\"%s\" for name:\"%s\" in section:\"%s\", ignored.\n",
                     value, name ,section);
            pconfig->daemonize = 0;
        }
    } 
    else if (MATCH("main", "fail_forward")) 
    {
        if (!strcmp(value, "yes") )
            pconfig->fail_forward = TRUE;
        else if ( (!strcmp(value , "no")) && (pconfig->fail_forward == 0)  )
            pconfig->fail_forward = FALSE;
        else 
        {
            fprintf(stderr, "Warning: invalid value:\"%s\" for name:\"%s\" in section:\"%s\", ignored.\n",
                     value, name ,section);
            pconfig->fail_forward = FALSE;
        }
    } 
    else if (MATCH("main", "log_level")) 
    {
        pconfig->log_level = atoi(value);
    } 
    else if (MATCH("test", "testname")) 
    {
        pconfig->testname = strdup(value); 
    } 
    else if (MATCH("test", "test_interval")) 
    {
        pconfig->test_interval = atoi(value); 
    } 
    else 
    {
        fprintf(stderr, "Warning: unknown section or name:\"%s\":\"%s\", ignored.\n", section, name);
    }
    return 0;
}

void set_default( Configuration * config)
{
    if (config->testname == NULL)
        config->testname = "www.mit.edu";
    if (config->test_interval == 0)
        config->test_interval = 300; //seconds

    if (config->open_resolver == NULL ) config->open_resolver = "8.8.8.8";
    if (config->open_resolver_port == 0 ) config->open_resolver_port = 53;
    if (config->local_resolver == NULL ) config->open_resolver = "127.0.0.1";
    if (config->local_resolver_port == 0 ) config->local_resolver_port = 53;

}
int load_config( char * file_config, Configuration * config )
{
    if (ini_parse(file_config, handler, config) < 0) {
        fprintf(stderr, "Can't load %s\n", file_config);
        return -1;
    }
    set_default( config);
    return 0;
}

/*
int main(int argc, char* argv[])
{
    Configuration config;
    int rcode;

    if( 0 > load_config("configure.ini", &config) )
    {
        printf("Load configuration error:configure.ini\n");    
    }

    printf("Main: Config loaded from : file_blacklist=%s, service_port=%d\n",
        config.file_blacklist, config.service_port);
    return 0;
}

*/
