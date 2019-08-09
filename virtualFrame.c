#include "virtualFrame.h"

#include "doubleLinkList.h"
#include "dynamicQ.h"
#include "occupy.h"

#include <stdio.h>

#include <assert.h>

/**
 * Private
 */
#define DISK_START (1UL << 19)
#define INTERFACE (this.interface)
#define this VirtualFrame_s

typedef struct VirtualPage_s VirtualPage_t;
__attribute__((__packed__)) struct VirtualPage_s {
    size_t frame_id : 23;
    size_t cap : 32;
    size_t copy_on_write : 1;
    size_t mapped : 1;
};

typedef struct Frame_s* Frame_t;
__attribute__((__packed__)) struct Frame_s {
    size_t virtual_id : 23;
    size_t frame_ref : 19;
    size_t disk_id : 19;
    size_t pin : 1;
    size_t dirty : 1;
    size_t considered : 1;
};

typedef union {
    struct VirtualPage_s page;
    uint64_t number;
} intBridge_t;

uint64_t __PageToInt(struct VirtualPage_s page)
{
    intBridge_t b;
    b.page = page;
    return b.number;
}

struct VirtualPage_s __IntToPage(uint64_t number)
{
    intBridge_t b;
    b.number = number;
    return b.page;
}

typedef struct WakeContext_s {
    uint64_t thread;
    uint64_t pageCount;
} * WakeContext_t;

static struct {
    DoubleLinkList_t pageTable;
    DynamicArrOne_t frameTable;
    Occupy_t diskTable;
    virtualFrame_Interface_t interface;
    size_t lastConsider;
} VirtualFrame_s;

static uint64_t __dumpPageCB(uint64_t data, void* unused);

/**
 * Helper Functions that decouple things
 */

void __swapCallback(int64_t err, void* data)
{
    WakeContext_t context = data;
    context->pageCount--;
    if (context->pageCount == 0) {
        ;
    }
}
static inline void __yieldByContext(WakeContext_t context)
{
    if (context->pageCount != 0) {
        ;
    }
    assert(context->pageCount == 0);
}

static inline Frame_t __getFrameById(size_t frame_id)
{
    return DynamicArrOne__get(this.frameTable, frame_id);
}

uint64_t __updatePageToNewFrameCB(uint64_t data, void* private)
{
    VirtualPage_t page = __IntToPage(data);
    size_t newFrame = *(size_t*)private;
    // __dumpPageCB(data, NULL);
    page.frame_id = newFrame;
    // if (newFrame > DISK_START && page.mapped)
    // {
    //     // unmap if that's a disk frame
    //     INTERFACE->unMapCap(page.cap);
    //     page.mapped = 0;
    // }
    return __PageToInt(page);
}

uint64_t __markCoWCB(uint64_t data, void* unused)
{
    VirtualPage_t page = __IntToPage(data);
    page.copy_on_write = 1;
    return __PageToInt(page);
}

static inline void __pointPageToNewFrame(size_t vfref, size_t frame_id)
{
    DoubleLinkList__foreach(this.pageTable,
        DoubleLinkList__getRoot(this.pageTable, vfref),
        __updatePageToNewFrameCB, &frame_id);
}
static inline VirtualPage_t __getPageByVfref(size_t vfref)
{
    return __IntToPage(DoubleLinkList__get(this.pageTable, vfref));
}

/**
 * @pre: valid page
 * @pre: valid frame
 */
static inline Frame_t __getFrameByVfref(size_t vfref)
{
    VirtualPage_t page = __getPageByVfref(vfref);
    return __getFrameById(page.frame_id);
}

/**
 * Debug
 */

static inline void dumpFrame(Frame_t frame)
{
    if (frame == NULL) {
        printf("I have dump a null frame\n");
    }

    printf("Con:%u\tDir:%u\tPin:%u\tDiskFrame:%u\tFrameRef:%u\tVirP:%u\n",
        frame->considered, frame->dirty, frame->pin, frame->disk_id,
        frame->frame_ref, frame->virtual_id);
}
static inline void dumpFrameTable()
{
    printf("\n");
    size_t alloced = DynamicArrOne__getAlloced(this.frameTable);
    for (size_t i = 0; i < alloced; i++) {
        Frame_t frame = DynamicArrOne__get(this.frameTable, i + 1);
        dumpFrame(frame);
    }
}

