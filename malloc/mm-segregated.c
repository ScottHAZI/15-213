/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "HJH",
    /* First member's full name */
    "HJH",
    /* First member's email address */
    "None",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


#define WSIZE 		4
#define DSIZE 		8
#define CHUNKSIZE 	(1<<12)
#define INITCHUNKSIZE (1<<6)
#define NUMLISTS    12

#define MAX(x, y)   ((x) > (y)? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) 		    (*(unsigned int *)(p))
#define PUT(p, val) 	(*(unsigned int *)(p) = (val))

#define GET_SIZE(p)	    (GET(p) & ~0x7)
#define GET_ALLOC(p)	(GET(p) & 0x1)

#define HDRP(bp)	    ((char *)(bp) - WSIZE)
#define FTRP(bp)	    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)	((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

#define PUT_NEXT_FREE(bp, addr) (*(char **)((char *)(bp) + WSIZE) = (char *)(addr)) 
#define PUT_PREV_FREE(bp, addr) (*(char **)(bp) = (char *)(addr))

#define GET_NEXT_FREE(bp) (*(char **)((char *)(bp) + WSIZE))
#define GET_PREV_FREE(bp) (*(char **)(bp))

static char *last_list;
static char *root;
static char *heap_listp;
static void *extend_heap(size_t words);
static char *find_list(size_t size);
static void *find_fit(size_t asize);
static void *place(void *bp, size_t asize);
static void *coalesce(void *bp);
static void insert_free(void *bp);
static void remove_free(void *bp);
static void *realloc_coalesce(void *bp, size_t size, int *success);
static void realloc_place(void *bp, size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    size_t i;
    char *tmp;

	if ((heap_listp = mem_sbrk((4 + 2*NUMLISTS) * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), 0);
    PUT(heap_listp + (2*WSIZE), 0);

    for (i = 0; i < NUMLISTS; i++) {
        tmp = heap_listp + (i*2+1) * WSIZE;
        PUT(tmp, 0);
        PUT(tmp + WSIZE, 0);
        PUT_NEXT_FREE(tmp, NULL);
        PUT_PREV_FREE(tmp, NULL);
    }
    
    last_list = tmp;

    i = NUMLISTS * 2;
    PUT(heap_listp + ((++i)*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + ((++i)*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + ((++i)*WSIZE), PACK(0, 1));

    root = heap_listp + (1*WSIZE);
    heap_listp += (i-1) * WSIZE;

    if (extend_heap(INITCHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0; 
} 

static void *extend_heap(size_t words)
{
    // use dwords (double words) and DSIZE for task 4
    //size_t size = (dwords % 2) ? (dwords+1) * DSIZE : dwords * DSIZE;
    size_t size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    void *bp;

    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static char *find_list(size_t size)
{
    size_t list_offset = 0;

    while (list_offset < (NUMLISTS-1) && (size >>= 1) > DSIZE)
        list_offset++;

    return root + list_offset * DSIZE;
}

static void insert_free(void *bp)
{
    char *next_bp;
    char *prev_bp;
    size_t size = GET_SIZE(HDRP(bp));
    char *list_root = find_list(size);

    prev_bp = list_root;

    for (next_bp = GET_NEXT_FREE(list_root); next_bp != NULL; next_bp = GET_NEXT_FREE(next_bp)) {
        if (GET_SIZE(HDRP(next_bp)) > size) 
            break;
        prev_bp = next_bp;
    }

    if (next_bp != NULL)
        PUT_PREV_FREE(next_bp, bp);
    PUT_NEXT_FREE(bp, next_bp);
    PUT_PREV_FREE(bp, prev_bp);
    PUT_NEXT_FREE(prev_bp, bp);
} 

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    //printf("malloc %d\n", size);

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE-1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL) {
        bp = place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == (void *)-1)
        return NULL;
    bp = place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    //printf("free %p\n", ptr);

    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
}

static void *find_fit(size_t asize)
{
    char *list_root = find_list(asize);
    char *bp;

    while (list_root <= last_list) {
        for (bp = GET_NEXT_FREE(list_root); bp != NULL; bp = GET_NEXT_FREE(bp)) {
            if (GET_SIZE(HDRP(bp)) >= asize) 
                return bp;
        }
        list_root += DSIZE;
    }
    
    return NULL;
}

static void *place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    void *new_bp;

    remove_free(bp);

    if ((csize - asize) < (2*DSIZE)) {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    
    else if (asize >=96) {
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        
        new_bp = NEXT_BLKP(bp);
        PUT(HDRP(new_bp), PACK(asize, 1));
        PUT(FTRP(new_bp), PACK(asize, 1));

        insert_free(bp);
        return new_bp;
    }

    else {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        
        new_bp = NEXT_BLKP(bp);
        PUT(HDRP(new_bp), PACK(csize-asize, 0));
        PUT(FTRP(new_bp), PACK(csize-asize, 0));

        insert_free(new_bp);
    }
    
    return bp;
}

static void remove_free(void *bp)
{
    if (GET_NEXT_FREE(bp) != NULL)
        PUT_PREV_FREE(GET_NEXT_FREE(bp), GET_PREV_FREE(bp));

    PUT_NEXT_FREE(GET_PREV_FREE(bp), GET_NEXT_FREE(bp));
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    void *next_bp = NEXT_BLKP(bp);
    void *prev_bp = PREV_BLKP(bp);

    if (prev_alloc && next_alloc) {
        ;
    }

    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(next_bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        remove_free(next_bp);
    }

    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(FTRP(prev_bp));
        PUT(HDRP(prev_bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
 
        remove_free(prev_bp);

        bp = prev_bp;
    }

    else {
        size += GET_SIZE(FTRP(PREV_BLKP(bp))) + 
            GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(prev_bp), PACK(size, 0));
        PUT(FTRP(next_bp), PACK(size, 0));

        remove_free(next_bp);
        remove_free(prev_bp);

        bp = prev_bp;
    }
    
    insert_free(bp);
    return bp;
}
/*
void *mm_realloc(void *ptr, size_t size)
{
    void *new_block = ptr;
    int remainder;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
    {
        size = 2 * DSIZE;
    }
    else
    {
        size = ALIGN(size + DSIZE);
    }

    if ((remainder = GET_SIZE(HDRP(ptr)) - size) >= 0)
    {
        return ptr;
    }
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr))))
    {
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && GET_SIZE(HDRP(NEXT_BLKP(ptr))))
            printf("hhhh\n");

        //if (!GET_SIZE(HDRP(NEXT_BLKP(ptr))))
        //    printf("end\n");


        if ((remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size) < 0)
        {
            if (extend_heap(MAX(-remainder, CHUNKSIZE)/WSIZE) == NULL)
                return NULL;
            remainder += MAX(-remainder, CHUNKSIZE);
        }
        else 
            printf("gggg\n");

        remove_free(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(size + remainder, 1));
        PUT(FTRP(ptr), PACK(size + remainder, 1));
    }
    else
    {
        new_block = mm_malloc(size);
        memcpy(new_block, ptr, GET_SIZE(HDRP(ptr)));
        mm_free(ptr);
    }

    return new_block;
}
*/

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

void *mm_realloc(void *ptr, size_t size)
{
    void *new_ptr;
    size_t oldsize = GET_SIZE(HDRP(ptr));
    size_t asize;
    size_t tmpsize;
    size_t extendsize;
    int success;
    void *tmp;
    
    if (ptr == NULL)
        return mm_malloc(size);

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + (DSIZE-1)) / DSIZE);

    if (oldsize == asize)
        return ptr;

    if (oldsize > asize) {
        realloc_place(ptr, asize);
        return ptr;
    }
    else { // oldsize < asize
        new_ptr = realloc_coalesce(ptr, asize, &success);
        if (!success) {
            if (GET_SIZE(HDRP(NEXT_BLKP(new_ptr))) == 0) {
                tmpsize = GET_SIZE(HDRP(new_ptr));
                PUT(HDRP(new_ptr), PACK(tmpsize, 1));
                PUT(FTRP(new_ptr), PACK(tmpsize, 1));

                extendsize = MAX(asize - tmpsize, CHUNKSIZE);
                if ((tmp = extend_heap(extendsize/WSIZE)) == NULL)
                    return NULL;
                
                remove_free(tmp);
                PUT(HDRP(new_ptr), PACK(GET_SIZE(HDRP(tmp)) + tmpsize, 1));
                PUT(FTRP(new_ptr), PACK(GET_SIZE(HDRP(tmp)) + tmpsize, 1));

                if (new_ptr != ptr)
                    memmove(new_ptr, ptr, size);
            }
            else {
                tmp = new_ptr;
                new_ptr = mm_malloc(size);
                memmove(new_ptr, ptr, size);
                // memcpy is not suitable here since it has undefined
                // behavior when destination and source overlap
                
                //mm_free(tmp);
                insert_free(tmp);
                // insert_free must be implemented after memmove since 
                // it would modify contents in tmp and tmp+4
                // consider what happens when tmp == ptr
            }
            return new_ptr;
        }

        //success
        if (new_ptr != ptr)
            memmove(new_ptr, ptr, size);
        realloc_place(new_ptr, asize);
        return new_ptr;
    }
}

static void realloc_place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    void *new_bp;

    //if ((csize - asize) >= (2*DSIZE)) {
    if (0) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        
        new_bp = NEXT_BLKP(bp);
        PUT(HDRP(new_bp), PACK(csize-asize, 0));
        PUT(FTRP(new_bp), PACK(csize-asize, 0));

        insert_free(new_bp);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

static void *realloc_coalesce(void *bp, size_t asize, int *success)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    void *next_bp = NEXT_BLKP(bp);
    void *prev_bp = PREV_BLKP(bp);

    // clear the alloc bit
    PUT(HDRP(bp), PACK(size, 0)); 
    PUT(FTRP(bp), PACK(size, 0)); 

    *success = 0;
    //prev_alloc = 1;

    if (prev_alloc && next_alloc) {
        ;
    }

    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(next_bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        remove_free(next_bp);

        if (size >= asize)
            *success = 1;
    }

    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(FTRP(prev_bp));
        PUT(HDRP(prev_bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
 
        remove_free(prev_bp);

        bp = prev_bp;

        if (size >= asize)
            *success = 1;
    }

    else {
        size += GET_SIZE(FTRP(PREV_BLKP(bp))) + 
            GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(prev_bp), PACK(size, 0));
        PUT(FTRP(next_bp), PACK(size, 0));

        remove_free(next_bp);
        remove_free(prev_bp);

        bp = prev_bp;

        if (size >= asize)
            *success = 1;
    }
   
    // insert_free must be implemented after memmove 
    /*
    if (!*success) {
        //insert_free(bp);
    }
    */
    return bp;
}

