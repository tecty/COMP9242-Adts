#include "frametable.h"
#include <stdio.h>
#include "dynamicArrOne.h"
#include <stdint.h>
#include <assert.h>

#define FRAME_LIMIT 16

static struct 
{
    size_t cap;
    DynamicArrOne_t frameArr;
    DynamicArrOne_t capArr;
    size_t alloced;
    FILE * swap;
} Frame_s;


void FrameTable__init(){
    Frame_s.cap = 0;
    Frame_s.alloced = 0;
    Frame_s.frameArr = DynamicArrOne__init(sizeof(void *));
    Frame_s.capArr = DynamicArrOne__init(sizeof(size_t));
    remove("swapfile");
    Frame_s.swap = fopen("swapfile","w+b");
}

size_t FrameTable__allocFrame(){
    int empty = 0;
    if (Frame_s.alloced >= FRAME_LIMIT)
    {
        // couldn't give more frame
        return 0;
    }
    Frame_s.alloced ++;

    return DynamicArrOne__add(Frame_s.frameArr, &empty);
}

size_t FrameTable__allocCspace(){
    size_t id;
     DynamicArrOne__alloc(Frame_s.capArr, &id );
    return id;
}
void FrameTable__delCap(size_t cap){
    // do nothing 
    assert(DynamicArrOne__get(Frame_s.capArr,cap) != NULL);
    DynamicArrOne__del(Frame_s.capArr, cap);
}

void FrameTable__copyFrameCap(size_t frameref, size_t dest){
    ;
}

void FrameTable__unMapCap(size_t cap){
    assert(DynamicArrOne__get(Frame_s.capArr,cap) != NULL);
    size_t * data = DynamicArrOne__get(Frame_s.capArr,cap);
    assert(*data == 1);
    *data = 0;
}

void FrameTable__mapCap(size_t cap){
    assert(DynamicArrOne__get(Frame_s.capArr,cap) != NULL);
    size_t * data = DynamicArrOne__get(Frame_s.capArr,cap);
    assert(*data == 0);
    *data = 1;
}

void * FrameTable__getFrameVaddr(size_t frame_id){
    return DynamicArrOne__get(Frame_s.frameArr, frame_id);
}

void FrameTable__swapOutFrame(
    size_t frame_id, size_t disk_id, frame_table_callback_t cb, void * data
){
    uint64_t * dataPtr = DynamicArrOne__get(Frame_s.frameArr, frame_id);
    assert(dataPtr != NULL);
    // printf("Frame:%u \t-> Block:%u\tData:%u\n",  frame_id , disk_id,*dataPtr);

    assert( 
        fseek(Frame_s.swap, 8 * disk_id, SEEK_SET) == 0
    );
    assert(
        fwrite(dataPtr, sizeof(uint64_t),1, Frame_s.swap)== 1
    );
    cb(0, data);
}

void FrameTable__swapInFrame(
    size_t frame_id, size_t disk_id ,frame_table_callback_t cb, void * data
){
    uint64_t * dataPtr = DynamicArrOne__get(Frame_s.frameArr, frame_id);
    assert(dataPtr != NULL);

    assert( 
        fseek(Frame_s.swap, 8 * disk_id, SEEK_SET) == 0
    );
    assert(
        fread(dataPtr, sizeof(uint64_t),1, Frame_s.swap)== 1
    );
    // printf("Block:%u \t-> Frame:%u\tData:%u\n", disk_id, frame_id ,*dataPtr);

    cb(0, data);
}

void FrameTable__dump(){
    printf(
        "I have allocated cap %u\n",DynamicArrOne__getAlloced(Frame_s.capArr)
    );
}

size_t FrameTable__getFrameCap(size_t frameref){
    return 0;
}
// void nullCallback(int64_t err, void * data){
//     assert(err == 0);
//     assert(data == NULL);
// }

// int main(int argc, char const *argv[])
// {
//     FrameTable__init();
//     size_t ids[FRAME_LIMIT];
//     for (size_t i = 0; i < FRAME_LIMIT; i++)
//     {
//         ids[i]= FrameTable__allocFrame();
//         assert(ids[i]!= 0);
//     }

//     for (size_t i = 0; i < FRAME_LIMIT; i++)
//     {
//         // no frame can be allocated 
//         assert(FrameTable__allocFrame() ==0);
//     }
    
//     for (size_t i = 0; i < FRAME_LIMIT; i++)
//     {
//         uint64_t * data = FrameTable__getFrameVaddr(ids[i]);        
//         *data = FRAME_LIMIT - i - 1 ;
//     }

//     for (size_t i = 0; i < FRAME_LIMIT; i++){
//         uint64_t * data = FrameTable__getFrameVaddr(ids[i]);        
//         assert(*data ==  FRAME_LIMIT - i - 1 );
//     }

//     for (size_t i = 0; i < FRAME_LIMIT; i++)
//     {
//         FrameTable__swapOutFrame(ids[i], ids[i], nullCallback, NULL);
//     }
    
//     for (size_t i = 0; i < FRAME_LIMIT; i++)
//     {
//         /* code */
//         FrameTable__swapInFrame(
//             ids[i], ids[FRAME_LIMIT - i - 1], nullCallback, NULL
//         );
//     }

//     for (size_t i = 0; i < FRAME_LIMIT; i++)
//     {
//         uint64_t * data = FrameTable__getFrameVaddr(ids[i]);        
//         assert(*data ==  i);
//     }
//     return 0;
// }
