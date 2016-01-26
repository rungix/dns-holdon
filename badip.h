#ifndef __BADIP_H_INCLUDE_GUARD__
#define __BADIP_H_INCLUDE_GUARD__

#include "hashtbl.h"

//#define MAX_BADIP 64
#define HASH_SIZE  347 

#define badip_lookup(a,b) hashtbl_get(a,b)
#define badip_list(hashtbl) hashtbl_list(hashtbl)

#define badip_release(hashtbl) hashtbl_destroy(hashtbl)

int badip_add(HASHTBL * badip, char * strIP);
int in_badip( HASHTBL * hashtbl, in_addr_t ip); 
int badip_dump(HASHTBL * badip, char * file);

HASHTBL * badip_load(char * file_name);

#endif

