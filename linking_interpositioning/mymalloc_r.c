#ifdef RUNTIME
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

void *malloc(size_t size)
{
    // printf will call malloc
    // thus we use print_times to avoid infinite iterations
    static int print_times = 0;
    print_times ++;

    void *(*mallocp)(size_t size) = NULL;
    char *error;
    mallocp = dlsym(RTLD_NEXT, "malloc");
    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
        exit(1);
    }
    char *ptr = mallocp(size);

    if (print_times == 1)
        printf("malloc(%ld)=%p\n", size, ptr);
    print_times--;

    return ptr;
}


void free(void *ptr)
{
    void (*freep)(void *) = NULL;
    char *error;

    if (!ptr)
        return;

    freep = dlsym(RTLD_NEXT, "free");
    if ((error = dlerror()) != NULL) {
        fputs(error, stderr);
        //exit(1);
        return;
    }

    freep(ptr);
    printf("free(%p)\n", ptr);
}

#endif
