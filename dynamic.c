#include "dynamic.h"
// not required 
#include <assert.h>
#include <stdbool.h>

typedef struct DynamicArr_s
{
    void * item_arr;
    bool * item_occupied;
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
    ret-> item_arr      = malloc(ret->length * item_size);
    ret-> item_occupied = malloc(ret->length* sizeof(bool));
    for (size_t i = 0; i < ret->length; i++)
    {
        ret->item_occupied[i] = false;
    }
    

    return ret;
}

void * DynamicArr__alloc(DynamicArr_t da, size_t * id){
    if (da->alloced == da-> length - 2){
        // double the size, pre_alloc to improve searching
        da->length *= 2;
        da->item_occupied = realloc(da->item_occupied, da->length * sizeof(bool));
        da->item_arr      = realloc(da->item_arr     , da->length * da->item_size);
        for (size_t i = da->length/2; i < da->length; i++)
        {
            /* code */
            da->item_occupied[i] = false;
        }
    }


    while (da->item_occupied[da->tail] == true)
    {
        DynamicArr__incTail(da);
    }

    *id = da->tail;
    da->item_occupied[da->tail] = true;
    da->alloced ++; 

    DynamicArr__incTail(da);
    return DynamicArr__get(da, *id);
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
    if (da->item_occupied[index] == false ){
        return NULL;
    }
    return da->item_arr + index * da->item_size;
}

void DynamicArr__del(DynamicArr_t da, size_t index){
    if (da->item_occupied[index] == true){
        da->item_occupied[index] = false;
        da->alloced --;
    } 
}

size_t DynamicArr__getAlloced(DynamicArr_t da){
    return da->alloced;
}

void DynamicArr__free(DynamicArr_t da){
    free(da->item_arr);
    free(da->item_occupied);
    free(da);
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
//         timer.callback = i*2;
//         size_t id = DynamicArr__add(dat,&timer);
//         Timer_t * stored_timer =  DynamicArr__get(dat, id);
//         assert(stored_timer->end_stamp == i);
//         assert(stored_timer->callback == i*2);
//     }



//     /* code */
//     return 0;
// }