static uint64_t __dumpPageCB(uint64_t data, void* unused)
{
    VirtualPage_t page = __IntToPage(data);
    printf("CoW:%u\tMaped:%u\t%s:%lu\tCap;%u\n", page.copy_on_write,
        page.mapped, page.frame_id < DISK_START ? "FtId" : "DkId",
        page.frame_id < DISK_START ? page.frame_id : page.frame_id - DISK_START,
        page.cap);
    if (page.frame_id < DISK_START && page.frame_id != 0)
        dumpFrame(__getFrameById(page.frame_id));
    return 0;
}

void dumpPage(size_t vfref)
{
    VirtualPage_t page = __getPageByVfref(vfref);
    printf("vfref:%lu\t", vfref);
    __dumpPageCB(__PageToInt(page), NULL);
}

static inline void dumpPageTable()
{
    DoubleLinkList__dumpEach(this.pageTable, __dumpPageCB, NULL);
}

/**
 * Body:
 */
void VirtualFrame__init(virtualFrame_Interface_t inf)
{
    assert(sizeof(struct Frame_s) == 8);
    assert(sizeof(struct VirtualPage_s) == 8);
    this.pageTable = DoubleLinkList__init();
    this.frameTable = DynamicArrOne__init(sizeof(uint64_t));
    this.diskTable = Occupy__init();
    this.interface = inf;
    this.lastConsider = 0;
}

void VirtualFrame__free()
{
    DoubleLinkList__free(this.pageTable);
    DynamicArrOne__free(this.frameTable);
    Occupy__free(this.diskTable);
}

size_t VirtualFrame__allocPage()
{
    VirtualPage_t vpt;
    vpt.cap = INTERFACE->allocCspace();
    vpt.mapped = 0;
    vpt.copy_on_write = 0;
    /**
     * NULL frame, when some operation show there's an acutal need
     * for the page, we then try to alloc a actual frame to it
     */
    vpt.frame_id = 0;
    // add it to page table to manage
    return DoubleLinkList__add(this.pageTable, __PageToInt(vpt));
}

void __incLastConsider()
{
    this.lastConsider++;
    this.lastConsider %= DynamicArrOne__getAlloced(this.frameTable);
}

// TODO: change the interface in the array mapping
uint64_t __unmapCB(uint64_t pageInt, void* unused)
{
    VirtualPage_t page = __IntToPage(pageInt);
    if (page.mapped)
        INTERFACE->unMapCap(page.cap);
    page.mapped = 0;
    return __PageToInt(page);
}

/**
 * @post: the request frame is alway pined to prevent to be swapout while
 * a swapping is alreading taking in plave
 * @post frame -> pin == 1
 */
size_t __requestFrame(WakeContext_t context)
{
    size_t allocedRef = INTERFACE->allocFrame();
    if (allocedRef != 0) {
        context->pageCount--;
        struct Frame_s frame;
        frame.considered = 0;
        frame.dirty = 0;
        frame.pin = 1;
        frame.virtual_id = 0;
        frame.disk_id = 0;
        frame.frame_ref = allocedRef;
        return DynamicArrOne__add(this.frameTable, &frame);
    }

    /**
     * Try to find a victim and swap it out
     */
    size_t victim = 0;
    Frame_t currFrame;
    size_t frameTableSize = DynamicArrOne__getAlloced(this.frameTable);
    for (size_t i = 0; i < 2 * frameTableSize && victim == 0; i++) {
        __incLastConsider();
        size_t curr = this.lastConsider + 1;
        currFrame = __getFrameById(curr);
        if (currFrame->pin)
            continue;
        if (currFrame->considered) {
            victim = curr;
            break;
        } else {
            currFrame->considered = 1;
        }
    }
    assert(victim != 0);

    // swap out the victim
    if (currFrame->disk_id == 0 && currFrame->dirty) {
        currFrame->disk_id = Occupy__alloc(this.diskTable);
    }

    if (currFrame->dirty || currFrame->disk_id != 0) {
        // pin this frame for swapping
        currFrame->pin = 1;

        // before swapped out, these page should be unmap
        DoubleLinkList__foreach(this.pageTable,
            DoubleLinkList__getRoot(this.pageTable, currFrame->virtual_id),
            __unmapCB, NULL);

        INTERFACE->swapOutFrame(
            currFrame->frame_ref, currFrame->disk_id, __swapCallback, context);

        // point all the page point to this frame to the disk frame
        __pointPageToNewFrame(
            currFrame->virtual_id, currFrame->disk_id + DISK_START);

    } else {
        // I don't need to swap out, release the lock for this frame
        context->pageCount--;
        __pointPageToNewFrame(currFrame->virtual_id, 0);
    }

    // reset the frame information
    currFrame->dirty = 0;
    currFrame->disk_id = 0;
    currFrame->virtual_id = 0;
    currFrame->considered = 0;
    return victim;
}

