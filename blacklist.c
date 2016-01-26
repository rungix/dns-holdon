#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/nameser.h>
#include "common.h"
#include "blacklist.h"


//extern FILE * fd_log;
Trie* trie_new()
{
   Trie *t = (Trie*)malloc(sizeof(Trie));
   if (t == NULL)
        return NULL;

   t->endflag = 0;
   t->key = '\0';
   t->first_child = NULL;
   t->next_sibling = NULL;

   return t;
}

Trie* trie_at_level(Trie *t, char c)
{
   while(t != NULL)
   {
      if(t->key == c)
      {
         return t;
      }
      t = t->next_sibling;
   }
   return NULL;
}

int trie_add(Trie *t, const char *str, int check)
{

   if( str == NULL || t == NULL)
        return -1;

   if( check)
   {
     if (trie_lookup(t, str) == END_FLAG) 
       return -1 ;
   }

   if (str == NULL)
        return -1;
   if (str[0] == '\0')
        return -1;
   if (t == NULL)
        return -1;

   const int n = strlen(str);
   int i;

   for(i=0; i<n; i++)
   //for(i=n-1; i>=0; i--)
   {
      //const char c = str[i];
      const char c = lower(str[i]);
      Trie* parent = t;

      t = t->first_child;
      t = trie_at_level(t,c);
      if(t == NULL)
      {
         t = trie_new();
         t->key = c;
         t->next_sibling = parent->first_child;
         parent->first_child = t;
      }
   }
   t->endflag = END_FLAG;

   return 0; //OK
}

int trie_lookup(Trie *t, const char *str)
{
   const int n = strlen(str);
   int i;
   Trie * parent ;

   for(i=0; i<n; i++)
   //for(i=n-1; i>=0; i--)
   {
      //const char c = str[i];
      const char c = lower(str[i]);
      parent = t;
      t = t->first_child;
      t = trie_at_level(t,c);
      if(t == NULL)
      {
         return parent->endflag; //- 1;
      }
      else
      {
         if (  t->endflag == END_FLAG )
         {
            return t->endflag; 
         } 
      }
   }
   return t->endflag;
}

void trie_travel(FILE * fh, Trie * t, char * str)
{
    //trie* parent = t;
    if ( t == NULL )
        return;

    int len =strlen(str);
/*
    if ( t->endflag == END_FLAG)
    {
        printf("%s%c\n", str,t->key);
    }
*/

    if(t->first_child != NULL ) {
        //strcat(str, t->key);
        str[len]=t->key;
        str[len+1] = '\0';
        trie_travel(fh, t->first_child,str); 
    }
    if ( NULL != t->next_sibling)
    {
        str[len] =0; 
        trie_travel(fh, t->next_sibling, str);
    }
    if ( t->endflag == END_FLAG)
    {
        char rvsStr[NS_MAXDNAME];
        strReverse(str, rvsStr);
        fprintf(fh, "%c%s\n", t->key,rvsStr);
    }
    return; 
}
int trie_free(Trie * t)
{
    //Trie* parent = t;
    if ( t == NULL )
        return 0;

/*
    if ( t->endflag == END_FLAG)
    {
        printf("%s%c\n", str,t->key);
    }
*/

    if(t->first_child != NULL ) {
        trie_free(t->first_child); 
    }
    if ( NULL != t->next_sibling)
    {
        trie_free(t->next_sibling);
    }
    free(t);
    return 0; 
}
int blacklist_dump(Blacklist * blist, char * filename)
{
    FILE *fh;
    char buffer[MAX_LINE];
    if( (blist == NULL) || (filename == NULL) ) 
        return -1;

    if ((fh=fopen(filename, "w")) == NULL) 
    {
        my_log(LOG_ERROR, "E: blacklist_dump is unable to open file:%s to dump blacklist.\n", filename);
        return -1;
    }
    
    memset(buffer, 0, sizeof(buffer));
    trie_travel(fh, blist, buffer); 

    fclose(fh);
    return 0;
}
/*
void trie_compress(Trie *t)
{
   Trie* parent = t;
   t = t->first_child;

   if(t->first_child != NULL)
      trie_compress(t);

   if(t->next_sibling == NULL)
   {
      parent->key = strcat(parent->key,t->key);
      parent->first_child = t->first_child;
      parent->endflag = t->first_child->endflag;
      free(t);

      return;
   }
   else
      trie_compress(t->next_sibling);
}

*/



/*Reverse the srcStr, store the result to dstStr
  You must assure dstStr has enough space 
char * strReverse(char *srcStr, char* dstStr)
{
    int i, length;
    if (srcStr == NULL || dstStr == NULL)
        return NULL;

    length = strlen(srcStr);
    for ( i =0; i < length; i++)
        dstStr[i] = srcStr[length - i - 1 ];
    dstStr[length] = '\0';

    return dstStr; 
}

char * strTrim(char * s){

    int head, tail, length,new_length, i;
    length=strlen(s); 

    for (head=0; head < length; head ++)
        if ( ! ISSPACE(s[head]) )
            break;

    for (tail=length-1; tail >head ; tail --)
        if ( ! ISSPACE(s[tail]) )
            break;

    new_length = tail - head +1;
    if (head != 0)
    {
        for (i=0; i< new_length; i++)
            s[i] = s[head+i];
    }
    s[new_length] = '\0';
    return s;  
     
}

*/
/* 
Load domain names from a file, and load them into the trie 
Return a pointer to the new generated trie on success;
Return NULL on error( malloc fail, all file open fail)
*/
Blacklist * blacklist_load(char * file, int check)
{
    Trie *bl;
    char line[MAX_LINE], r_name[MAX_LINE];
    char *name; 

    bl=trie_new();
    if (bl == NULL)
        return NULL;    
    FILE *fh= fopen(file, "r");
    if (fh ==NULL)
    {
        //trie_free(bl);
        return NULL;
    }
    while( fgets(line, MAX_LINE, fh) != NULL)
    {
        name = strTrim(line);
        if (name[0] == '#' || name[0] =='\0' || name[0] == ';')
            continue;
        strReverse(name, r_name);
        trie_add(bl, r_name, check); 
    }
    fclose(fh);

    return bl;
}
/*
int main( int argc , char * argv[])
{
    Trie  *myt;
    char string[]="abc";
    char buffer[128];
    char *domains[5]={"google.net","witter.com", "www.facebook.com", "pic.twitter.com", "c.twitter.com" };
    char r_name[MAX_LINE];
    int i, check;
    char input_file[MAX_LINE]="test.txt";

    check=0;
    if (argc >= 2)
    {
        if( ! strcmp(argv[1] , "-c")) 
            check = 1;
    } 
    if (argc == 3)
    {
        strcpy(input_file, argv[2]);
    }

    Blacklist *blist=blacklist_load(input_file, check);

    memset(buffer, 0, sizeof(buffer));
    trie_travel(blist,buffer);


    for ( i=0; i<5; i++)
    {
        strReverse(domains[i], r_name);
        printf("%d: Lookup %s:%d\n", i, r_name, trie_lookup(blist, r_name)); 
    }

}
*/


