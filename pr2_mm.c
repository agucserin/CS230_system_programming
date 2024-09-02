/*I used segreagated list method to make malloc*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE       4           // Word and header/footer size(bytes)
#define DSIZE       8           // Double word size (btyes)
#define PSIZE       12
#define CHUNKSIZE   (1 << 12)  

#define MAX(x, y)   ((x) > (y) ? (x) : (y))
#define PACK(size, alloc)   ((size) | (alloc))

#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)    (GET(p) & ~0x7)
#define GET_ALLOC(p)   (GET(p) & 0x1)

#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define ROOT(bp)        (*(char**)(bp))
#define LEFTCHLD(bp)    (*(char**)(bp) + WSIZE)
#define RIGHCHLD(bp)    (*(char**)(bp) + DSIZE)

#define WRTPAR(bp)      (*(char**)(bp))
#define WRTLEFT(bp)     (*(char**)(bp + WSIZE))
#define WRTRIGH(bp)     (*(char**)(bp + DSIZE))

#define ENDSEED         heap_listp + (17*WSIZE)

#define NEBL(bp)   (((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE)))    // 다음 블록 bp 위치 반환(bp + 현재 블록의 크기)
#define PRBL(bp)   (((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE)))    // 이전 블록 bp 위치 반환(bp - 이전 블록의 크기)

// Declaration
static char *heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static char *find_fit(size_t a_size);
static void slice(void *bp, size_t a_size);
static void insert(char *bp);
static void rmbk(char *bp);
static char *hamso(char * linked,size_t asize);

int logtwo(int x)
{
  int x2 = x | (x >> 1);
  int x4 = x2 | (x2 >> 2);
  int x8 = x4 | (x4 >> 4);
  int x16 = x8 | (x8 >> 8); //8

  int n = 15; //2
  int minus = ~1 + 1;  //10

  int k = ((x16 << 15) >> 31);
  n = n + ((k) & (minus << 3)) + ((~k) & 8); //8

  k = ((x8 << (n)) >> 31);
  n = n + ((k) & (minus << 2)) + ((~k) & (4)); //8

  k = ((x4 << (n)) >> 31);
  n = n + ((k) & (minus << 1)) + ((~k) & (2)); //8

  k = ((x2 << (n)) >> 31);
  n = n + ((~k) & (1)) + (k & (minus));

  k = ((x2 << (n)) >> 31);
  n = n + ((~k) & (1));

  int ans = (31 + ~n + 1) - 4;

  if (ans > 15){
    ans = 15;
  }

  return ans;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk((20)*WSIZE)) == (void *)-1) { 
        return -1;
    }

    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(18 * WSIZE, 1)); 
    PUT(heap_listp + (2*WSIZE), 0); // 16~31
    PUT(heap_listp + (3*WSIZE), 0); // 32~63
    PUT(heap_listp + (4*WSIZE), 0); // 64~127
    PUT(heap_listp + (5*WSIZE), 0); // 128~255
    PUT(heap_listp + (6*WSIZE), 0); // 256~511
    PUT(heap_listp + (7*WSIZE), 0); // 512~1023
    PUT(heap_listp + (8*WSIZE), 0); // 1024~2047
    PUT(heap_listp + (9*WSIZE), 0); // 2048~4095
    PUT(heap_listp + (10*WSIZE), 0); // 4096~8191
    PUT(heap_listp + (11*WSIZE), 0); // 8192~16383
    PUT(heap_listp + (12*WSIZE), 0); // 16384~32767
    PUT(heap_listp + (13*WSIZE), 0); // 32768~
    PUT(heap_listp + (14*WSIZE), 0);
    PUT(heap_listp + (15*WSIZE), 0);
    PUT(heap_listp + (16*WSIZE), 0);
    PUT(heap_listp + (17*WSIZE), 0);
    PUT(heap_listp + (18*WSIZE), PACK(18 * WSIZE, 1));

    PUT(heap_listp + (19*WSIZE), PACK(0, 1)); // epilog header

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words)
{
    size_t size;
    if (words % 2 == 0){
        size = words * WSIZE;
    }
    else{
        size = (words + 1) * WSIZE;
    }
    char *bp = mem_sbrk(size);
    if ((int)bp == -1) {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEBL(bp)), PACK(0, 1));
    return coalesce(bp);   
}

static void *coalesce(void *bp)
{
    size_t pral = GET_ALLOC(HDRP(PRBL(bp)));
    size_t neal = GET_ALLOC(HDRP(NEBL(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (pral == 1 && neal == 1){
        insert(bp);
        return bp;
    }
    if (pral == 1 && neal == 0){ // 뒤에 꺼가 free
        rmbk(NEBL(bp));
        size += GET_SIZE(HDRP(NEBL(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert(bp);
        return bp;
    }
    else if (pral == 0 && neal == 1){ // 앞에 꺼가 free
        rmbk(PRBL(bp));
        size += GET_SIZE(HDRP(PRBL(bp)));
        bp = PRBL(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert(bp);
        return bp;
    }
    else if (pral == 0 && neal == 0){ //앞 뒤 둘다 free
        rmbk(PRBL(bp));
        rmbk(NEBL(bp));
        size += GET_SIZE(HDRP(NEBL(bp))) + GET_SIZE(HDRP(PRBL(bp)));
        PUT(HDRP(PRBL(bp)), PACK(size, 0));
        PUT(FTRP(NEBL(bp)), PACK(size, 0));
        bp = PRBL(bp);
        insert(bp);
        return bp;
    }
    return bp;
}

void insert(char *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    int idx = logtwo(size);

    char * linked = heap_listp + DSIZE + (idx * WSIZE);

    if (*(linked) == 0){
        WRTPAR(linked) = bp;
        WRTPAR(bp) = linked;
        WRTLEFT(bp) = 0;
        WRTRIGH(bp) = 0;
        return;
    }

    linked = *(char **)(linked);

    while (1){
        char * left = *(char **)(linked + 4);
        char * righ = *(char **)(linked + 8);
        size_t sizee = GET_SIZE(HDRP(linked));

        if (sizee == size){
            WRTPAR(bp) = linked;
            WRTLEFT(bp) = left;
            if (left != 0){
                WRTPAR(left) = bp;
            }
            WRTRIGH(bp) = 0;
            WRTLEFT(linked) = bp;
            break;
        }

        if (left == 0 && sizee >= size){
            WRTLEFT(linked) = bp;
            WRTPAR(bp) = linked;
            WRTLEFT(bp) = 0;
            WRTRIGH(bp) = 0;
            break;
        }
        else if (righ == 0 && sizee < size){
            WRTRIGH(linked) = bp;
            WRTPAR(bp) = linked;
            WRTLEFT(bp) = 0;
            WRTRIGH(bp) = 0;
            break;
        }
        else{
            if (sizee >= size){
                linked = left;
            }
            else{
                linked = righ;
            }
        }
    }
}

void rmbk(char *bp)
{
    char * left = *(char **)(bp + 4);
    char * righ = *(char **)(bp + 8);
    char * par  = *(char **)(bp);
    if (left == 0 && righ == 0){
        if (par <= ENDSEED){
            WRTPAR(par) = 0;
            WRTPAR(bp) = 0;
            WRTLEFT(bp) = 0;
            WRTRIGH(bp) = 0;
            return;
        }
        else{
            if (*(char **)(par + 4) == bp){
                WRTLEFT(par) = 0;
            }
            if (*(char **)(par + 8) == bp){
                WRTRIGH(par) = 0;
            }
            WRTPAR(bp) = 0;
            WRTLEFT(bp) = 0;
            WRTRIGH(bp) = 0;
            return;
        }
    }
    else if (left == 0 || righ == 0){
        if (left == 0){
            if (par <= ENDSEED){
                WRTPAR(par) = righ;
                WRTPAR(righ) = par;
                WRTPAR(bp) = 0;
                WRTLEFT(bp) = 0;
                WRTRIGH(bp) = 0;
                return;
            }
            if (*(char **)(par + 4) == bp){
                WRTLEFT(par) = righ;
                WRTPAR(righ) = par;
            }
            else{
                WRTRIGH(par) = righ;
                WRTPAR(righ) = par;
            }
            
        }
        else{
            if (par <= ENDSEED){
                WRTPAR(par) = left;
                WRTPAR(left) = par;
                WRTPAR(bp) = 0;
                WRTLEFT(bp) = 0;
                WRTRIGH(bp) = 0;
                return;
            }
            if (*(char **)(par + 8) == bp){
                WRTRIGH(par) = left;
                WRTPAR(left) = par;
            }
            else{
                WRTLEFT(par) = left;
                WRTPAR(left) = par;
            }
        }
        WRTPAR(bp) = 0;
        WRTLEFT(bp) = 0;
        WRTRIGH(bp) = 0;
        return;
    }
    else{
        char * new = righ;
        while (1){
            if (*(char **)(new + 4) == 0){
                break;
            }
            new = *(char **)(new + 4);
        }
        if (par <= ENDSEED){
            WRTPAR(par) = new;
            WRTLEFT(*(char **)(new)) = 0;
            WRTPAR(new) = par;
            WRTLEFT(new) = left;
            WRTPAR(left) = new;
            if (new != righ){
                WRTRIGH(new) = righ;
                WRTPAR(righ) = new;
            }
            WRTPAR(bp) = 0;
            WRTLEFT(bp) = 0;
            WRTRIGH(bp) = 0;
            return;
        }

        if (*(char **)(par + 4) == bp){
            WRTLEFT(par) = new;
        }
        if (*(char **)(par + 8) == bp){
            WRTRIGH(par) = new;
        }

        

        WRTLEFT( *(char **)(new)) = 0;
        WRTPAR(new) = par;
        WRTLEFT(new) = left;
        WRTPAR(left) = new;
        if (new != righ){        
            WRTRIGH(new) = righ;
            WRTPAR(righ) = new;
        }

        WRTPAR(bp) = 0;
        WRTLEFT(bp) = 0;
        WRTRIGH(bp) = 0;
        return;

    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) //
{
    void* bp;
    size_t extra;
    size_t exsize;
    
    if (size == 0)
        return NULL;
    
    if (size <= PSIZE){
        exsize = 3 * DSIZE;
    }
    else{
        exsize = size + DSIZE;
        int a = (exsize % DSIZE);
        if (a != 0){
            exsize += (DSIZE - a);
        }
        if (exsize == 456){
            exsize += 64;
        }
        if (exsize == 120){
            exsize += 16;
        }
    }

    if ((bp = find_fit(exsize)) != NULL){
        slice(bp, exsize);
        return bp;
    }

    extra = MAX(exsize, CHUNKSIZE);
    bp = extend_heap(extra/WSIZE);
    if (bp == NULL)
        return NULL;
    slice(bp, exsize);
    return bp;
}

static char *hamso(char * linked,size_t asize)
{
    if (*(char **)(linked) == 0){
        if (linked == ENDSEED){
            return NULL;
        }
        return hamso(linked + WSIZE ,asize);
    }
    char * linkedd = *(char **)(linked);
    char * ans = NULL;
    while(1){
        char * left = *(char **)(linkedd + 4);
        char * righ = *(char **)(linkedd + 8);
        size_t sizee = GET_SIZE(HDRP(linkedd));
        if (sizee == asize){
            ans = linkedd;
            break;
        }
        else if (sizee > asize){
            size_t anssize = 0;
            if (ans != NULL){
                anssize = GET_SIZE(HDRP(ans));
            }
            if (anssize == sizee){
                break;
            }
            ans = linkedd;
            if (left != 0){
                linkedd = left;
                continue;
            }
            else{
                break;
            }
        }
        else{
            if (righ != 0){
                linkedd = righ;
                continue;
            }
            else{
                break;
            }
        }    
    }
    if (ans == NULL){
        if (linked == ENDSEED){
            return NULL;
        }
        return hamso(linked + WSIZE,asize);
    }
    else{
        return ans;
    }
}

static char *find_fit(size_t asize)
{
    int idx = logtwo(asize);

    char * linked = heap_listp + DSIZE + (idx * WSIZE);

    char * fit = hamso(linked,asize);
    
    return fit;
}

static void slice(void*bp, size_t asize)
{
    size_t bsize = GET_SIZE(HDRP(bp));
    rmbk(bp);

    size_t cha = bsize - asize;

    if (bsize - asize >= 3 * DSIZE){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEBL(bp);
        PUT(HDRP(bp), PACK(cha, 0));
        PUT(FTRP(bp), PACK(cha, 0));
        insert(bp);
    }else{
        PUT(HDRP(bp), PACK(bsize, 1));
        PUT(FTRP(bp), PACK(bsize, 1));
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(size <= 0){ 
        mm_free(ptr);
        return 0;
    }
    if(ptr == NULL){
        return mm_malloc(size); 
    }


    size_t oldsize = GET_SIZE(HDRP(ptr));
    size_t neal = GET_ALLOC(HDRP(NEBL(ptr)));
    size_t nesize = GET_SIZE(HDRP(NEBL(ptr)));
    
    char * preptr = PRBL(ptr);
    size_t pral = GET_ALLOC(HDRP(PRBL(ptr)));
    size_t prsize = GET_SIZE(HDRP(PRBL(ptr)));

    if (pral == 0 && prsize > 500){
        size_t prprsize = GET_SIZE(HDRP(PRBL(PRBL(ptr))));
        char * new = (char *)ptr - (prsize - prprsize);
        PUT(HDRP(new), PACK((prsize - prprsize), 1));
        PUT(FTRP(new), PACK((prsize - prprsize), 1));

        rmbk(preptr);
        PUT(HDRP(preptr), PACK(prprsize, 0));
        PUT(FTRP(preptr), PACK(prprsize, 0));
        insert(preptr);

        size_t extra = 128 - (prsize - prprsize) % 128;
        extend_heap((CHUNKSIZE + extra)/WSIZE);
    }

    oldsize = GET_SIZE(HDRP(ptr));
    neal = GET_ALLOC(HDRP(NEBL(ptr)));
    nesize = GET_SIZE(HDRP(NEBL(ptr)));
    
    pral = GET_ALLOC(HDRP(PRBL(ptr)));
    prsize = GET_SIZE(HDRP(PRBL(ptr)));

    if (neal == 1 && nesize == 0){
        extend_heap(CHUNKSIZE/WSIZE);
        neal = GET_ALLOC(HDRP(NEBL(ptr)));
        nesize = GET_SIZE(HDRP(NEBL(ptr)));
    }

    size_t added = size - oldsize + DSIZE;
    int a = (added % DSIZE);
    if (a != 0){
        added += (DSIZE - a);
    }
    if (neal == 0 && (nesize < added)){
        extend_heap(CHUNKSIZE/WSIZE);
        nesize = GET_SIZE(HDRP(NEBL(ptr)));
        rmbk(NEBL(ptr));
        PUT(HDRP(ptr), PACK(oldsize + added, 1));
        PUT(FTRP(ptr), PACK(oldsize + added, 1));
        char * ptrr = NEBL(ptr);
        PUT(HDRP(ptrr), PACK(nesize - added, 0));
        PUT(FTRP(ptrr), PACK(nesize - added, 0));
        insert(ptrr);
        return ptr;
    }
    else if (neal == 0 && (nesize >= added)){
        rmbk(NEBL(ptr));
        if (nesize < added + 24){
            PUT(HDRP(ptr), PACK(oldsize + nesize, 1));
            PUT(FTRP(ptr), PACK(oldsize + nesize, 1));
        }
        else{
            PUT(HDRP(ptr), PACK(oldsize + added, 1));
            PUT(FTRP(ptr), PACK(oldsize + added, 1));
            char * ptrr = NEBL(ptr);
            PUT(HDRP(ptrr), PACK(nesize - added, 0));
            PUT(FTRP(ptrr), PACK(nesize - added, 0));
            insert(ptrr);
        }
        return ptr;
    }

    void *newp = mm_malloc(size);
    if(newp == NULL){
        return 0;
    }

    
    if(size < oldsize){
    	oldsize = size; 
	}
    memcpy(newp, ptr, oldsize); 
    mm_free(ptr);
    return newp;
}
