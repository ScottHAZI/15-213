#include "csapp.h"
#include "cache.h"

extern cstat_t cstat;

void cache_init(size_t max_csize, size_t max_osize)
{
    cstat.head = Malloc(sizeof(cnode_t));
    cstat.tail = Malloc(sizeof(cnode_t));
    cstat.head->next = cstat.tail;
    cstat.tail->prev = cstat.head;
    cstat.size = 0;
    cstat.max_csize = max_csize;
    cstat.max_osize = max_osize;
    cstat.readcnt = 0;
    Sem_init(&cstat.mutex, 0, 1);
    Sem_init(&cstat.w, 0, 1);
}

void cache_remove(cnode_t *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    
    cstat.size -= node->size;
}

void cache_insert(cnode_t *node)
{
    node->prev = cstat.head;
    node->next = cstat.head->next;
    node->next->prev = node;
    cstat.head->next = node;

    cstat.size += node->size;
}

cnode_t *find_hit(char *hostname, char *port, char *uri)
{
    cnode_t *node;

    for (node = cstat.head->next; node != cstat.tail; node = node->next) {
        if (strcmp(node->hostname, hostname))
            continue;
        if (strcmp(node->port, port))
            continue;
        if (strcmp(node->uri, uri))
            continue;
        return node;
    }

    return NULL;
}

cnode_t *cnode_create(char *hostname, char *port, char *uri, char *object, size_t size)
{
    cnode_t *node;
   
    node = Malloc(sizeof(cnode_t));

    node->hostname = Malloc(strlen(hostname)+1);
    strcpy(node->hostname, hostname);

    node->port = Malloc(strlen(port)+1);
    strcpy(node->port, port);

    node->uri = Malloc(strlen(uri)+1);
    strcpy(node->uri, uri);

    node->object = Malloc(size);
    // object might contain serveral strings
    memcpy(node->object, object, size);

    //node->size = strlen(object); // this is wrong!
    node->size = size;
    
    return node;
}

char *cache_reader(char *hostname, char *port, char *uri, size_t *sizep)
{
    cnode_t *node;
    char *object = NULL;

    P(&cstat.mutex);
    cstat.readcnt++;
    if (cstat.readcnt == 1)
        P(&cstat.w);
    V(&cstat.mutex);

    if ((node = find_hit(hostname, port, uri)) != NULL) {
        *sizep = node->size;
        object = Malloc(node->size);
        // use memcpy for object
        memcpy(object, node->object, node->size);
        cache_remove(node);
        cache_insert(node);
    }

    P(&cstat.mutex);
    cstat.readcnt--;
    if (cstat.readcnt == 0)
        V(&cstat.w);
    V(&cstat.mutex);

    return object;
}

void cache_writer(cnode_t *node)
{
    size_t available = cstat.max_csize - cstat.size;
    cnode_t *temp;

    P(&cstat.w);

    while (available < node->size) {
        temp = cstat.tail->prev;
        available += temp->size;
        cache_remove(temp);
        cnode_free(temp);
    }
    
    cache_insert(node);

    V(&cstat.w);
}

cnode_t *cnode_free(cnode_t *node)
{
    cnode_t *ret;

    Free(node->hostname);
    Free(node->port);
    Free(node->uri);
    Free(node->object);
    ret = node->next;
    Free(node);

    return ret;
}

void cache_free(void)
{
    cnode_t *node;

    for (node = cstat.head->next; node != cstat.tail; node = cnode_free(node))
       ; 

    Free(cstat.head);
    Free(cstat.tail);
}

