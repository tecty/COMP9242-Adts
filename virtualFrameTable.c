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
        size_t frame_id, size_t disk_id, 
        virtual_frame_table_callback_t cb, void * data
    );
    void (*swapInFrame)(
        size_t frame_id, size_t disk_id ,
        virtual_frame_table_callback_t cb, void * data
    );
    void (*delCap)(size_t cap);
    void (*unMapCap)(size_t cap);
} * virtualFrameTable_Interface_t;




typedef  __attribute__((__packed__)) struct
{
    uint32_t frame_ref:19;
    char write:1;
    char read :1;
    
} mapContext_t;

/**
 * Private 
 */
#define DISK_START (1UL << 19)
#define INTERFACE (This.interface)
#define This (VirtualFrameTable_s)

typedef __attribute__((__packed__))  struct 
{
    uint32_t frame_id : 20;
    char copy_on_write: 1;
    char mapped: 1;
    uint32_t cap;
} VirtualPage_t;

typedef __attribute__((__packed__)) struct Frame_s
{
    uint32_t frame_ref  : 19;
    uint32_t disk_id    : 19;
    uint32_t virtual_id : 23;
    char pin:1;
    char dirty:1;
    char considered:1;
} * Frame_t;

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
    size_t lastConsider;
} VirtualFrameTable_s;

void VirtualFrameTable__init(virtualFrameTable_Interface_t inf){
    This.pageTable = DoubleLinkList__init();
    This.frameTable = DynamicArrOne__init(sizeof(uint64_t));
    This.diskTable = Occupy__init();
    This.interface = inf;
    This.lastConsider = 0;
}

void VirtualFrameTable__free(){
    DoubleLinkList__free(This.pageTable);
    DynamicArrOne__free(This.frameTable);
    Occupy__free(This.diskTable);
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
    return DoubleLinkList__add(This.pageTable, vpt);
}

void VirtualFrameTable__delPage(size_t pageId){
    // fetch the candidate root 
    VirtualPage_t vpt= DoubleLinkList__get(
        This.pageTable, pageId
    );
    assert(vpt != 0);
    INTERFACE->delCap(vpt.cap);
    
    size_t candidateRoot = DoubleLinkList__del(
        This.pageTable, pageId
    );

    // there's enough information in the dll 
    if (candidateRoot != 0) return;

    /**
     * ELSE
     * this is the last item in the page list
     */
    // remove the frame table 
    if (This.frame_id >= DISK_START){
        // free from disk 
        Occupy__del(
            This.diskTable, vpt.frame_id - DISK_START
        );
    } else if (vpt.frame_id > 0 ){
        // this is in the frame table
        // tell the frame table, do nothing when you see this page 
        Frame_t framePtr = DynamicArrOne__get(
            This.frameTable, vpt.frame_id
        ); 

        /**
         * By reset it as an empty frame
         */
        framePtr->considered = 0;
        framePtr->dirty = 0;
        framePtr->pin = 0;
        framePtr->virtual_id = 0;
        framePtr->disk_id = 0;
    }
    
}

