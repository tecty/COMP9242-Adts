#if !defined(VIRTUAL_FRAME_H)
#define VIRTUAL_FRAME_H

#include <stdlib.h>

#include "dynamicArrOne.h"

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
 * Public functions
 */
void VirtualFrame__init(virtualFrame_Interface_t inf);
void VirtualFrame__free();
size_t VirtualFrame__allocPage();
void VirtualFrame__pinPage(size_t vfref);
void VirtualFrame__unpinPage(size_t vfref);
// unpin required 
void * VirtualFrame__getVaddrByPageRef(size_t vfref);
void VirtualFrame__markPageDirty(size_t vfref);
size_t VirtualFrame__dupPageMap(size_t vfref);
size_t VirtualFrame__dupPageShare(size_t vfref);
mapContext_t VirtualFrame__getMapContext(size_t vfref);
void VirtualFrame__delPage(size_t vfref);
void VirtualFrame__pinPageArr(DynamicArrOne_t  vfrefArr);
void VirtualFrame__unpinPageArr(DynamicArr_t vfrefArr);
size_t VirtualFrame__getPinableFrameCount();

#endif // VIRTUAL_FRAME_H
