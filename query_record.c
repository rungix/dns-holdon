#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "query_record.h"

extern FILE * fd_log;
//New Query history, allocate an array with <num> query record
QR *qr_new(int num)
{
    if (num <=0) return NULL;
    QR * pt = malloc(sizeof( QR) * num);
    memset(pt,0, sizeof(QR) * num);
    return pt;
}

//new_id is the index
QR *qr_add( QR  history[], int old_id,int  new_id, char *name, struct sockaddr_in *client_addr)
{
    if(old_id <0 || new_id < 0 || new_id > MAX_QUERY_ID )
    {
        return NULL;
    }
    QR * pt = &history[new_id];
    pt->old_id=old_id;
    pt->new_id=new_id;
    pt->num_answers = 0;
    pt->time_reply = 0;
    strncpy(pt->query_name ,name , MAX_NAME_LEN);
    memcpy(&pt->client_addr, client_addr, sizeof (struct sockaddr_in));
    pt->time_query=getMillisecond();
    return pt;
}

//new_id is the index
QR *qr_lookup(QR  history[], int new_id, char *name, int *found)
{
    if (history == NULL)
        return NULL;
    if( new_id <0 || new_id > MAX_QUERY_ID )
        return NULL;
    
    QR * pt = &history[new_id];
    if ( !strncmp(pt->query_name, name, MAX_NAME_LEN) && (pt->old_id !=0) )
    {
        *found=1;
    } 
    else
    {
        *found=0;
    }
    return pt;
}


//new_id is the index
QR *qr_delete(QR  history[], int old_id,int  new_id, char *name, struct sockaddr_in *client_addr)
{

    return 0;
}

int qr_release(QR * history)
{
    
    return 0;
}