size_t VirtualFrameTable__dupPage(size_t pageId, size_t copyOnWrite){
    VirtualPage_t source = DoubleLinkList__get(
        This.pageTable, pageId
    );
    
    VirtualPage_t dest ;
    dest.cap = INTERFACE->allocCspace();
    dest.mapped = 0;
    dest.frame_id = source.frame_id;
    if (copyOnWrite > 0)
    {
        dest.copy_on_write = 1;
    } else {
        dest.copy_on_write = 0;
    }
    // add it to page table to manage 
    return DoubleLinkList__add(This.pageTable, dest);
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

uint64_t __VirtualFrameTable__assignToDisk(uint64_t data){
    /**
     * Update the virtual page information
     */
    VirtualPage_t page = data;
    Frame_t frame = DynamicArrOne__get(This.frameTable,page.frame_id); 
    page.frame_id = frame->disk_id + DISK_START;
    /**
     * Unmap those page from client
     */
    INTERFACE->unMapCap(page.cap);
    page.mapped = 0;
    // coherient information
    return page;
}


static inline void VirtualFrameTable__swapOut(
    Frame_t frame, WakeContext_t context
){
    if (frame->disk_id ==0) {
        // alloc a disk frame to it
        frame->disk_id = Occupy__alloc(This.diskTable);
    }
    /**
     * Tell the page table, these page is mapped to disk
     */
    size_t root = DoubleLinkList__getRoot(This.pageTable, frame->virtual_id);
    DoubleLinkList__foreach(
        This.pageTable, root, __VirtualFrameTable__assignToDisk
    );

    // There's not dirty, no need for swap out 
    if (! frame->dirty) return;
    
    INTERFACE->swapOutFrame(
        frame->frame_ref, frame->disk_id,
        __VirtualFrameTable__callback, context
    );
}

/**
 * @pre: client is prepared to yield, if there's not enought frame 
 */
// we will swap out this page, this might be sync or async 
static size_t VirtualFrameTable__requestFrame(WakeContext_t context){
    /**
     * Alloc a frame here 
     */
    size_t frameRef = INTERFACE->allocFrame();
    if (frameRef != 0){
        // successfully alloced a frame 
        struct Frame_s frame; 
        frame.frame_ref = frameRef;
        frame.dirty =0 ;
        frame.considered = 0;
        frame.pin = 0;
        frame.dirty = 0;

        // we don't need to wait a frame in context 
        context->pageCount --;

        return DynamicArrOne__add(This.frameTable, &frame);
    }

    /**
     * No frame is alloced, need to find a frame to alloc 
     */
    size_t alloced = DynamicArrOne__getAlloced(This.frameTable);
    Frame_t frame;
    while (1) {
        This.lastConsider =
            (This.lastConsider + 1) % alloced;
        frame = DynamicArrOne__get(
            This.frameTable, 
            This.lastConsider + 1  
        );

        // pinned frame need to skip
        if (frame->pin) continue;
        // I need to return this frame
        if (frame->considered == 1) break;

        frame->considered = 1;
    }

    VirtualFrameTable__swapOut(frame, context);
    // reset those bits 
    frame->considered = 0;
    frame->dirty      = 0;
    frame->pin        = 1;
    frame->disk_id    = 0;

    /**
     * Frame table id
     */
    return This.lastConsidered + 1 ;
}

static inline void __VirtualFrameTable__unPinFrame(size_t frameId){
    Frame_t frame = DynamicArrOne__get(This.frameTable, frameId);
    frame->pin = 0;
}

static inline void __VirtualFrameTable__PinFrame(size_t frameId){
    Frame_t frame = DynamicArrOne__get(This.frameTable, frameId);
    frame->pin = 1;
}

// this is for private use to assign new frame
static size_t __frameId;
static __VirtualFrameTable__assignToFrame(uint64_t data){
    /**
     * Update the virtual page information
     */
    VirtualPage_t page = data;
    page.frame_id = __frameId;
    // coherient information
    return page;
}

void VirtualFrameTable__pinPage(uint32_t vfref) {
    VirtualPage_t vf = DoubleLinkList__get(This.pageTable, vfref);
    if (vf.frame_id > DISK_START ){
        // this frame is get to the dist 
        struct WakeContext_s context;
        context.pageCount = 1;
        size_t frameId = VirtualFrameTable__requestFrame(&context);

        // tell the page table it has a new frame
        size_t root = DoubleLinkList__getRoot(This.pageTable, vfref); 
        __frameId = frameId;
        DoubleLinkList__foreach(
            This.pageTable, root, __VirtualFrameTable__assignToFrame
        );
        
        if (context.pageCount != 0){
            // yield here 
            ;
        }
        
        context.pageCount = 1;
        // Map in the disk frame
        INTERFACE->swapInFrame(
            frameId, vf.frame_id,__VirtualFrameTable__callback, &context
        );

        if (context.pageCount != 0){
            // yield here again
            ;
        }
    }
}


void VirtualFrameTable__unpinPage(uint32_t vfref) {
    VirtualPage_t vf = DoubleLinkList__get(This.pageTable, vfref);
    // do nothing, the page is alread in the disk 
    if (vf.frame_id > DISK_START) return;
    // unpin the frame 
    Frame_t frame = DynamicArrOne__get(This.frameTable, vf.frame_id);
    frame->pin = 0;
}

void * VirtualFrameTable__getVaddrByVfref(uint32_t vfref) {
    // this will trigger the swap-in;
    VirtualFrameTable__pinPage(vfref);
    VirtualPage_t vf = DoubleLinkList__get(This.pageTable, vfref);
    VirtualFrameTable__unpinPage(vfref);
    // get the addr that mapped in frame 
    return INTERFACE->getFrameVaddr(vf.frame_id);
}

void VirtualFrameTable__pinPageQ(DynamicQ_t  vfrefq) {
    struct WakeContext_s context;
    context.pageCount = DynamicQ__getAlloced(vfrefq);



    context.pageCount = DynamicQ__getAlloced(vfrefq);

}

void VirtualFrameTable__unpinPageQ(DynamicQ_t vfrefq) {

}

mapContext_t VirtualFrameTable__getMapContext(uint32_t faultType) {

}

uint32_t VirtualFrameTable__getPinableCount(
    
){

}