/**
 * @pre: vfref is a valid page
 * @post: page is on frame
 * @post: page will point to a valid frmae
 */
void __requestPage(size_t vfref)
{
    VirtualPage_t page = __getPageByVfref(vfref);
    // page is in the frame
    if (page.frame_id > 0 && page.frame_id < DISK_START)
        return;

    // clean frame is needed
    struct WakeContext_s context;
    context.pageCount = 1;
    context.thread = 0;
    size_t victim = __requestFrame(&context);
    __yieldByContext(&context);
    Frame_t victimFrame = __getFrameById(victim);

    victimFrame->virtual_id = vfref;

    if (page.frame_id > DISK_START) {
        context.pageCount = 1;
        // swapin
        INTERFACE->swapInFrame(
            victim, page.frame_id - DISK_START, __swapCallback, &context);
        __yieldByContext(&context);
        // assign new disk id to it
        victimFrame->disk_id = page.frame_id - DISK_START;
        page.frame_id = victim;
    } else if (page.frame_id == 0) {
        // clean the page data
        size_t* data = INTERFACE->getFrameVaddr(victim);
        *data = 0;
        // TODO:
        // memset(
        //     INTERFACE->getFrameVaddr(victimFrame->frame_ref), 0, (1<< 12));
    } else {
        // there shouldn't be a state to get here
        assert(1 == 0);
    }

    // save the page data
    page.frame_id = victim;
    DoubleLinkList__update(this.pageTable, vfref, __PageToInt(page));
    __pointPageToNewFrame(vfref, victim);
    // unpin since the swapping ins finished
    victimFrame->pin = 0;
}

void VirtualFrame__pinPage(size_t vfref)
{
    assert(DynamicArrOne__get(this.pageTable, vfref) != NULL);
    __requestPage(vfref);
    Frame_t frame = __getFrameByVfref(vfref);
    assert(frame != NULL);
    frame->pin = 1;
}

void VirtualFrame__unpinPage(size_t vfref)
{
    assert(DynamicArrOne__get(this.pageTable, vfref) != NULL);
    VirtualPage_t page = __getPageByVfref(vfref);
    if (page.frame_id == 0 || page.frame_id > DISK_START)
        return;

    Frame_t frame = __getFrameById(page.frame_id);
    assert(frame != NULL);
    frame->pin = 0;
}

// TODO: COW
void VirtualFrame__markPageDirty(size_t vfref)
{
    assert(DynamicArrOne__get(this.pageTable, vfref) != NULL);
    __requestPage(vfref);
    // printf("Dump page at mark dirty\n");
    // dumpPage(vfref);
    Frame_t frame = __getFrameByVfref(vfref);
    assert(frame != NULL);
    frame->dirty = 1;
}

void* VirtualFrame__getVaddrByPageRef(size_t vfref)
{
    assert(DynamicArrOne__get(this.pageTable, vfref) != NULL);
    __requestPage(vfref);
    Frame_t frame = __getFrameByVfref(vfref);
    return INTERFACE->getFrameVaddr(frame->frame_ref);
}

// before you write to make it consistant
// make sure it
// last page which have CoW, then no swap out is needed

