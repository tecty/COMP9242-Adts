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



typedef struct mapContext_s mapContext_t;
struct mapContext_s
{
    uint32_t frame_ref:19;
    char write:1;
    char read :1;
    
};

/**
 * Private 
 */
#define DISK_START (1UL << 19)
#define INTERFACE (This.interface)
#define This (VirtualFrameTable_s)

typedef struct VirtualPage_s VirtualPage_t;
__attribute__((__packed__))  struct VirtualPage_s
{
    uint32_t frame_id : 23;
    char copy_on_write: 1;
    char mapped: 1;
    uint32_t cap :32;
};

typedef  struct Frame_s * Frame_t;
__attribute__((__packed__)) struct Frame_s
{
    size_t virtual_id   : 23 ; 
    size_t frame_ref    : 19 ; 
    size_t disk_id      : 19 ; 
    size_t pin          : 1  ;
    size_t dirty        : 1  ;
    size_t considered   : 1  ;
};


typedef union {
    struct VirtualPage_s page;
    uint64_t number;
} intBridge_t;

uint64_t __PageToInt(struct VirtualPage_s page){
    intBridge_t b; b.page = page; return b.number;
}



struct VirtualPage_s __IntToPage(uint64_t number){
    intBridge_t b; b.number = number; return b.page;
}

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



/**
 * Helper Functions that decouple things
 */


static inline void __yieldByContext(WakeContext_t context){
    if (context->pageCount != 0)
    {
        // yield here 
    }
}

static inline Frame_t __getFrameById(size_t frame_id){
    return DynamicArrOne__get(This.frameTable, frame_id);
}

size_t __newFrame; 
uint64_t  __updatePageToNewFrameCB(uint64_t data){
    VirtualPage_t page = __IntToPage(data);
    page.frame_id = __newFrame;
    return __PageToInt(page);
}

static inline void __pointPageToNewFrame(size_t vfref, size_t frame_id){
    __newFrame = frame_id;
    DoubleLinkList__foreach(
        This.pageTable, 
        DoubleLinkList__getRoot(This.pageTable, vfref),
        __updatePageToNewFrameCB
    );
}
static inline VirtualPage_t __getPageByVfref(size_t vfref){
    return __IntToPage(DoubleLinkList__get(
        This.pageTable, vfref
    ));
}


/**
 * Debug
 */
#include <stdio.h>

void dumpFrame (Frame_t frame){
    printf("Con:%lu\tDir:%lu\tPin:%u\tDiskFrame:%u\tFrameRef:%u\tVirP:%u\n",
        frame->considered,
        frame->dirty,
        frame->pin,
        frame->disk_id,
        frame->frame_ref,
        frame->virtual_id
    );
}
void dumpFrameTable(){
    printf("\n");
    size_t alloced = DynamicArrOne__getAlloced(This.frameTable);
    for (size_t i = 0; i < alloced; i++) {
        Frame_t frame = DynamicArrOne__get(This.frameTable, i + 1);
        dumpFrame(frame);
    }
}
void dumpPage(size_t vfref){
    VirtualPage_t page = __IntToPage(vfref);
    printf("CoW:%u\tMaped:%u\tFtId:%u\tCap;%u\n",
        page.copy_on_write,
        page.mapped,
        page.frame_id,
        page.cap 
    );
    if (page.frame_id < DISK_START)
    {
        dumpFrame(__getFrameById(page.frame_id));
    }
    
}



/**
 * Body:
 */

void VirtualFrameTable__init(virtualFrameTable_Interface_t inf){
    assert( sizeof(struct Frame_s)  == 8);
    assert( sizeof(struct VirtualPage_s)  == 8);
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
    return
    DoubleLinkList__add(This.pageTable,  __PageToInt(vpt));
}

void __swapCallback(int64_t err, void * data){
    WakeContext_t context = data; 
    context->pageCount --;
    if (context->pageCount ==0)
    {
        // wake up 
    }
}

void __incLastConsider(){
    This.lastConsider ++;
    This.lastConsider %= DynamicArrOne__getAlloced(This.frameTable);
}
/**
 * @post: the request frame is alway pined to prevent to be swapout while 
 * a swapping is alreading taking in plave 
 */
size_t VirtualFrameTable__requestFrame(WakeContext_t context){
    size_t allocedRef = INTERFACE->allocFrame();
    if (allocedRef != 0) {
        struct Frame_s frame;
        frame.considered =0;
        frame.dirty = 0;
        frame.pin = 0;
        frame.virtual_id = 0;
        frame.disk_id = 0;
        frame.frame_ref = allocedRef;
        return DynamicArrOne__add(This.frameTable, &frame);
    }
    
}

void VirtualFrameTable__pinPage(size_t vfref){
    struct WakeContext_s context;
    context.thread = 0;
    context.pageCount = 1;
    VirtualFrameTable__requestFrame(&context);
    
}

void VirtualFrameTable__unpinPage(size_t vfref){
    VirtualPage_t page = __getPageByVfref(vfref);
    Frame_t frame = __getFrameById(page.frame_id);
    frame->pin = 0;
}

void * VirtualFrameTable__getVaddrByPageRef(size_t vfref){
    VirtualFrameTable__pinPage(vfref);
    Frame_t frame = __getFrameById(__getPageByVfref(vfref).frame_id);
    // mark as dirty, because when you point to this addr in kernel,
    //  theres' no clue about whether I will change it 
    frame->dirty = 1;
    return INTERFACE->getFrameVaddr(frame->frame_ref);
}

/**
 * Main
 */

#include "frametable.h"
static struct virtualFrameTable_Interface_s simpleInterface = {
    .allocFrame    = FrameTable__allocFrame,
    .allocCspace   = FrameTable__allocCspace,
    .getFrameVaddr = FrameTable__getFrameVaddr,
    .swapOutFrame  = FrameTable__swapOutFrame,
    .swapInFrame   = FrameTable__swapInFrame,
    .delCap        = FrameTable__delCap,
    .unMapCap      = FrameTable__unMapCap,
};

int main(int argc, char const *argv[])
{
    FrameTable__init();
    VirtualFrameTable__init(& simpleInterface);
    size_t ids[32];
    for (size_t i = 0; i < 16; i++) {
        ids[i] = VirtualFrameTable__allocPage();
        assert(ids[i] == i + 1);
        VirtualFrameTable__pinPage(ids[i]);
        // VirtualFrameTable__unpinPage(ids[i]);
    }
    // printf("\nWrite Thing to page table\n");
    // for (size_t i = 0; i < 32; i++)
    // {
    //     size_t * dataPtr = VirtualFrameTable__getVaddrByPageRef(ids[i]);    
    //     *dataPtr = i;
    //     VirtualFrameTable__unpinPage(ids[i]);
    // }
    
    // printf("\nRead Thing to page table\n");
    // for (size_t i = 0; i < 32; i++)
    // {
    //     size_t * dataPtr = VirtualFrameTable__getVaddrByPageRef(ids[i]);    
    //     assert(*dataPtr == i);
    //     VirtualFrameTable__unpinPage(ids[i]);
    // }

    return 0;
}
