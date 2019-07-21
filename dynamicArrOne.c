#include "dynamic.h"
#include "dynamicArrOne.h"

// not required 
// #include <stdbool.h>
// #include <assert.h>

/**
 * This dynamic arr always has one is invalid 
 */

DynamicArrOne_t DynamicArrOne__init(size_t item_size){
    return DynamicArr__init(item_size);
}
void * DynamicArrOne__alloc(DynamicArrOne_t da, size_t * id){
    void * ret = DynamicArr__alloc(da, id);
    *id ++;
    return ret;
}
size_t DynamicArrOne__add(DynamicArrOne_t da,void * data){
    return 1 + DynamicArr__add(da, data);
}
void * DynamicArrOne__get(DynamicArrOne_t da, size_t index){
    if (index == 0 ) return NULL;
    return DynamicArr__get(da, index - 1);
}
void DynamicArrOne__del(DynamicArrOne_t da, size_t index){
    // the data is not exist 
    if (index == 0) return;
    return DynamicArr__del(da, index - 1);
}
void DynamicArrOne__free(DynamicArrOne_t da){
    return DynamicArrOne__free(da);
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
//     DynamicArrOne_t dat = DynamicArrOne__init(sizeof(Timer_t));
//     Timer_t timer;

//     size_t ids[10];
    
//     for (size_t i = 0; i < 10; i++){
//         timer.end_stamp = i;
//         timer.callback = i*2;
//         ids[i]= DynamicArrOne__add(dat,&timer);
//     }
    

//     for (size_t i = 0; i < 10; i++)
//     {
//         Timer_t * stored_timer  = DynamicArrOne__get(dat,ids[i]);
//         assert(stored_timer->end_stamp == i);
//         assert(stored_timer->callback == i*2);
//     }
    
//     for (size_t i = 0; i < 10; i++)
//     {
//         timer.end_stamp = i;
//         timer.callback = i*2;
//         ids[i] = DynamicArrOne__add(dat,&timer);
//     }
    
//     DynamicArrOne__del(dat, ids[2]);
//     DynamicArrOne__del(dat, ids[5]);
//     DynamicArrOne__del(dat, ids[7]);

//     bool has_zero = false;


//     size_t skipped= 0;

//     for (size_t i = 0; i < 10; i++)
//     {
//         Timer_t * stored_timer  = DynamicArrOne__get(dat,ids[i]);
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
//         size_t id = DynamicArrOne__add(dat,&timer);
//         Timer_t * stored_timer =  DynamicArrOne__get(dat, id);
//         assert(stored_timer->end_stamp == i);
//         assert(stored_timer->callback == i*2);
//     }

//     return 0;
// }
