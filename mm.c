
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * Implemented malloc(), free(), and realloc()
 * by using segregated free list among explicit free lists.
 * In the segregated free list, nodes are divided according to 
 * the number of powers of the block size.
 */
#include "mm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* Basic constatns and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amout (bytes) */

/* Segregated list size */
#define LIST_SIZE 20

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(ptr) ((char *)(ptr)-WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr)-WSIZE))
#define PREV_BLKP(ptr) ((char *)(ptr)-GET_SIZE((char *)(ptr)-DSIZE))

/* Forward/Back pointers which stored in free block */
#define GET_PREV(ptr) ((char *)(ptr))
#define GET_NEXT(ptr) ((char *)(ptr) + WSIZE)

/* Adjacent node in segregated list */
#define PREV_NODE(ptr) (*(char **)(ptr))
#define NEXT_NODE(ptr) (*(char **)(GET_NEXT(ptr)))

/* Set pointer */
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* Segregated list */
void *segregated_free_list[LIST_SIZE];

char *heap_start;

static void *extend_heap(size_t size);
static void *coalesce(void *);
static void *place(void *, size_t);
static void pushNode(void *, size_t);
static void popNode(void *);
inline size_t getSize(size_t);
static void *realloc_coalesce(void *, size_t);
static void check_mark_free();
static void check_contiguous_free();
static void check_free_in_list();
static void check_valid_free();
static void check_block_overlap();
static void check_heap_address();
static void mm_check();

/* Get size which append offset */
inline size_t getSize(size_t size) {
    if (size < DSIZE)
        return 2 * DSIZE;
    return ALIGN(size + DSIZE);
}

static void *extend_heap(size_t size) {
    void *ptr;

    /* Allocate an even number of words to maintain alignment */
    size = ALIGN(size);
    if ((ptr = mem_sbrk(size)) == (void *)-1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(ptr), PACK(size, 0));         /* Free block header */
    PUT(FTRP(ptr), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1)); /* New epilogue header */
    pushNode(ptr, size);

    /* Coalesce if the adjacent blocks were free */
    return coalesce(ptr);
}

/* Push pointer in segregated list */
static void pushNode(void *ptr, size_t size) {
    /* Find index of list */
    int i = 0;
    for (; i < LIST_SIZE - 1; i++) {
        size >>= 1;
    }

    /* Find position in set of list */
    void *preNode = segregated_free_list[i];
    void *node = NULL;
    while ((preNode != NULL) && (size > GET_SIZE(HDRP(preNode)))) {
        node = preNode;
        preNode = PREV_NODE(preNode);
    }

    /* Link adjacent nodes */
    SET_PTR(GET_PREV(ptr), preNode);
    SET_PTR(GET_NEXT(ptr), node);
    if (preNode != NULL)
        SET_PTR(GET_NEXT(preNode), ptr);
    if (node != NULL) {
        SET_PTR(GET_NEXT(ptr), node);
        SET_PTR(GET_PREV(node), ptr);
    } else {
        SET_PTR(GET_PREV(ptr), preNode);
        segregated_free_list[i] = ptr;
    }

    return;
}

/* Pop node to allocate */
static void popNode(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));

    if (PREV_NODE(ptr) != NULL)
        SET_PTR(GET_NEXT(PREV_NODE(ptr)), NEXT_NODE(ptr));
    if (NEXT_NODE(ptr) != NULL)
        SET_PTR(GET_PREV(NEXT_NODE(ptr)), PREV_NODE(ptr));
    else {
        int i = 0;
        for (; i < LIST_SIZE - 1; i++) {
            size >>= 1;
        }

        segregated_free_list[i] = PREV_NODE(ptr);
    }

    return;
}

/* Merge adjacent nodes in list */
static void *coalesce(void *ptr) {
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if (prev_alloc && next_alloc)
        return ptr;

    popNode(ptr);
    if (prev_alloc && !next_alloc) {
        popNode(NEXT_BLKP(ptr));

        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        popNode(PREV_BLKP(ptr));

        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
        PUT(HDRP(ptr), PACK(size, 0));
    } else {
        popNode(PREV_BLKP(ptr));
        popNode(NEXT_BLKP(ptr));

        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
        PUT(HDRP(ptr), PACK(size, 0));
    }

    pushNode(ptr, size);

    return ptr;
}

