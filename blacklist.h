#ifndef __BLACKLIST_H_INCLUDE__
#define __BLACKLIST_H_INCLUDE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define END_FLAG 1
//#define MAX_LINE 1024
//#define ISSPACE(x)  ( (x == ' ' ) || (x =='\n')|| (x == '\r') || (x == '\t'))

#define lower(ch)  ( ch >=65 && ch<=90 )? ch+32: ch

typedef struct trie
{
   unsigned char endflag;
   char key;
   struct trie *first_child;
   struct trie *next_sibling;
}Trie;

#define Blacklist  Trie
#define blacklist_lookup(a, b) trie_lookup(a, b)
#define blacklist_add(list, name) trie_add(list, name, 1)
#define blacklist_release(list) trie_free(list)

//#define blacklist_dump(x) trie_travel(x)

//Trie * blacklist_new();
Blacklist * blacklist_load(char * file, int check);

//int blacklist_lookup(Blacklist * blacklist, char * str);
int blacklist_dump(Blacklist *blacklist, char * filename);

int trie_lookup(Trie *t, const char *str);
int trie_add(Trie *t, const char *str, int check);

Trie* trie_new();
int  trie_free( Blacklist * blist);

#endif

