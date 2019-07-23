#include "doubleLinkList.h"
#include "dynamicQ.h"
#include "occupy.h"

#include <assert.h>

typedef void (* virtual_frame_table_callback_t)(int64_t, void *);

typedef struct virtualFrame_Interface_s
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
    void (*copyFrameCap)(size_t frameref, size_t dest);
    void (*delCap)(size_t cap);
    void (*unMapCap)(size_t cap);
} * virtualFrame_Interface_t;



typedef struct mapContext_s mapContext_t;
struct mapContext_s
{
    size_t pageCap:32;
    size_t write:2;
};

/**
 * Private 
 */
#define DISK_START (1UL << 19)
#define INTERFACE (This.interface)
#define This (VirtualFrame_s)

typedef struct VirtualPage_s VirtualPage_t;
__attribute__((__packed__))  struct VirtualPage_s
{
    size_t frame_id : 23;
    size_t cap :32;
    char copy_on_write: 2;
    char mapped: 1;
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
    virtualFrame_Interface_t interface;
    size_t lastConsider;
} VirtualFrame_s;



/**
 * Helper Functions that decouple things
 */

void __swapCallback(int64_t err, void * data){
    WakeContext_t context = data; 
    context->pageCount --;
    if (context->pageCount ==0)
    {
        // wake up 
    }
}
static inline void __yieldByContext(WakeContext_t context){
    if (context->pageCount != 0)
    {
        // yield here 
    }
}

static inline Frame_t __getFrameById(size_t frame_id){
    return DynamicArrOne__get(This.frameTable, frame_id);
}
#include <stdio.h>

size_t __newFrame; 
uint64_t  __updatePageToNewFrameCB(uint64_t data){
    VirtualPage_t page = __IntToPage(data);
    page.frame_id = __newFrame;
    if (__newFrame > DISK_START && page.mapped)
    {
        // unmap if that's a disk frame 
        INTERFACE->unMapCap(page.cap);
        page.mapped = 0;
    }
    return __PageToInt(page);
}

