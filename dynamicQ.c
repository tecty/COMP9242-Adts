#include "dynamicQ.h"

// not required 
#include <assert.h>
#include <stdio.h>
typedef struct DynamicQ_s
{
    void * item_arr;
    uint64_t item_size;
    uint64_t alloced; 
    uint64_t length; 
    // the last position it's empty
    uint64_t head;
    uint64_t tail;
}  * DynamicQ_t;

static inline void DynamicQ__incTail(DynamicQ_t dq){
    dq->tail++; dq->tail %= dq->length;
}

static inline void DynamicQ__incHead(DynamicQ_t dq){
    dq->head++; dq->head %= dq->length;
}

DynamicQ_t DynamicQ__init(uint64_t item_size){
    DynamicQ_t ret = malloc(sizeof(struct DynamicQ_s));
    // basic setup 
    ret->length    = 4;
    ret->head      = 0;
    ret->tail      = 0; //point to an empty slot 
    ret->alloced   = 0;
    ret->item_size = item_size;
    // create the dynamic arr 
    ret-> item_arr      = calloc(ret->length, item_size);

    return ret;
}

void DynamicQ__enQueue(DynamicQ_t dq,void * data){
    if (dq->alloced == dq-> length - 2){
        // printf("DynamicQ is resizing\n");
        // double the size, pre_alloc to improve searching
        dq->length *= 2;
        dq->item_arr      = realloc(dq->item_arr, dq->length* dq->item_size);
        if (dq->tail < dq->head)
        {
            /* Protential bug */
            // copy the first half to the end of second half 
            memcpy(
                dq->item_arr + (dq->length / 2) * dq->item_size,
                dq->item_arr,
                dq->tail  * dq->item_size 
            );
            dq->tail += dq->length /2;
        }
        
    }
    dq->alloced ++;

    // store the new item here 
    memcpy (dq->item_arr + (dq->tail * dq->item_size), data, dq->item_size);
    // some routine to maintain the data intergity
    DynamicQ__incTail(dq);
}

bool DynamicQ__isEmpty(DynamicQ_t dq){
    // if (dq->alloced ==0) {
    //     printf("Head:%lu\tTail:%lu\talloced%lu\tsize:%lu\n",dq->head, dq->tail, dq->alloced, dq->length);
    //     assert(dq->head == dq->tail);
    // }
    return dq->alloced == 0; 
}

void * DynamicQ__first(DynamicQ_t dq){
    // nothing in the queue 
    if (DynamicQ__isEmpty(dq)) return  NULL;
    return dq->item_arr + dq->head * dq->item_size;
}

void DynamicQ__deQueue(DynamicQ_t dq){
    if (DynamicQ__isEmpty(dq)) return;

    // decrement 
    dq->alloced --;
    DynamicQ__incHead(dq);
}

size_t DynamicQ__getAlloced(DynamicQ_t dq){
    return dq->alloced;
}

void DynamicQ__free(DynamicQ_t dq){
    free(dq->item_arr);
    free(dq);
}

void DynamicQ__foreach(DynamicQ_t dq, dynamicQ_callback_t callback){
    void * data;
    while ((data = DynamicQ__first(dq)) && data != NULL){
        callback(data);
        DynamicQ__deQueue(dq) ;
    }
}

DynamicQ_t DynamicQ__filter(
    DynamicQ_t dq, dynamicQ_compare_t cmp, void * private_data
){
    void * data;
    DynamicQ_t new = DynamicQ__init(dq->item_size);
    while ((data = DynamicQ__first(dq)) && data != NULL){
        // enqueue to new
        if (cmp(data, private_data)) DynamicQ__enQueue(new, data);
        DynamicQ__deQueue(dq) ;
    }
    DynamicQ__free(dq);
    return new;    
}

DynamicQ_t DynamicQ__dup(DynamicQ_t dq){
    DynamicQ_t ret = DynamicQ__init(dq->item_size);
    ret->alloced = dq->alloced;
    ret->head = dq->head;
    ret->tail = dq->tail;
    ret->length = dq->length;
    free(ret->item_arr);
    ret->item_arr = malloc(ret->item_size * ret->length);
    memcpy(ret->item_arr, dq->item_arr, ret->item_size * ret->length);
    return ret;
}

typedef struct
{
    uint64_t end_stamp;
    uint64_t callback;
    void * data; 
    // if someone want to disable some clock
    // this is the word 
    bool enabled;
} Timer_t;

uint64_t sum;
void sumUp(void* data){
    sum += *(uint64_t *) data;
}

bool largerCB(void * data, void * pdata){
    return *(uint64_t *) data > (uint64_t) pdata;
}

// int main(int argc, char const *argv[])
// {
//     DynamicQ_t dqt = DynamicQ__init(sizeof(Timer_t));
//     Timer_t timer;

//     for (size_t i = 0; i < 10; i++)
//     {
//         timer.end_stamp = i;
//         timer.callback = 2*i;
//         DynamicQ__enQueue(dqt, &timer);
//     }
//     for (size_t i = 0; i < 10; i++)
//     {
//         Timer_t * first = DynamicQ__first(dqt);
//         assert(first->end_stamp == i);
//         assert(first->callback == 2*i);
//         DynamicQ__deQueue(dqt);
//     }
           
//     assert(DynamicQ__isEmpty(dqt) == true);

//     for (size_t i = 0; i < 4096; i++)
//     {
//         timer.end_stamp = i;
//         timer.callback = 2*i;
//         DynamicQ__enQueue(dqt, &timer);
//         Timer_t * first = DynamicQ__first(dqt);
//         assert(first->end_stamp == i);
//         assert(first->callback == 2*i);
//         DynamicQ__deQueue(dqt);
//     }
           
//     assert(DynamicQ__isEmpty(dqt) == true);


//     for (size_t i = 0; i < 4096; i++)
//     {
//         timer.end_stamp = i;
//         timer.callback = 2*i;
//         DynamicQ__enQueue(dqt, &timer);
//     }
//     for (size_t i = 0; i < 4096; i++)
//     {
//         Timer_t * first = DynamicQ__first(dqt);
//         assert(first->end_stamp == i);
//         assert(first->callback == 2*i);
//         DynamicQ__deQueue(dqt);
//     }

//     DynamicQ__free(dqt);
//     DynamicQ__init(sizeof(uint64_t));
//     for (size_t i = 0; i < 10; i++)
//     {
//         DynamicQ__enQueue(dqt, &i);
//     }
 
//     // sum = 0;
//     // DynamicQ__foreach(dqt, sumUp);
//     // assert(sum == 45);

//     DynamicQ_t filtered = DynamicQ__filter(dqt, largerCB, (void *) 5);
//     sum = 0;
//     DynamicQ__foreach(filtered, sumUp);
//     assert(sum == 30);
           
//     assert(DynamicQ__isEmpty(dqt) == true);
//     return 0;
// }