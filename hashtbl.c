#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtbl.h"


char *mystrdup(const char *s)
{
    char *b;
    if(!(b=malloc(strlen(s)+1))) return NULL;
    strcpy(b, s);
    return b;
}

//static hash_size def_hashfunc(const char *key)
unsigned short def_hashfunc( in_addr_t key)
{
    unsigned short  left, right, hash;
    
    //while(*key) hash+=(unsigned char)*key++;
    left = (key >>16) ^ 0x5555;
    right= (key & 0xFFFF) ^ 0x5555;
    hash = left ^ right;
    return hash;
}

HASHTBL *hashtbl_create(hash_size size, unsigned short (*hashfunc)(in_addr_t))
{
    HASHTBL *hashtbl;

    if(!(hashtbl=malloc(sizeof(HASHTBL)))) return NULL;

    if(!(hashtbl->nodes=calloc(size, sizeof(struct hashnode_s*)))) {
        free(hashtbl);
        return NULL;
    }

    hashtbl->size=size;

    if(hashfunc) hashtbl->hashfunc=hashfunc;
    else hashtbl->hashfunc=def_hashfunc;

    return hashtbl;
}

void hashtbl_destroy(HASHTBL *hashtbl)
{
    hash_size n;
    struct hashnode_s *node, *oldnode;
    
    if(hashtbl == NULL) 
        return;

    for(n=0; n<hashtbl->size; ++n) {
        node=hashtbl->nodes[n];
        while(node) {
            //free(node->key);
            oldnode=node;
            node=node->next;
            free(oldnode);
        }
    }
    free(hashtbl->nodes);
    free(hashtbl);
}

int hashtbl_insert(HASHTBL *hashtbl, in_addr_t key, void *data)
{
    struct hashnode_s *node;
    hash_size hash=hashtbl->hashfunc( key) % hashtbl->size;

    /*  fprintf(stderr, "hashtbl_insert() key=%s, hash=%d, data=%s\n", key, hash, (char*)data);*/

    node=hashtbl->nodes[hash];
    while(node) {
        //if(!strcmp(node->key, key)) {
        if(node->key == key) {
            node->data=data;
            return 0;
        }
        node=node->next;
    }

    if(!(node=malloc(sizeof(struct hashnode_s)))) return -1;
    /*
    if(!(node->key=mystrdup(key))) {
        free(node);
        return -1;
    }
    */
    node->key = key;
    node->data = data;
    node->next = hashtbl->nodes[hash];
    hashtbl->nodes[hash]=node;

    return 0;
}

int hashtbl_remove(HASHTBL *hashtbl, in_addr_t key)
{
    struct hashnode_s *node, *prevnode=NULL;
    hash_size hash=hashtbl->hashfunc(key) % hashtbl->size;

    node=hashtbl->nodes[hash];
    while(node) {
        //if(!strcmp(node->key, key)) {
        if(node->key ==  key) {
            //free(node->key);
            if(prevnode) prevnode->next=node->next;
            else hashtbl->nodes[hash]=node->next;
            free(node);
            return 0;
        }
        prevnode=node;
        node=node->next;
    }

    return -1;
}


void *hashtbl_get(HASHTBL *hashtbl, in_addr_t key)
{
    struct hashnode_s *node;
    hash_size hash=hashtbl->hashfunc(key) % hashtbl->size;

/*  fprintf(stderr, "hashtbl_get() key=%s, hash=%d\n", key, hash);*/

    node=hashtbl->nodes[hash];
    while(node) {
        if(node->key == key) return node->data;
        node=node->next;
    }

    return NULL;
}

void hashtbl_list( HASHTBL *hashtbl)
{
    int i, num=1;
    char strIP[64];
    struct hashnode_s *node;
    if (hashtbl ==NULL) 
        return ;
    for(i=0; i< hashtbl->size; i ++)
    {
        node = hashtbl->nodes[i];
        while(node){ 
            inet_ntop(AF_INET,  (void *)&node->key, strIP, sizeof(strIP));
            printf("%2d : Hash[%d]:{%d:%s}\n",num, i,  node->key, strIP);  
            //printf("%d :{%d:%s}\n",i,  node->key, (char *)node->data);  
            node =node->next;
            num ++;
        } 
    }
    return;
}
/*
int hashtbl_resize(HASHTBL *hashtbl, hash_size size)
{
    HASHTBL newtbl;
    hash_size n;
    struct hashnode_s *node,*next;

    newtbl.size=size;
    newtbl.hashfunc=hashtbl->hashfunc;

    if(!(newtbl.nodes=calloc(size, sizeof(struct hashnode_s*)))) return -1;

    for(n=0; n<hashtbl->size; ++n) {
        for(node=hashtbl->nodes[n]; node; node=next) {
            next = node->next;
            hashtbl_insert(&newtbl, node->key, node->data);
            hashtbl_remove(hashtbl, node->key);
            
        }
    }

    free(hashtbl->nodes);
    hashtbl->size=newtbl.size;
    hashtbl->nodes=newtbl.nodes;

    return 0;
}
*/