// dup for just map reading
size_t VirtualFrame__dupPageMap(size_t vfref)
{
    if (DynamicArrOne__get(this.pageTable, vfref) == NULL)
        return 0;

    VirtualPage_t page = __getPageByVfref(vfref);
    page.cap = INTERFACE->allocCspace();
    page.mapped = 0;
    size_t ret = DoubleLinkList__add(this.pageTable, __PageToInt(page));
    DoubleLinkList__link(this.pageTable, vfref, ret);
    return ret;
}

// dup for share frame
// TODO: Cow
size_t VirtualFrame__dupPageShare(size_t vfref)
{
    size_t ret = VirtualFrame__dupPageMap(vfref);
    VirtualPage_t page = __getPageByVfref(ret);
    page.copy_on_write = 1;

    DoubleLinkList__update(this.pageTable, ret, __PageToInt(page));
    DoubleLinkList__foreach(this.pageTable,
        DoubleLinkList__getRoot(this.pageTable, vfref), __markCoWCB, NULL);
    return ret;
}

mapContext_t VirtualFrame__getMapContext(size_t vfref)
{
    mapContext_t context;
    VirtualPage_t page = __getPageByVfref(vfref);
    if (page.frame_id > DISK_START || page.frame_id == 0) {
        /**
         * this will take a long time and yield
         */
        VirtualFrame__pinPage(vfref);
        // update the new page info
        page = __getPageByVfref(vfref);
        VirtualFrame__unpinPage(vfref);
    }
    Frame_t frame = __getFrameById(page.frame_id);
    frame->considered = 0;

    // if (vfref == 10) dumpPage(vfref);

    if (!page.mapped) {
        // if (
        //     DoubleLinkList__get(this.pageTable,vfref) == 0
        // ){
        //     printf(
        //         "vfref %lu, Original number is %lu\n",
        //         vfref,
        //         DoubleLinkList__get(this.pageTable,vfref)
        //     );
        // }
        // printf("==> Cap has copied while get map context\n");
        INTERFACE->copyFrameCap(frame->frame_ref, page.cap);

        page.mapped = 1;
        // store the new mapped page
        DoubleLinkList__update(this.pageTable, vfref, __PageToInt(page));
    } else {
        // there's a mapped frame in there
        INTERFACE->unMapCap(page.cap);
        INTERFACE->copyFrameCap(frame->frame_ref, page.cap);
    }
    context.write = frame->dirty;
    context.pageCap = page.cap;

    return context;
}

void VirtualFrame__delPage(size_t vfref)
{
    VirtualPage_t page = __getPageByVfref(vfref);
    // dumpPage(vfref);
    INTERFACE->delCap(page.cap);
    size_t candidateRoot = DoubleLinkList__delink(this.pageTable, vfref);
    if (candidateRoot == 0) {
        // this is the last page: free the disk and frame
        if (page.frame_id > 0 && page.frame_id < DISK_START) {
            Frame_t frame = __getFrameById(page.frame_id);
            frame->considered = 1;
            // printf("===> Unset frame %u dirty by delete page\n",
            // page.frame_id );
            frame->dirty = 0;
            frame->disk_id = 0;
            frame->pin = 0;
            frame->virtual_id = 0;
            // free the disk
            Occupy__del(this.diskTable, frame->disk_id);
        } else if (page.frame_id != 0) {
            // free the disk
            Occupy__del(this.diskTable, page.frame_id);
        }
    } else {
        if (page.frame_id < DISK_START && page.frame_id) {
            Frame_t frame = __getFrameById(page.frame_id);
            if (frame->virtual_id == vfref) {
                // point back the candidate root
                frame->virtual_id = candidateRoot;
            }
        }
    }
    DoubleLinkList__del(this.pageTable, vfref);
}

typedef struct pinFrameContext_s {
    struct WakeContext_s wakeContext;
    DynamicQ_t frameQ;
} * pinFrameContext_t;

/**
 * Pin and unPin by Array
 */
