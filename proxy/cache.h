#ifndef CACHE_H
#define CACHE_H

#include "csapp.h"

typedef struct cnode {
    char *hostname;
    char *port;
    char *uri;
    char *object;
    struct cnode *next;
    struct cnode *prev;
    size_t size;
} cnode_t;
   
typedef struct {
    struct cnode *head;
    struct cnode *tail;
    size_t size;
    size_t max_csize;
    size_t max_osize;
    int readcnt;
    sem_t mutex;
    sem_t w;
} cstat_t;

extern cstat_t cstat;

void cache_init(size_t max_csize, size_t max_osize);
void cache_remove(cnode_t *node);
void cache_insert(cnode_t *node);
cnode_t *find_hit(char *hostname, char *port, char *uri);
cnode_t *cnode_create(char *hostname, char *port, char *uri, char *object, size_t size);
char *cache_reader(char *hostname, char *port, char *uri, size_t *sizep);
void cache_writer(cnode_t *node);
cnode_t *cnode_free(cnode_t *node);
void cache_free(void);

#endif
