/* 
 * hello.c - Example program to demonstrate different ways to
 *           interpose on the malloc and free functions.
 *
 * Note: be sure to compile unoptimized (-O0) so that gcc won't
 * optimize away the calls to malloc and free.
 */
/* $begin interposemain */
#include <stdio.h>
#include <malloc.h>

int main()
{
    int *p = malloc(32);
    free(p);
    p = malloc(32);
    free(p);
    return(0); 
}
/* $end interposemain */

/*
#include <stdio.h>
#include <malloc.h>

int main()
{
	printf("malloc\n");
	int *p = malloc(2);
	printf("free\n");
	free(p);
	return 0;
}
*/