/* Split a node to allocate */
static void *place(void *ptr, size_t size) {
    size_t nodeSize = GET_SIZE(HDRP(ptr));
    size_t newSize = nodeSize - size;

    popNode(ptr);

    if (newSize <= DSIZE * 2) {
        PUT(HDRP(ptr), PACK(nodeSize, 1));
        PUT(FTRP(ptr), PACK(nodeSize, 1));
    } else {
        PUT(HDRP(ptr), PACK(size, 1));
        PUT(FTRP(ptr), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(newSize, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(newSize, 0));
        pushNode(NEXT_BLKP(ptr), newSize);
    }
    return ptr;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    /* Initialize segregated list */
    int i;
    for (i = 0; i < LIST_SIZE; i++) {
        segregated_free_list[i] = NULL;
    }

    /* Create the initial empty heap */
    if ((heap_start = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_start, 0);                            /* Alignment padding */
    PUT(heap_start + WSIZE, PACK(DSIZE, 1));       /* Prologue header */
    PUT(heap_start + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_start + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / DSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    if (size == 0)
        return NULL;

    size = getSize(size);

    void *node = NULL;
    int i = 0;
    size_t temp = size;
    for (; i < LIST_SIZE; i++) {
        if (segregated_free_list[i] != NULL && temp <= 1) { /* Find fit node set in list */
            node = segregated_free_list[i];
            while (node != NULL && size > GET_SIZE(HDRP(node))) { /* If there is no node size to allocate, pass to next set */
                node = PREV_NODE(node);
            }
            if (node != NULL)
                return place(node, size);
        }
        temp >>= 1;
    }

    // if free block is not found, extend the heap
    if ((node = extend_heap(MAX(size, CHUNKSIZE))) == NULL)
        return NULL;

    // mm_check();

    return place(node, size);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    pushNode(ptr, size);

    coalesce(ptr);

    // mm_check();

    return;
}

/* Merge adjacent nodes in list when reallocating */
static void *realloc_coalesce(void *ptr, size_t newSize) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));

    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        if (size >= newSize) {
            popNode(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(size, 1));
            PUT(FTRP(ptr), PACK(size, 1));
            return ptr;
        }
    } else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        if (size >= newSize) {
            popNode(PREV_BLKP(ptr));
            PUT(FTRP(ptr), PACK(size, 1));
            PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 1));
            ptr = PREV_BLKP(ptr);
            return ptr;
        }
    } else if (!prev_alloc && !next_alloc) {
        size += GET_SIZE(FTRP(NEXT_BLKP(ptr))) + GET_SIZE(HDRP(PREV_BLKP(ptr)));
        if (size >= newSize) {
            popNode(PREV_BLKP(ptr));
            popNode(NEXT_BLKP(ptr));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 1));
            PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 1));
            ptr = PREV_BLKP(ptr);
            return ptr;
        }
    }
    return NULL;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t newSize = getSize(size);
    size_t oldsize = GET_SIZE(HDRP(ptr));
    if (oldsize < newSize) {
        char *new = realloc_coalesce(ptr, newSize);
        if (new == NULL) { /* If there doesn't exist adjacent node */
            new = mm_malloc(size);
            memcpy(new, ptr, size);
            mm_free(ptr);
            return new;
        }
        if (new != ptr)
            memcpy(new, ptr, size);
        PUT(HDRP(new), PACK(GET_SIZE(HDRP(new)), 1));
        PUT(FTRP(new), PACK(GET_SIZE(HDRP(new)), 1));
        return new;
    }

    /* If reallocate size is smaller than existing one, use it. */
    PUT(HDRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 1));
    PUT(FTRP(ptr), PACK(GET_SIZE(HDRP(ptr)), 1));

    // mm_check();

    return ptr;
}

