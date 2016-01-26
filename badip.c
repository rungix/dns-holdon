#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "hashtbl.h"
#include "common.h"
#include "badip.h"


int badip_add(HASHTBL * badip, char * strIP)
{
    in_addr_t ipAddr;
    if (badip == NULL)
        return -1;
    if (strIP == NULL)
        return -1;
    if (inet_pton(AF_INET, strIP, &ipAddr) != 1)
    {
        fprintf(stderr, "EORROR: inet_pton error on %s\n" , strIP); 
        return -1;
    }
 
    hashtbl_insert(badip, ipAddr, strIP);

    return 0;
}
HASHTBL * badip_load(char * file_name)
{
    FILE * fp;
    char line[MAX_LINE];
    char *data;
    in_addr_t ipAddr;
    HASHTBL * hasht;
     

    if( (fp=fopen(file_name, "r"))==NULL)
    {
        return NULL;
    }

    hasht=hashtbl_create(HASH_SIZE,  NULL); 
    if (hasht == NULL )
    {
        fclose(fp);
        return NULL;
    }

    while( fgets(line, MAX_LINE, fp) )
    {
        strTrim(line);        
        if( line[0] ==';' || line[0] == '#')
            continue;
        data = strdup(line);
        if(1 != inet_pton(AF_INET, line, (void *)&ipAddr)) 
        {
            fprintf(stderr, "Invalid IP:%s in file %s\n", line, file_name);
            continue; //error
        }
        hashtbl_insert(hasht, ipAddr, data);
    
    }
    
    fclose(fp);
    return hasht;
}

//int in_badip( HASHTBL * hashtbl, const char * ip)
int in_badip( HASHTBL * hashtbl, in_addr_t ip)
{
    void * pt = hashtbl_get(hashtbl, ip);
    if (pt == NULL) 
        return 0;
    else 
        return 1;
}

int badip_dump( HASHTBL *hashtbl, char *file)
{
    int i;
    char strIP[64];
    struct hashnode_s *node;
    FILE *fp;

    if (hashtbl ==NULL)
        return -1;
    if (file == NULL)
        return -2;
    if ( (fp = fopen(file, "w"))==NULL)
        return -3;

    for(i=0; i< hashtbl->size; i ++)
    {
        node = hashtbl->nodes[i];
        while(node){
            inet_ntop(AF_INET,  (void *)&node->key, strIP, sizeof(strIP));
            fprintf(fp, "%s\n",strIP);
            node =node->next;
        }
    }
    fclose(fp);
    return 0;
}


/*
FILE * fd_log;
int main(int argc , char * argv[])
{
    HASHTBL *badip = badip_load("badip.txt");
    hashtbl_list(badip);

    in_addr_t addrIP1, addrIP2;
    char strIP1[]="1.2.3.4";
    char strIP2[]="59.24.3.173";
    fd_log=stderr;
    
    inet_pton(AF_INET, strIP1, (void *) &addrIP1);
    inet_pton(AF_INET, strIP2, (void *) &addrIP2);
    printf("%s in_badip:%d\n", strIP1, in_badip(badip,addrIP1 )); 
    printf("%s in_badip:%d\n", strIP2, in_badip(badip,addrIP2 )); 
}

*/
