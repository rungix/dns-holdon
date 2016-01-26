#ifndef __HASHTBL_H_INCLUDE_GUARD__
#define __HASHTBL_H_INCLUDE_GUARD__

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>


typedef size_t hash_size;

struct hashnode_s {
    in_addr_t key;
    void *data;
    struct hashnode_s *next;
};

typedef struct hashtbl {
    hash_size size;
    struct hashnode_s **nodes;
    //hash_size (*hashfunc)(const char *);
    unsigned short (*hashfunc)( in_addr_t );
} HASHTBL;

HASHTBL *hashtbl_create(hash_size size, unsigned short (*hashfunc)(in_addr_t ));
void hashtbl_destroy(HASHTBL *hashtbl);
int hashtbl_insert(HASHTBL *hashtbl, in_addr_t key, void *data);
int hashtbl_remove(HASHTBL *hashtbl, in_addr_t key);
void *hashtbl_get(HASHTBL *hashtbl, in_addr_t  key);
void hashtbl_list( HASHTBL *hashtbl);
//int hashtbl_resize(HASHTBL *hashtbl, hash_size size);

#endif

