#include "priority_q.h"

// We don't need this 
#include <assert.h>
#include <time.h>


typedef struct PriorityQueue_s
{
    void ** queue; 
    size_t len;
    size_t tail;
    compare_fun_t compare;
} * PriorityQueue_t ;

static inline size_t PriorityQueue__parent(size_t curr){
    if (curr == 0)
    {
        return 0;
    }
    return (curr-1) /2;
}

static inline size_t PriorityQueue__child(size_t curr){
    return curr *2 +1;
}

static inline void PriorityQueue__swap(
    PriorityQueue_t pq, size_t index_a, size_t index_b
){
    void * tmp = pq->queue[index_a];
    pq->queue[index_a] = pq->queue[index_b];
    pq->queue[index_b] = tmp;
} 

static inline int PriorityQueue__compare(
    PriorityQueue_t pq, size_t index_a, size_t index_b
){
    return pq->compare(pq->queue[index_a], pq->queue[index_b]);
}


PriorityQueue_t PriorityQueue__init(compare_fun_t compare){
    PriorityQueue_t ret = malloc(sizeof(struct PriorityQueue_s));
    ret->len   = 4;
    ret->tail  = 0;
    ret->compare = compare;
    ret->queue = malloc(ret->len *sizeof(void *) );
    return ret;
}

void PriorityQueue__dump(PriorityQueue_t pq){
    printf("[");
    for(size_t i = 0; i < pq->tail; i++)
    {
        printf("%p, ", pq->queue[i]);
    }
    printf("] \n");    
}

void PriorityQueue__add(PriorityQueue_t pq, void* data){
    /**
     * Append the item
     */
    if(pq->len == pq->tail + 1){
        pq->len  *= 2;
        pq->queue = realloc(pq->queue, pq->len * sizeof(void *));
    }
    pq->queue[pq->tail] = data;
    
    // bubble it up 
    size_t curr   = pq->tail;
    pq->tail ++;
    for (
        size_t parent = PriorityQueue__parent(curr);
        // the conditions 
        // while my parent is not important as me, continue
        curr != 0 && 
        // BUG:
        PriorityQueue__compare(pq, curr, parent) > 0;
        parent = PriorityQueue__parent(parent)
    ){
        PriorityQueue__swap(pq, curr, parent);
        curr = parent;
    }
    // PriorityQueue__dump(pq);
}

/**
 * @return: the first data's pointer
*/
void* PriorityQueue__first(PriorityQueue_t pq){
    if (pq->tail ==0)
    {
        return NULL;
    }
    return pq-> queue[0];
}

void* PriorityQueue__pop(PriorityQueue_t pq){
    void* ret = PriorityQueue__first(pq);
    if (pq-> tail != 0)
    {
        PriorityQueue__swap(pq, 0, -- pq->tail);
        size_t curr = 0;
        for (
            size_t important_suc = PriorityQueue__child(curr);
            important_suc < pq->tail;
            important_suc = PriorityQueue__child(curr)
        ){
            /* try to find the min successor and swap it */
            if(
                important_suc +1 < pq->tail &&
                PriorityQueue__compare(
                    pq, important_suc +1, important_suc
                ) > 0
            ){
                // my peer is more important than me 
                important_suc ++;
            }

            // try to swap with the most important successor
            if( PriorityQueue__compare(pq, important_suc, curr) > 0){
                // the successor is important than me, swap it
                PriorityQueue__swap(pq, curr, important_suc);
                curr = important_suc;
            } else {
                // doesn't need further swapping
                break;
            }
        }
    }
    
    return ret;
}

void PriorityQueue__free(PriorityQueue_t pq){
    free(pq->queue);
    free(pq);
}

int int_bigger(void * index_a, void* index_b){
    __uint64_t a = (__uint64_t) index_a;
    __uint64_t b = (__uint64_t) index_b;
    if (a > b)
    {
        return 1;
    } else if (a< b)
    {
        return -1;
    } else {
        /* code */
        return 0;
    }
}


int main(int argc, char const *argv[])
{
    // srand(time(NULL));
    PriorityQueue_t pq = PriorityQueue__init(int_bigger);
    for (size_t i = 0; i < 10; i++)
    {
        PriorityQueue__add(pq, (void *)(__uint64_t) rand());
    }

    __uint64_t poped  = (__uint64_t) PriorityQueue__pop(pq);
    __uint64_t this;
    while(
        this    = (__uint64_t) PriorityQueue__pop(pq) && 
        this   != (size_t) NULL
    ) {
        assert(this < poped);
    }

    for (size_t i = 0; i < 10; i++)
    {
        PriorityQueue__add(pq, (void *)(__uint64_t) rand());
    }
    for (size_t i = 0; i < 5; i++)
    {
        PriorityQueue__pop(pq);
    }
    for (size_t i = 0; i < 10; i++)
    {
        PriorityQueue__add(pq, (void *)(__uint64_t) rand());
    }
    
    poped  = (__uint64_t) PriorityQueue__pop(pq);
    while(
        this    = (__uint64_t) PriorityQueue__pop(pq) && 
        this   != (size_t) NULL
    ) {
        assert(this < poped);
        assert(this == poped);
    }

    // data intergrtiy 
    pq = PriorityQueue__init(int_bigger);
    for(size_t i = 0; i < 10; i++)
    {
        PriorityQueue__add(pq, (void *)(__uint64_t) i);
    }
    
    for(size_t i = 0; i < 10; i++)
    {
        // so it would be 9 ~ 0 array 
        // PriorityQueue__dump(pq);
        assert ((__uint64_t)PriorityQueue__pop(pq) == (__uint64_t)(10 - i -1) );
    }
    

    return 0;
}