uint64_t __markCoWCB(uint64_t data){
    VirtualPage_t page = __IntToPage(data);
    page.copy_on_write = 1;
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

void __dumpPageCB(void * data, void * unused){
    VirtualPage_t page = __IntToPage(*(uint64_t *)data);
    printf("CoW:%u\tMaped:%u\t%s:%u\tCap;%u\n",
        page.copy_on_write,
        page.mapped,
        page.frame_id < DISK_START ? "FtId": "DkId" ,
        page.frame_id < DISK_START ? page.frame_id : page.frame_id -DISK_START ,
        page.cap 
    );
    if (page.frame_id < DISK_START && page.frame_id != 0)
        dumpFrame(__getFrameById(page.frame_id));
}

void dumpPage(size_t vfref){
    VirtualPage_t page = __getPageByVfref(vfref);
    printf("vfref:%u\t", vfref);
    __dumpPageCB(&page, NULL);
}

void dumpPageTable(){
    DynamicArrOne__foreach(This.pageTable, __dumpPageCB, NULL);
}


/**
 * Body:
 */
void VirtualFrame__init(virtualFrame_Interface_t inf){
    assert( sizeof(struct Frame_s)  == 8);
    assert( sizeof(struct VirtualPage_s)  == 8);
    This.pageTable = DoubleLinkList__init();
    This.frameTable = DynamicArrOne__init(sizeof(uint64_t));
    This.diskTable = Occupy__init();
    This.interface = inf;
    This.lastConsider = 0;
}

void VirtualFrame__free(){
    DoubleLinkList__free(This.pageTable);
    DynamicArrOne__free(This.frameTable);
    Occupy__free(This.diskTable);
}

size_t VirtualFrame__allocPage(){
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


void __incLastConsider(){
    This.lastConsider ++;
    This.lastConsider %= DynamicArrOne__getAlloced(This.frameTable);
}
/**
 * @post: the request frame is alway pined to prevent to be swapout while 
 * a swapping is alreading taking in plave 
 */
size_t VirtualFrame__requestFrame(WakeContext_t context){
    size_t allocedRef = INTERFACE->allocFrame();
    if (allocedRef != 0) {
        context->pageCount --;
        struct Frame_s frame;
        frame.considered =0;
        frame.dirty = 0;
        frame.pin = 1;
        frame.virtual_id = 0;
        frame.disk_id = 0;
        frame.frame_ref = allocedRef;
        return DynamicArrOne__add(This.frameTable, &frame);
    }


    Frame_t curr ; 
    while (1)
    {
        __incLastConsider();
        curr = DynamicArrOne__get(This.frameTable, This.lastConsider + 1);
        if (curr ->pin) continue;
        if (curr->considered == 0)
        {
            curr->considered = 1;
            continue;
        }
        break;
    }
    
    /**
     * Swap out 
     */
    if (curr->dirty) {
        /**
         * Swap out
         */
        if(curr->disk_id == 0){
            curr->disk_id= Occupy__alloc(This.diskTable);
        }
        INTERFACE->swapOutFrame(
            curr->frame_ref, curr->disk_id, __swapCallback, context
        );
        // printf("swap out %u\n", curr->virtual_id);
    }

    if (curr->disk_id == 0)
    {
        // point to empty null frame 
        __pointPageToNewFrame(curr->virtual_id, 0);
    } else {
        // point to the disk page 
        __pointPageToNewFrame(curr->virtual_id, DISK_START + curr->disk_id);
    }
    VirtualPage_t  page =  __getPageByVfref(curr->virtual_id);

    curr->virtual_id = 0;
    curr->considered = 0;
    curr->dirty =0;
    curr->pin = 1;
    // dumpFrame(curr);
    return This.lastConsider + 1;
}

void VirtualFrame__pinPage(size_t vfref){
    VirtualPage_t page = __getPageByVfref(vfref);
    if (page.frame_id > 0 && page.frame_id < DISK_START)
    {
        __getFrameById(page.frame_id)->pin = 1;
        return ;
    }
    
    struct WakeContext_s context;
    context.thread = 0;
    context.pageCount = 1;

    size_t requestedFrame= VirtualFrame__requestFrame(&context);
    __getFrameById(requestedFrame)->virtual_id = vfref;
    __yieldByContext(&context);

    // swap in if needed 
    if (page.frame_id > DISK_START)
    {
        context.pageCount = 1;
        INTERFACE->swapInFrame(
            requestedFrame, page.frame_id - DISK_START,
            __swapCallback, &context
        );
        __yieldByContext(&context);

        // printf("I swapped in %lu\n", *(uint64_t *)INTERFACE->getFrameVaddr(requestedFrame));
        // point the frame to the disk frame
        __getFrameById(requestedFrame)->disk_id = page.frame_id - DISK_START;
    }else {
        __getFrameById(requestedFrame)->disk_id = 0;
    }
    
    
    __pointPageToNewFrame(vfref, requestedFrame);
}

void VirtualFrame__unpinPage(size_t vfref){
    VirtualPage_t page = __getPageByVfref(vfref);
    if (page.frame_id == 0) return ;
    
    Frame_t frame = __getFrameById(page.frame_id);
    frame->pin = 0;
}

void * VirtualFrame__getVaddrByPageRef(size_t vfref){
    VirtualFrame__pinPage(vfref);
    Frame_t frame = __getFrameById(__getPageByVfref(vfref).frame_id);
    return INTERFACE->getFrameVaddr(frame->frame_ref);
}

// before you write to make it consistant
// make sure it 
// last page which have CoW, then no swap out is needed 
void VirtualFrame__markPageDirty(size_t vfref){
    size_t srcInt = DoubleLinkList__get(This.pageTable, vfref);
    if (srcInt == 0) return;
    VirtualPage_t src = __IntToPage(srcInt);
    Frame_t frame = __getFrameById(src.frame_id);

    if (src.copy_on_write) {
        size_t candidateRoot = DoubleLinkList__delink(This.pageTable, vfref);
        src.copy_on_write = 0;

        if (candidateRoot) {
            // there still some page point to it 
            if (frame->dirty) {
                if ( ! frame->disk_id) {
                    // swapout to have a new idsk 
                    frame->disk_id = Occupy__alloc(This.diskTable);
                } 
                struct WakeContext_s context;
                context.thread = 0;
                context.pageCount = 1;
                INTERFACE->swapOutFrame(
                    src.frame_id, frame->disk_id, __swapCallback, &context
                );
                __yieldByContext(&context);
            }
            

            // point to a disk page 
            __pointPageToNewFrame(
                candidateRoot, 
                frame->disk_id ? frame->disk_id + DISK_START: 0 
            );

            // this frame have a null disk 
            frame->disk_id = 0;
            frame->virtual_id = vfref;
        }
        // save the page, since we update the cow 
        DoubleLinkList__update(This.pageTable, vfref, __PageToInt(src));
    }

    // mark this frame as dirty 
    frame->dirty = 1;
    frame->considered = 0;
}

// dup for just map reading 
size_t VirtualFrame__dupPageMap(size_t vfref){
    size_t srcInt = DoubleLinkList__get(This.pageTable, vfref);
    // page not found 
    if (srcInt == 0 ) return 0;
    VirtualPage_t src = __IntToPage(srcInt);
    size_t destRef = VirtualFrame__allocPage();
    VirtualPage_t dest = __getPageByVfref(destRef);
    dest.frame_id = src.frame_id;
    // link up thoes pages 
    DoubleLinkList__link(This.pageTable, vfref, destRef);
    DoubleLinkList__update(This.pageTable, destRef, __PageToInt(dest));
    return destRef;   
}


// dup for share frame
size_t VirtualFrame__dupPageShare(size_t vfref){
    size_t ret = VirtualFrame__dupPageMap(vfref);
    VirtualPage_t page = __getPageByVfref(ret);
    page.copy_on_write = 1;

    DoubleLinkList__update(This.pageTable, ret, __PageToInt(page));
    DoubleLinkList__foreach(
        This.pageTable,
        DoubleLinkList__getRoot(This.pageTable, vfref),
        __markCoWCB
    );
    return ret;
}

mapContext_t VirtualFrame__getMapContext(size_t vfref){
    mapContext_t context;
    VirtualPage_t page = __getPageByVfref(vfref);
    if (page.frame_id > DISK_START || page.frame_id == 0) {
        /**
         * This will take a long time and yield
         */
        VirtualFrame__pinPage(vfref);
        // update the new page info 
        page = __getPageByVfref(vfref);
    }
    Frame_t frame =  __getFrameById(page.frame_id);
    if (! page.mapped) {
        INTERFACE->copyFrameCap(frame->frame_ref, page.cap);
        page.mapped = 1;
        // store the new mapped page 
        DoubleLinkList__update(This.pageTable, vfref, __PageToInt(page));
    }
    context.write = frame->dirty;
    context.pageCap = page.cap;        

    return context;
}

void VirtualFrame__delPage(size_t vfref){
    VirtualPage_t page = __getPageByVfref(vfref);
    // dumpPage(vfref);
    INTERFACE->delCap(page.cap);
    size_t candidateRoot = DoubleLinkList__delink(This.pageTable, vfref);
    if(candidateRoot ==0){
        // this is the last page: free the disk and frame 
        if (page.frame_id > 0 && page.frame_id < DISK_START)
        {
            Frame_t frame = __getFrameById(page.frame_id);
            frame->considered  = 1;
            frame->dirty = 0;
            frame->disk_id = 0;
            frame->pin =0;
            frame->virtual_id = 0;
            // free the disk 
            Occupy__del(This.diskTable, frame->disk_id);
        } else if (page.frame_id != 0) {
            // free the disk 
            Occupy__del(This.diskTable, page.frame_id);
        }
    } else {
        Frame_t frame = __getFrameById(page.frame_id);
        if( frame ->virtual_id == vfref){
            // point back the candidate root
            frame->virtual_id = candidateRoot;
        }
    }
    DoubleLinkList__del(This.pageTable, vfref);
}

/**
 * Main
 */

#include "frametable.h"
static struct virtualFrame_Interface_s simpleInterface = {
    .allocFrame    = FrameTable__allocFrame,
    .allocCspace   = FrameTable__allocCspace,
    .getFrameVaddr = FrameTable__getFrameVaddr,
    .swapOutFrame  = FrameTable__swapOutFrame,
    .swapInFrame   = FrameTable__swapInFrame,
    .delCap        = FrameTable__delCap,
    .unMapCap      = FrameTable__unMapCap,
    .copyFrameCap  = FrameTable__copyFrameCap
};

int main(int argc, char const *argv[])
{
    FrameTable__init();
    VirtualFrame__init(& simpleInterface);
    size_t ids[32];
    for (size_t i = 0; i < 32; i++) {
        ids[i] = VirtualFrame__allocPage();
        assert(ids[i] == i + 1);
        VirtualFrame__pinPage(ids[i]);
        VirtualFrame__unpinPage(ids[i]);
    }

    // printf("\nPretend swapping\n");
    // for (size_t i = 0; i < 16; i++)
    // {
    //     struct WakeContext_s context ;
    //     context.thread = 0;
    //     context.pageCount = 1;
    //     VirtualFrame__requestFrame(&context);
    // }
    // dumpPageTable();

    for (size_t i = 0; i < 16; i++)
    {
        assert(__getPageByVfref(ids[i]).frame_id == 0);
        assert(__getPageByVfref(ids[i+16]).frame_id != 0);
    }
    for (size_t i = 0; i < 32; i++)
    {
        size_t * dataPtr = VirtualFrame__getVaddrByPageRef(ids[i]);    
        VirtualFrame__markPageDirty(ids[i]);
        *dataPtr = i;
        VirtualFrame__unpinPage(ids[i]);
    }
    for (size_t i = 0; i < 32; i++)
    {
        size_t * dataPtr = VirtualFrame__getVaddrByPageRef(ids[i]);    
        assert(*dataPtr == i);
        VirtualFrame__unpinPage(ids[i]);
    }
    for (size_t i = 0; i < 32; i++)
    {
        size_t * dataPtr = VirtualFrame__getVaddrByPageRef(ids[i]);    
        VirtualFrame__markPageDirty(ids[i]);
        *dataPtr *= 4;
        VirtualFrame__unpinPage(ids[i]);
    }

    for (size_t i = 0; i < 32; i++)
    {
        size_t * dataPtr = VirtualFrame__getVaddrByPageRef(ids[i]);    
        assert(*dataPtr == i*4);
        VirtualFrame__unpinPage(ids[i]);
    }

    size_t share_ids[32];
    share_ids[0] = ids[0];
    for (size_t i = 1; i < 32; i++){
        share_ids[i] = VirtualFrame__dupPageShare(ids[0]);
    }

    for (size_t i = 0; i < 32; i++) {
        // all the page has the data of original frame 
        uint64_t * data  = VirtualFrame__getVaddrByPageRef(share_ids[i]);
        assert(* data == 0 );
        // update will be duplicate 
        VirtualFrame__markPageDirty(share_ids[i]);
        *data = i;
        assert(* data == i);
        VirtualFrame__unpinPage(share_ids[i]);
    }

    // dumpPage(share_ids[0]);
    // dumpPage(share_ids[1]);

    /**
     * Data read back from the disks will be seperate 
     */
    for (size_t i = 0; i < 32; i++)
    {
        uint64_t * data  = VirtualFrame__getVaddrByPageRef(share_ids[i]);
        // now I can read those new value and it's independent 
        assert(* data == i);
        // printf("share %u see %lu\n", i, *data);
        // dumpPage(share_ids[i]);
        VirtualFrame__unpinPage(share_ids[i]);
    }
    dumpPageTable();
    
    for (size_t i = 0; i < 32; i++)
    {
        uint64_t * data  = VirtualFrame__getVaddrByPageRef(share_ids[i]);
        // now I can read those new value and it's independent 
        assert(* data == i);
        // printf("share %u see %lu\n", i, *data);
        // dumpPage(share_ids[i]);
        VirtualFrame__unpinPage(share_ids[i]);
    }
    // no matter how, all the structure will be free 

    for (size_t i = 0; i < 32; i++)
    {
        VirtualFrame__delPage(ids[i]);
        if (i != 0) VirtualFrame__delPage(share_ids[i]);
    }
    


    return 0;
}
