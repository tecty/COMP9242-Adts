#include <stdlib.h>
#include <stdio.h>
#include "dynamicArrOne.h"

#define FRAME_LIMIT 16

static struct 
{
    size_t cap;
    DynamicArr_t frameArr;
    size_t alloced;
    FILE * swap;
} Frame_s;


void FrameTable__init(){
    Frame_s.cap = 0;
    Frame_s.alloced = 0;
    Frame_s.frameArr = DynamicArrOne__init(sizeof(void *));
    remove("swapfile");
    Frame_s.swap = fopen("swapfile","w+");
}

int FrameTable__allocFrame(){
    int empty = 0;
    Frame_s.alloced ++;
    if (Frame_s.alloced > FRAME_LIMIT)
    {
        // couldn't give more frame
        return 0;
    }
    return DynamicArrOne__add(Frame_s.frameArr, &empty);
}

size_t FrameTable__allocCspace(){
    return Frame_s.cap ++;
}

void FrameTable__swapOutFrame(){

}