/* Is every block in the free list marked as free? */
static void check_mark_free() {
    int i = 0;
    for (; i < LIST_SIZE; i++) {
        void *ptr;
        if ((ptr = segregated_free_list[i]) != NULL)
            while (ptr != NULL) {
                if (GET_ALLOC(HDRP(ptr))) {
                    printf("There is a block which marked as allocated in free list.\n");
                    assert(0);
                }
                ptr = NEXT_NODE(ptr);
            }
    }
}

/* Are there any contiguous free blocks that somehow escaped coalescing? */
static void check_contiguous_free() {
    char *ptr = heap_start;

    while (GET_SIZE(HDRP(ptr)) > 0) {
        if (!GET_ALLOC(HDRP(ptr)) && !GET_ALLOC(HDRP(NEXT_BLKP(ptr)))) {
            size_t size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
            int i = 0;
            for (; i < LIST_SIZE; i++) {
                if (size <= 1)
                    break;
                size >>= 1;
            }

            void *node = segregated_free_list[i];
            while (node != NULL) {
                if (node == NEXT_BLKP(ptr)) {
                    printf("There are contiguous free blocks in free list.\n");
                    assert(0);
                }
                node = NEXT_NODE(node);
            }
        } else if (!GET_ALLOC(HDRP(ptr)) && !GET_ALLOC(HDRP(PREV_BLKP(ptr)))) {
            size_t size = GET_SIZE(HDRP(PREV_BLKP(ptr)));
            int i = 0;
            for (; i < LIST_SIZE; i++) {
                if (size <= 1)
                    break;
                size >>= 1;
            }

            void *node = segregated_free_list[i];
            while (node != NULL) {
                if (node == PREV_BLKP(ptr)) {
                    printf("There are contiguous free blocks in free list.\n");
                    assert(0);
                }
                node = NEXT_NODE(node);
            }
        }
        ptr = NEXT_BLKP(ptr);
    }
}

/* Is every free block actually in the free list? */
static void check_free_in_list() {
    char *ptr = heap_start;

    while (ptr != NULL && GET_SIZE(HDRP(ptr)) > 0) {
        if (!GET_ALLOC(HDRP(ptr))) {
            size_t size = GET_SIZE(HDRP(ptr));
            int i = 0;
            for (; i < LIST_SIZE; i++) {
                if (size <= 1)
                    break;
                size >>= 1;
            }

            void *node = segregated_free_list[i];
            while (node != NULL) {
                if (node == ptr)
                    return;
                node = NEXT_NODE(node);
            }
            printf("There is a free block which is not in free list.\n");
            assert(0);
        }
        ptr = NEXT_BLKP(ptr);
    }
}

/* Do the pointers in the free list point to valid free blocks? */
static void check_valid_free() {
    int i = 0;
    for (; i < LIST_SIZE; i++) {
        void *node = segregated_free_list[i];
        while (node != NULL) {
            if (GET_ALLOC(HDRP(GET_NEXT(node))) || GET_ALLOC(HDRP(GET_PREV(node)))) {
                printf("There is a free block which is not in free list.\n");
                assert(0);
            }
            node = NEXT_NODE(node);
        }
    }
}

/* Do any allocated blocks overlap? */
static void check_block_overlap() {
    void *ptr = heap_start;

    while (GET_SIZE(ptr) > 0) {
        if (GET_SIZE(HDRP(ptr)) != GET_SIZE(FTRP(ptr))) {
            printf("There are overlaped blocks.\n");
            assert(0);
        }
        ptr = NEXT_BLKP(ptr);
    }
}

/* Do the pointers in a heap block point to valid heap addresses? */
static void check_heap_address() {
    void *heap_lo = mem_heap_lo();
    void *heap_hi = mem_heap_hi();
    void *ptr = heap_start;

    while (GET_SIZE(ptr) > 0) {
        if (!(heap_lo <= ptr && ptr <= heap_hi)) {
            printf("There is a block which do not have valid heap address.\n");
            assert(0);
        }
        ptr = NEXT_BLKP(ptr);
    }
}

/* Heap Consistency Checker */
static void mm_check() {
    check_mark_free();
    check_contiguous_free();
    check_free_in_list();
    check_valid_free();
    check_block_overlap();
    check_heap_address();
}
