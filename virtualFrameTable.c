#include "doubleLinkList.h"
#include "dynamicQ.h"

typedef struct virtualFrameTable_s * virtualFrameTable_t;


typedef struct __attribute__((__packed__)) 
{
    uint32_t frame_id : 24;
    char copy_on_write: 1;
    char mapped: 1;
    uint32_t cap;
} virtualFrame_t

typedef struct __attribute__((__packed__)) 
{
    uint32_t frame_ref:19;
    char pin:1;
    char dirty:1;
    char considered:1;
    uint32_t virtual_id: 32;
} frame_t;

typedef struct __attribute__((__packed__)) 
{
    uint32_t frame_ref:19;
    char write:1;
    char read :1;
    
} mapContext_t;

struct virtualFrameTable_s
{
    DoubleLinkList_t virtual_table;
    DynamicArrOne_t frame_table;
};

virtualFrameTable_t virtualFrameTable__init(){
    virtualFrameTable_t ret = malloc(sizeof(struct virtualFrameTable_s));
    ret->virtual_table = DoubleLinkList__init();
    ret->frame_table = DynamicArrOne__init(sizeof(uint64_t));
    return ret;
}

void virtualFrameTable__free(virtualFrameTable_t vft){
    DoubleLinkList__free(vft->virtual_table);
    DynamicArrOne__free(vft->frame_table);
}

void * virtualFrameTable__getVaddrByVfref(
    virtualFrameTable_t vft, uint32_t vfref
){

}

void virtualFrameTable__pinFrame(
    virtualFrameTable_t vft, uint32_t vfref
){

}

void virtualFrameTable__unpinFrame(
    virtualFrameTable_t vft, uint32_t vfref
){

}

void virtualFrameTable__pinFrameQ(
    virtualFrameTable_t vft, DynamicQ_t  vfrefq
){

}

void virtualFrameTable__unpinFrameQ(
    virtualFrameTable_t vft, DynamicQ_t vfrefq
){

}

mapContext_t virtualFrameTable__getMapContext(
    virtualFrameTable_t vft, uint32_t faultType
){

}

uint32_t virtualFrameTable__getPinableCount(
    virtualFrameTable_t vft
){

}