#include "doubleLinkList.h"
#include "dynamicQ.h"
#include "occupy.h"

typedef struct VirtualFrameTable_s * VirtualFrameTable_t;


typedef struct __attribute__((__packed__)) 
{
    uint32_t frame_id : 24;
    char copy_on_write: 1;
    char mapped: 1;
    uint32_t cap;
} virtualFrame_t;

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

struct VirtualFrameTable_s
{
    DoubleLinkList_t virtual_table;
    DynamicArrOne_t frame_table;
    Occupy_t disk_table;
};

VirtualFrameTable_t VirtualFrameTable__init(){
    VirtualFrameTable_t ret = malloc(sizeof(struct VirtualFrameTable_s));
    ret->virtual_table = DoubleLinkList__init();
    ret->frame_table = DynamicArrOne__init(sizeof(uint64_t));
    ret->disk_table = Occupy__init();
    return ret;
}

void VirtualFrameTable__free(VirtualFrameTable_t vft){
    DoubleLinkList__free(vft->virtual_table);
    DynamicArrOne__free(vft->frame_table);
    Occupy__free(vft->disk_table);
}

void * VirtualFrameTable__getVaddrByVfref(
    VirtualFrameTable_t vft, uint32_t vfref
){

}

void VirtualFrameTable__pinFrame(
    VirtualFrameTable_t vft, uint32_t vfref
){

}

void VirtualFrameTable__unpinFrame(
    VirtualFrameTable_t vft, uint32_t vfref
){

}

void VirtualFrameTable__pinFrameQ(
    VirtualFrameTable_t vft, DynamicQ_t  vfrefq
){

}

void VirtualFrameTable__unpinFrameQ(
    VirtualFrameTable_t vft, DynamicQ_t vfrefq
){

}

mapContext_t VirtualFrameTable__getMapContext(
    VirtualFrameTable_t vft, uint32_t faultType
){

}

uint32_t VirtualFrameTable__getPinableCount(
    VirtualFrameTable_t vft
){

}