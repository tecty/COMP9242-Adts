#include "addressRegion.h"
#include "contRegion.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define PROCESS_MMAP_START (0xB000000000)

typedef struct AddressRegion_Region_s
{
    enum addressRegionTypes_e type;
    uint64_t start;
    uint64_t size;
    ContinueRegion_Region_t crrt;
} * AddressRegion_Region_t;

struct AddressRegion_s
{
    /* data */
    DynamicArr_t regions;
    ContinueRegion_t mapRegions;
};

AddressRegion_t AddressRegion__init()
{
    AddressRegion_t ret = malloc(sizeof(struct AddressRegion_s));
    ret->regions = DynamicArr__init(sizeof(struct AddressRegion_Region_s));
    ret->mapRegions = ContinueRegion__init();

    return ret;
}

void __dumpRegionRegion(AddressRegion_Region_t region)
{
    printf(
        "=> start %lu\tsize %lu\tcrrt: %p\tType:%u\n",
        region->start, region->size, region->crrt, region->type);
}

/**
 * @ret: the id to the inside managed strcuture 
 */
void AddressRegion__declare(
    AddressRegion_t ar, enum addressRegionTypes_e type,
    void *start, uint64_t size)
{
    struct AddressRegion_Region_s arrs = {
        .type = type,
        .size = (uint64_t)size,
        .start = (uint64_t)start,
        .crrt = NULL};

    if (type == STACK)
    {
        arrs.start = (uint64_t)start - (uint64_t)size;
    }

    DynamicArr__add(ar->regions, &arrs);
}

static inline bool __strictInRegion(
    struct AddressRegion_Region_s *arrs, void *vaddr)
{
    return (
        arrs->start <= (uint64_t)vaddr &&
        arrs->start + arrs->size > (uint64_t)vaddr);
}

typedef struct findRegionContext_s
{
    AddressRegion_Region_t found;
    enum addressRegionTypes_e type;
    void *vaddr;

} * findRegionContext_t;

static void __inRegionCB(void *data, void *pdata)
{
    findRegionContext_t context = pdata;
    AddressRegion_Region_t arrt = data;

    if (__strictInRegion(data, context->vaddr))
    {
        context->found = data;
    }
}

enum addressRegionTypes_e AddressRegion__isInRegion(
    AddressRegion_t ar, void *index)
{
    struct findRegionContext_s context;
    context.vaddr = index;
    context.found = NULL;

    DynamicArr__foreach(ar->regions, __inRegionCB, &context);

    // not found the region
    if (context.found == NULL)
        return 0;
    return context.found->type;
}

static void __foundByTypeCB(void *data, void *pdata)
{
    findRegionContext_t context = pdata;
    AddressRegion_Region_t arrt = data;
    if (arrt->type == context->type && context->found == NULL)
    {
        context->found = data;
    }
}

static AddressRegion_Region_t __getRegionByType(
    AddressRegion_t ar, enum addressRegionTypes_e type)
{
    struct findRegionContext_s context;
    context.type = type;
    context.found = NULL;

    DynamicArr__foreach(ar->regions, __foundByTypeCB, &context);
    return context.found;
}

/* This might be use by mmap */
void AddressRegion__regionAddSize(
    AddressRegion_t ar, enum addressRegionTypes_e type, size_t add_size)
{
    // return null to indecate this item is delted
    struct AddressRegion_Region_s *the_region = __getRegionByType(ar, type);
    if (!the_region)
        return;

    // the region doesn't found
    assert(the_region->type == type);
    if (type == STACK)
    {
        the_region->start -= add_size;
        return;
    }
    the_region->size += add_size;
}

static inline uint64_t __size4kAlign(uint64_t size)
{
    uint64_t mask = ~((1 << 12) - 1);
    size = (size + ((1 << 12) - 1)) & mask;
    return size;
}

bool AddressRegion__resizeByAddr(
    AddressRegion_t ar, enum addressRegionTypes_e type, void *index)
{
    // struct AddressRegion_Region_s * arrs = AddressRegion__getRegionByPtr(ar, index);
    // if (arrs == NULL) return false;
    // valid region to resize
    struct AddressRegion_Region_s *arrs = __getRegionByType(ar, type);
    if (arrs == NULL || arrs->type != type)
        return false;
    // already in malloced
    if (__strictInRegion(arrs, index))
        return true;
    uint64_t new_size;
    if (arrs->type == HEAP)
    {
        new_size = ((uint64_t)index) - arrs->start;
        // to much for a single malloc
        if (new_size - arrs->size > (1 << 17))
            return false;
        arrs->size = __size4kAlign(new_size);
        return true;
    }
    else if (arrs->type == STACK)
    {
        new_size = __size4kAlign(arrs->start - (uint64_t)index);
        // printf("hell %lu %p\n", arrs->start, index);
        if (new_size > ALLOW_STACK_MALLOC)
            return false;
        arrs->start -= new_size;
        arrs->size += new_size;
        return true;
    }
    return false;
}

