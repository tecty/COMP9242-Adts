#include "doubleLinkList.h"
#include "dynamicQ.h"
#include "occupy.h"

#include <assert.h>

typedef void (* virtual_frame_table_callback_t)(int64_t, void *);

typedef struct virtualFrameTable_Interface_s
{
    /**
     * All the function need to implement by the driver 
     * Then this library is trivial and just calling these functions
     * A simple implemtation can be see in frametable.h (for testing)
     */
    size_t (*allocFrame)();
    size_t (*allocCspace)();
    void * (*getFrameVaddr)(size_t frame_id);
    void (*swapOutFrame)(
        size_t frame_id, size_t disk_id, frame_table_callback_t cb, void * data
    );
    void (*swapInFrame)(
        size_t frame_id, size_t disk_id ,frame_table_callback_t cb, void * data
    );
    void (*delCap)(size_t cap);
} * virtualFrameTable_Interface_t;




typedef struct __attribute__((__packed__)) 
{
    uint32_t frame_ref:19;
    char write:1;
    char read :1;
    
} mapContext_t;

/**
 * Private 
 */
#define DISK_START (1UL << 19)
#define INTERFACE (VirtualFrameTable_s.interface)

typedef struct __attribute__((__packed__)) 
{
    uint32_t frame_id : 24;
    char copy_on_write: 1;
    char mapped: 1;
    uint32_t cap;
} VirtualPage_t;

typedef struct __attribute__((__packed__)) 
{
    uint32_t frame_ref:19;
    char pin:1;
    char dirty:1;
    char considered:1;
    uint32_t virtual_id: 32;
} Frame_t;

typedef struct WakeContext_s {
    uint64_t thread;
    uint64_t pageCount;
} * WakeContext_t;

static struct 
{
    DoubleLinkList_t pageTable;
    DynamicArrOne_t frameTable;
    Occupy_t diskTable;
    virtualFrameTable_Interface_t interface;
} VirtualFrameTable_s;

void VirtualFrameTable__init(virtualFrameTable_Interface_t inf){
    VirtualFrameTable_s.pageTable = DoubleLinkList__init();
    VirtualFrameTable_s.frameTable = DynamicArrOne__init(sizeof(uint64_t));
    VirtualFrameTable_s.diskTable = Occupy__init();
    VirtualFrameTable_s.interface = inf;
}

void VirtualFrameTable__free(){
    DoubleLinkList__free(vft->pageTable);
    DynamicArrOne__free(vft->frameTable);
    Occupy__free(vft->diskTable);
}

size_t VirtualFrameTable__allocPage(){
    VirtualPage_t vpt ;
    vpt.cap = INTERFACE->allocCspace();
    vpt.mapped = 0;
    vpt.copy_on_write = 0;
    /**
     * NULL frame, when some operation show there's an acutal need 
     * for the page, we then try to alloc a actual frame to it
     */
    vpt.frame_id = 0;
    // add it to page table to manage 
    return DoubleLinkList__add(VirtualFrameTable_s.pageTable, vpt);
}

void VirtualFrameTable__delPage(size_t pageId){
    // fetch the candidate root 
    VirtualPage_t vpt= DoubleLinkList__get(
        VirtualFrameTable_s.pageTable, pageId
    );
    assert(vpt != 0);
    INTERFACE->delCap(vpt.cap);
    
    size_t candidateRoot = DoubleLinkList__del(
        VirtualFrameTable_s.pageTable, pageId
    );

    // there's enough information in the dll 
    if (candidateRoot != 0) return;

    /**
     * ELSE
     * this is the last item in the page list
     */
    // remove the frame table 
    if (vpt.frame_id >= DISK_START){
        // free from disk 
        Occupy__del(
            VirtualFrameTable_s.diskTable, vpt.frame_id - DISK_START
        );
    } else if (vpt.frame_id > 0 ){
        // this is in the frame table
        // tell the frame table, do nothing when you see this page 
        Frame_t * framePtr = DynamicArrOne__get(
            VirtualFrameTable_s.frameTable, vpt.frame_id
        ); 

        /**
         * By reset it as an empty frame
         */
        framePtr->considered = 0;
        framePtr->dirty = 0;
        framePtr->pin = 0;
        framePtr->virtual_id = 0;
    }
    
}


static void __VirtualFrameTable__callback(int64_t err, void * data){
    // wake here 
    WakeContext_t context = data;
    context->pageCount --;

    assert(context->pageCount >= 0);
    if (context->pageCount == 0)
    {
        // wake up 
        context->pageCount = context->pageCount;        

    }
    
    return;
}


static inline void VirtualFrameTable__requestPage()

void * VirtualFrameTable__getVaddrByVfref(uint32_t vfref) {
    VirtualPage_t vf = DoubleLinkList__get(vft->pageTable, vfref);
    if (vf.frame_id > DISK_START ){
        // this frame is get to the dist 

    }

    if ()

}

void VirtualFrameTable__pinPage(uint32_t vfref) {

}

void VirtualFrameTable__unpinPage(uint32_t vfref) {

}

void VirtualFrameTable__pinPageQ(DynamicQ_t  vfrefq) {

}

void VirtualFrameTable__unpinPageQ(DynamicQ_t vfrefq) {

}

mapContext_t VirtualFrameTable__getMapContext(uint32_t faultType) {

}

uint32_t VirtualFrameTable__getPinableCount(
    
){

}