void __requestFrmaeCB(void* data, void* private)
{
    size_t vfref = *(size_t*)data;
    pinFrameContext_t context = private;
    VirtualPage_t page = __getPageByVfref(vfref);

    if (page.frame_id > 0 && page.frame_id < DISK_START) {
        // this is a frame we needed
        Frame_t frame = __getFrameById(page.frame_id);
        frame->pin = 1;
        frame->considered = 0;
        context->wakeContext.pageCount--;
        return;
    }

    // request a frame
    size_t frameID = __requestFrame(&context->wakeContext);
    DynamicQ__enQueue(context->frameQ, &frameID);
}

void __swapInPageCB(void* data, void* private)
{
    size_t vfref = *(size_t*)data;
    pinFrameContext_t context = private;

    VirtualPage_t page = __getPageByVfref(vfref);
    if (page.frame_id > 0 && page.frame_id < DISK_START) {
        // not need to do anything ,short path
        context->wakeContext.pageCount--;
        return;
    }

    size_t frameId = *(size_t*)DynamicQ__first(context->frameQ);
    DynamicQ__deQueue(context->frameQ);
    Frame_t frame = __getFrameById(frameId);

    if (page.frame_id > DISK_START) {
        /* swapin is needed */
        INTERFACE->swapInFrame(frame->frame_ref, page.frame_id - DISK_START,
            __swapCallback, &context->wakeContext);
        frame->disk_id = page.frame_id - DISK_START;
    } else {
        frame->disk_id = 0;
        context->wakeContext.pageCount--;
    }

    frame->virtual_id = vfref;

    // page need to point to this frame
    __pointPageToNewFrame(vfref, frameId);
}

void VirtualFrame__pinPageArr(DynamicArrOne_t vfrefArr)
{
    struct pinFrameContext_s context;
    context.wakeContext.thread = 0;
    context.wakeContext.pageCount = DynamicArrOne__getAlloced(vfrefArr);
    context.frameQ = DynamicQ__init(sizeof(size_t));

    /**
     * swap out
     */
    DynamicArrOne__foreach(vfrefArr, __requestFrmaeCB, &context);
    // wait for all page is swapped in;
    __yieldByContext(&context.wakeContext);

    /**
     * swap in
     */
    context.wakeContext.pageCount = DynamicArrOne__getAlloced(vfrefArr);
    DynamicArrOne__foreach(vfrefArr, __swapInPageCB, &context);
    __yieldByContext(&context.wakeContext);
}

void __unpinCB(void* data, void* unused)
{
    size_t vfref = *(size_t*)data;
    VirtualFrame__unpinPage(vfref);
}

void VirtualFrame__unpinPageArr(DynamicArr_t vfrefArr)
{
    // trivialy calling the foreach
    DynamicArrOne__foreach(vfrefArr, __unpinCB, NULL);
}

size_t VirtualFrame__getPinableFrameCount()
{
    return DynamicArrOne__getAlloced(this.frameTable) / 2;
}

void VirtualFrame__flushPage(size_t vfref)
{
    VirtualPage_t page = __getPageByVfref(vfref);
    if (page.frame_id == 0 || page.frame_id > DISK_START)
        return;
    Frame_t frame = __getFrameById(page.frame_id);
    if (frame == NULL)
        return;
    INTERFACE->flushFrame(frame->frame_ref);
}

size_t VirtualFrame__getFrameCapByPage(size_t vfref)
{
    VirtualPage_t page = __getPageByVfref(vfref);
    if (page.frame_id == 0 || page.frame_id > DISK_START) {
        VirtualFrame__pinPage(vfref);
        page = __getPageByVfref(vfref);
        VirtualFrame__unpinPage(vfref);
    }
    Frame_t frame = __getFrameById(page.frame_id);
    return INTERFACE->getFrameCap(frame->frame_ref);
}

/**
 * Main
 */

#include "frametable.h"
static struct virtualFrame_Interface_s simpleInterface
    = { .allocFrame = FrameTable__allocFrame,
          .allocCspace = FrameTable__allocCspace,
          .getFrameVaddr = FrameTable__getFrameVaddr,
          .swapOutFrame = FrameTable__swapOutFrame,
          .swapInFrame = FrameTable__swapInFrame,
          .delCap = FrameTable__delCap,
          .unMapCap = FrameTable__unMapCap,
          .copyFrameCap = FrameTable__copyFrameCap,
          .getFrameCap = FrameTable__getFrameCap };

