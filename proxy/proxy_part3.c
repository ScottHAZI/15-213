#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void *thread(void *vargp);
void doit(int fd);
int additional_header(char *buf);
void parse_url(char *url, char *hostname, char *port, char *uri);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

cstat_t cstat;

int main(int argc, char **argv)
{
    // port number: 10288 
    int listenfd, *connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    cache_init(MAX_CACHE_SIZE, MAX_OBJECT_SIZE);

    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port); 
        Pthread_create(&tid, NULL, thread, connfd);
    }

    cache_free();
    return 0;
}

void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);
    Close(connfd);
    return NULL;
}

void doit(int fd)
{
    char buf[MAXLINE], method[MAXLINE], url[MAXLINE], version[MAXLINE], uri[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE];
    rio_t rio_browser, rio_server;
    int clientfd;
    int n;
    size_t cnt = 0;
    char *object;
    size_t objectlen = 0;
    cnode_t *node;

    Rio_readinitb(&rio_browser, fd);
    Rio_readlineb(&rio_browser, buf, MAXLINE);
    sscanf(buf, "%s %s %s\n", method, url, version);
    if (strcasecmp(method, "GET")) {
        clienterror(fd, method, "501", "Not implemented", "Proxy does not implement this method");
        return;
    }
    
    parse_url(url, hostname, port, uri);
    printf("hostname: %s\n", hostname);
    printf("port: %s\n", port);

    if ((object = cache_reader(hostname, port, uri, &cnt)) != NULL) {
        printf("Cache hit!\n");
        Rio_writen(fd, object, cnt);
        Free(object);

        /* Read remaining part from browser */
        while (strcmp(buf, "\r\n")) {
            Rio_readlineb(&rio_browser, buf, MAXLINE);
        }
    }
    else {
        clientfd = Open_clientfd(hostname, port);
        Rio_readinitb(&rio_server, clientfd);

        printf("Connect to sever.\n");
        printf("\nRequest headers:\n");
        /* Build request headers */
        sprintf(buf, "GET %s HTTP/1.0\r\n", uri);
        Rio_writen(clientfd, buf, strlen(buf));
        printf("%s", buf);
        sprintf(buf, "Host: %s\r\n", hostname);
        Rio_writen(clientfd, buf, strlen(buf));
        printf("%s", buf);
        sprintf(buf, "%s", user_agent_hdr);
        Rio_writen(clientfd, buf, strlen(buf));
        printf("%s", buf);
        sprintf(buf, "Connection: close\r\n");
        Rio_writen(clientfd, buf, strlen(buf));
        printf("%s", buf);
        sprintf(buf, "Proxy-Connection: close\r\n");
        Rio_writen(clientfd, buf, strlen(buf));
        printf("%s", buf);

        /* Forward remaining request headers from browser */
        while (strcmp(buf, "\r\n")) {
            n = Rio_readlineb(&rio_browser, buf, MAXLINE);
            if (additional_header(buf)) {
                Rio_writen(clientfd, buf, n);
                printf("%s", buf);
            }
            //printf("strcmp: %d\n", strcmp(buf, "\r\n"));
        }
        Rio_writen(clientfd, "\r\n", 2);
        printf("\r\n");
        
        printf("Response...\n");
        object = Malloc(cstat.max_osize);
        /* Receive responses from sever and forward them to browser */
        while ((n = Rio_readlineb(&rio_server, buf, MAXLINE)) > 0) {
            //Rio_writen(fd, buf, strlen(buf)); // This is wrong !!!
            if (objectlen + n <= cstat.max_osize) {
                memcpy(object+objectlen, buf, n);
                objectlen += n;
            }
            Rio_writen(fd, buf, n);
            cnt += n;
            // n may > strlen(buf) since there might be several 
            // strings ('\0' terminated) glued together !!!
            //printf("%d %d\n", strlen(buf), n);

            //printf("%s", buf);
            //printf("%d\n", n);
        }
    }
    
    Close(clientfd);
    printf("%ld bytes in total (including header and body).\n", cnt);
    printf("Response finish. Connection closed.\n\n");

    if (objectlen > 0 && objectlen <= cstat.max_osize) {
        node = cnode_create(hostname, port, uri, object, objectlen);
        cache_writer(node);
        Free(object);
    }
}

int additional_header(char *buf)
{
    if (!strncmp(buf, "\n", 1))
        return 0;
    if (!strncmp(buf, "\r\n", 2))
        return 0;
    if (!strncasecmp(buf, "Host", strlen("Host")))
        return 0;
    if (!strncasecmp(buf, "User-Agent", strlen("User-Agent")))
        return 0;
    if (!strncasecmp(buf, "Connection", strlen("Connection")))
        return 0;
    if (!strncasecmp(buf, "Proxy-Connection", strlen("Proxy-Connection")))
        return 0;

    return 1;
}

void parse_url(char *url, char *hostname, char *port, char *uri)
{
    char *ptr;

    url += strlen("http://");
    strcpy(uri, "/");
    if ((ptr = strchr(url, '/')) != NULL) {
        strcat(uri, ptr+1);
        *ptr = 0;
    }

    if ((ptr = strchr(url, ':')) != NULL) {
        strcpy(port, ptr+1);
        *ptr = 0;
    }
    else {
        strcpy(ptr, "80");
    }

    strcpy(hostname, url);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char* longmsg)
{
    char buf[MAXLINE], body[MAXLINE];

    /* Build HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The proxy sever</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s \r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