void *AddressRegion__declareForMmap(AddressRegion_t ar, size_t size)
{
    ContinueRegion_Region_t crrt =
        ContinueRegion__requestRegion(
            ar->mapRegions, __size4kAlign(size) >> 12);

    AddressRegion_Region_t region;
    size_t id;
    region = DynamicArr__alloc(ar->regions, &id);
    // DynamicArr__add(ar->regions, &region);
    region->start = 
        (ContinueRegionRegion__getStart(crrt) << 12)  + PROCESS_MMAP_START;
    region->size = ContinueRegionRegion__getSize(crrt) << 12;
    region->crrt = crrt;
    region->type = MMAP;
    return (void *)region->start;
}

/**
 * @pre: size is aligned 
 */
void *AddressRegion__unmap(AddressRegion_t ar, void *start, size_t size)
{
    // printf("==> I have got size %u\n", size);
    struct findRegionContext_s context;
    context.vaddr = start;
    context.found = NULL;

    DynamicArr__foreach(ar->regions, __inRegionCB, &context);

    // not found the region
    if (context.found == NULL ||
        context.found->type != MMAP ||
        context.found->size != size)
    {
        if (context.found)
            __dumpRegionRegion(context.found);
        return NULL;
    }
    // printf("==> I have got here for releasing\n");

    ContinueRegion__release(ar->mapRegions, context.found->crrt);
    void *ret = (void *)context.found->start;
    DynamicArr__del(
        ar->regions,
        DynamicArr__getIndexByPtr(ar->regions, context.found));
    return ret;
}

void AddressRegion__free(AddressRegion_t ar)
{
    ContinueRegion__free(ar->mapRegions);
    DynamicArr__free(ar->regions);
    free(ar);
}

// not required
#include <stdio.h>

int main(int argc, char const *argv[])
{
    AddressRegion_t art = AddressRegion__init();
    AddressRegion__declare(art, STACK, (void *)0x500000, 0x1000);
    AddressRegion__declare(art, HEAP, (void *)0x100000, 0x1000);
    for (size_t i = 0; i < 0x1000; i += 0x100)
    {
        /* code */
        assert(
            AddressRegion__isInRegion(art, (void *)0x500000 - i - 8) == STACK);
        assert(
            AddressRegion__isInRegion(art, (void *)0x100000 + i) == HEAP);
    }

    for (size_t i = 0; i < 0x1000; i += 0x100)
    {
        /* code */
        assert(AddressRegion__isInRegion(art, (void *)0x501000 - i) == 0);
        // assert(AddressRegion__isInRegion(art,(void *) 0x499000 - i) == STACK);
        // assert(AddressRegion__isInStack(art,(void *) 0x501000 - i) == STACK);
    }

    for (size_t i = 1; i < 0x1000; i += 0x100)
    {
        /* code */
        assert(AddressRegion__isInRegion(art, (void *)0x501000 - i) == 0);
        assert(AddressRegion__isInRegion(art, (void *)0x101000 + i) == 0);
        assert(AddressRegion__isInRegion(art, (void *)0xfff000 - i) == 0);
        // assert(AddressRegion__isInStack(art,(void *) 0x501000 - i) == STACK);
    }

    // I malloc with stack
    assert(AddressRegion__resizeByAddr(art, STACK, (void *)0x4fc000) == false);
    assert(AddressRegion__resizeByAddr(art, STACK, (void *)0x4fd000) == true);
    assert(AddressRegion__resizeByAddr(art, HEAP, (void *)0x102000) == true);

    // now I have right to use these region
    assert(AddressRegion__isInRegion(art, (void *)0x4fd000) == STACK);
    assert(AddressRegion__isInRegion(art, (void *)0x102000 - 8) == HEAP);

    void *start[10];

    for (size_t i = 0; i < 10; i++)
    {

        start[i] = AddressRegion__declareForMmap(art, (i + 28) << 12);
        assert(start[i] >= (void *)PROCESS_MMAP_START);
    }

    for (size_t i = 0; i < 10; i++)
    {
        assert(
            start[i] ==
            AddressRegion__unmap(art, start[i] + (i << 5), (i + 28) << 12));
    }

    assert(AddressRegion__unmap(art, start[5], (5 + 28) << 12) == NULL);

    /* code */
    return 0;
}