int main(int argc, char const* argv[])
{
    FrameTable__init();
    VirtualFrame__init(&simpleInterface);
    size_t ids[32];
    for (size_t i = 0; i < 32; i++) {
        ids[i] = VirtualFrame__allocPage();
        assert(ids[i] == i + 1);
        VirtualFrame__pinPage(ids[i]);
        VirtualFrame__unpinPage(ids[i]);
        // printf("==> Im here %d\n", i);
    }

    // VirtualFrame__pinPage(ids[16]);
    // printf("\nPretend swapping\n");
    // for (size_t i = 0; i < 16; i++)
    // {
    //     struct WakeContext_s context ;
    //     context.thread = 0;
    //     context.pageCount = 1;
    //     VirtualFrame__requestFrame(&context);
    // }
    // dumpPageTable();

    // for (size_t i = 0; i < 16; i++)
    // {
    //     assert(__getPageByVfref(ids[i]).frame_id == 0);
    //     assert(__getPageByVfref(ids[i+16]).frame_id != 0);
    // }

    /***
     * Simple swaping
     */
    for (size_t i = 0; i < 32; i++) {
        size_t* dataPtr = VirtualFrame__getVaddrByPageRef(ids[i]);
        VirtualFrame__markPageDirty(ids[i]);
        *dataPtr = i;
        VirtualFrame__unpinPage(ids[i]);
    }

    for (size_t i = 0; i < 32; i++) {
        size_t* dataPtr = VirtualFrame__getVaddrByPageRef(ids[i]);
        assert(*dataPtr == i);
        VirtualFrame__unpinPage(ids[i]);
    }
    for (size_t i = 0; i < 32; i++) {
        size_t* dataPtr = VirtualFrame__getVaddrByPageRef(ids[i]);
        VirtualFrame__markPageDirty(ids[i]);
        *dataPtr *= 4;
        VirtualFrame__unpinPage(ids[i]);
    }

    for (size_t i = 0; i < 32; i++) {
        size_t* dataPtr = VirtualFrame__getVaddrByPageRef(ids[i]);
        assert(*dataPtr == i * 4);
        VirtualFrame__unpinPage(ids[i]);
    }

    /**
     * Pin Bit and Dirty consistancy
     * TODO:
     */
    for (size_t i = 0; i < 16; i++) {
        size_t dupId[4];
        for (size_t j = 0; j < 4; j++) {
            dupId[j] = VirtualFrame__dupPageMap(ids[i + j]);
            // printf("==> Dup: %u == %u\n", dupId[j], ids[i + j]);
            VirtualFrame__pinPage(dupId[j]);
            VirtualFrame__markPageDirty(dupId[j]);
            struct mapContext_s map = VirtualFrame__getMapContext(dupId[j]);
            FrameTable__mapCap(map.pageCap);
        }
        for (size_t j = 0; j < 4; j++) {
            // assert the properties
            VirtualPage_t page = __getPageByVfref(dupId[j]);
            // printf("==> Now dup page has info ");
            // dumpPage(dupId[j]);
            assert(page.copy_on_write == 0);
            assert(page.mapped == 1);
            Frame_t frame = __getFrameById(page.frame_id);
            assert(frame != NULL);
            assert(frame->pin == 1);
            assert(frame->dirty == 1);
            // assert(frame->virtual_id  == ids[j]);
            assert(frame->considered == 0);
            VirtualFrame__unpinPage(dupId[j]);
        }
        for (size_t j = 0; j < 4; j++) {
            VirtualFrame__delPage(dupId[j]);
        }

        // dumpPageTable();
    }

    /**
     * Delete page will have a clean page table
     */

    // printf("\nAfter swapping\n");
    // dumpPageTable();
    // no matter how much time it swap,
    //     the data still in the disk

    // no matter how, all the structure will be free
    for (size_t i = 0; i < 32; i++) {
        VirtualFrame__delPage(ids[i]);
        // if (i != 0) VirtualFrame__delPage(share_ids[i]);
    }

    // here shouldn't dump anything
    // dumpPageTable();
    // FrameTable__dump();

    return 0;
}
