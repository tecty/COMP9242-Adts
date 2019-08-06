#include "dynamic.h"
// not required 
#include <assert.h>
#include <stdbool.h>

typedef struct DynamicArr_s
{
    void ** item_arr;
    size_t item_size;
    size_t alloced; 
    size_t length; 
    // the last position it's empty
    size_t tail;
}  * DynamicArr_t;

static inline void DynamicArr__incTail(DynamicArr_t da){
    da->tail++;
    da->tail %= da->length;
}

DynamicArr_t DynamicArr__init(size_t item_size){
    DynamicArr_t ret = malloc(sizeof(struct DynamicArr_s));
    // basic setup 
    ret->length    = 4;
    ret->tail      = 0;
    ret->alloced   = 0;
    ret->item_size = item_size;
    // create the dynamic arr 
    ret-> item_arr      = malloc(ret->length * sizeof(void *));
    memset(ret->item_arr, 0, ret->length * sizeof(void *));

    return ret;
}

void * DynamicArr__alloc(DynamicArr_t da, size_t * id){
    if (da->alloced == da-> length - 2){
        // double the size, pre_alloc to improve searching
        da->length   *= 2;
        da->item_arr  = realloc(da->item_arr     , da->length * sizeof(void *));
        memset(&(da->item_arr[da->length/2]), 0, sizeof(void *) * da->length / 2);
    }

    while (da->item_arr[da->tail]) DynamicArr__incTail(da);

    *id = da->tail;
    void * ret = malloc(da->item_size);
    memset(ret, 0, da->item_size);
    da->item_arr[da->tail] = ret;
    da->alloced ++; 

    DynamicArr__incTail(da);
    return ret;
}


/**
 * @ret: the id to the inside managed strcuture 
 */
size_t DynamicArr__add(DynamicArr_t da,void * data){
    size_t id;
    // store the new item here 
    memcpy (DynamicArr__alloc(da, &id), data, da->item_size);
    // some routine to maintain the data intergity
    return id;
}

void * DynamicArr__get(DynamicArr_t da, size_t index){
    // return null to indecate this item is delted 
    assert(index < da->length);
    return da->item_arr[index];
}

void DynamicArr__del(DynamicArr_t da, size_t index){
    if (da->item_arr[index])
    {
        free(da->item_arr[index]);
        da->item_arr[index] = NULL;
        da->alloced --;
    } else {
        printf("==> Deprecated: free a not exist obj %lu\n", index);
    }
}

size_t DynamicArr__getAlloced(DynamicArr_t da){
    return da->alloced;
}

void DynamicArr__foreach(
    DynamicArr_t da, dynamicArr_callback_t cb, void * privateData
){
    for (size_t i = 0; i < da->length; i++) {
        if (da->item_arr[i]) {
            cb(DynamicArr__get(da,i), privateData);
            // printf(
            //     "Slot:%u\tOccupy %u\tNum:%lu\n",
            //     i, da->item_occupied[i],
            //     *(size_t *) DynamicArr__get(da,i)
            // );
        }
    }
}

void __freeCB(void * data, void * unused){
    free(data);
}

void DynamicArr__free(DynamicArr_t da){
    DynamicArr__foreach(da, __freeCB, NULL);
    free(da->item_arr);
    free(da);
}


size_t DynamicArr__getIndexByPtr(DynamicArr_t da, void * ptr){
    assert(ptr != NULL);

    for (size_t i = 0; i < da->length; i++)
    {
        if (da->item_arr[i] == ptr)
        {
            return i;
        }
    }

    printf("==> Error: Unable to find the obj %p\n", ptr);
    assert(0);    
}

// typedef struct
// {
//     size_t end_stamp;
//     size_t callback;
//     void * data; 
//     // if someone want to disable some clock
//     // this is the word 
//     bool enabled;
// } Timer_t;


// int main(int argc, char const *argv[])
// {
//     DynamicArr_t dat = DynamicArr__init(sizeof(Timer_t));
//     Timer_t timer;
//     for (size_t i = 0; i < 10; i++)
//     {
//         timer.end_stamp = i;
//         timer.callback = i*2;
//         DynamicArr__add(dat,&timer);
//     }
    

//     for (size_t i = 0; i < 10; i++)
//     {
//         Timer_t * stored_timer  = DynamicArr__get(dat,i);
//         assert(stored_timer->end_stamp == i);
//         assert(stored_timer->callback == i*2);
//     }
    
//     size_t ids[10];
//     for (size_t i = 0; i < 10; i++)
//     {
//         timer.end_stamp = i;
//         timer.callback = i*2;
//         ids[i] = DynamicArr__add(dat,&timer);
//     }
    
//     DynamicArr__del(dat, ids[2]);
//     DynamicArr__del(dat, ids[5]);
//     DynamicArr__del(dat, ids[7]);

//     bool has_zero = false;


//     size_t skipped= 0;

//     for (size_t i = 0; i < 10; i++)
//     {
//         Timer_t * stored_timer  = DynamicArr__get(dat,ids[i]);
//         if (stored_timer == NULL){
//             skipped ++;
//             continue;
//         }
//         assert(stored_timer->end_stamp == i);
//         assert(stored_timer->callback == i*2);
//     }
    
//     assert(skipped == 3);

//     for (size_t i = 0; i < 100; i++)
//     {
//         timer.end_stamp = i;
//         timer.callback  = i*2;
//         size_t id = DynamicArr__add(dat,&timer);
//         Timer_t * stored_timer =  DynamicArr__get(dat, id);
//         assert(stored_timer->end_stamp == i);
//         assert(stored_timer->callback == i*2);
//     }

//     for (size_t i = 0; i < 10; i++)
//     {
//         assert(
//             DynamicArr__getIndexByPtr(dat, DynamicArr__get(dat, ids[i])) 
//             == ids[i] 
//         );
//     }
    

//     /* code */
//     return 0;
// }
