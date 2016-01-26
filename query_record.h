#ifndef __QUERY_H__
#define __QUERY_H__

#include <netinet/in.h>

#define QR Query_Record
#define MAX_QUERY_ID 65535
#define MAX_NAME_LEN 16 

typedef struct {
    int old_id;
    int new_id;
    int num_answers;
    char query_name[MAX_NAME_LEN];
    struct sockaddr_in client_addr; 
    unsigned long time_query, time_reply;
} Query_Record;

QR *qr_new(int num);

//new_id is the index
QR *qr_add( QR history[], int old_id,int  new_id, char *name, struct sockaddr_in *client_addr);

//new_id is the index
//If found and the name 
QR *qr_lookup(QR  history[], int new_id, char *name, int *found);

//new_id is the index
QR *qr_delete(QR  history[],  int old_id,int  new_id, char *name, struct sockaddr_in *client_addr);

int qr_release(QR * history);


#endif
