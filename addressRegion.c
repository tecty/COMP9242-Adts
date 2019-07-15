#include "addressRegion.h"
#include <string.h>
#include <stdlib.h>

// not required 
#include <assert.h>
#include <stdio.h>

struct AddressRegion_Region_s
{
    enum addressRegionTypes_e type;
    uint64_t start; 
    uint64_t size; 
};


struct AddressRegion_s
{
    struct AddressRegion_Region_s * regions;
    struct AddressRegion_Region_s * stack_region;
    uint16_t size; 
    uint16_t occupied;
};


AddressRegion_t AddressRegion__init(){
    AddressRegion_t ret = malloc(sizeof(struct AddressRegion_s));
    ret->size           = 8;
    ret->occupied       = 0;
    ret->regions = malloc(
        ret->size * sizeof(struct AddressRegion_Region_s)
    );
    // zero out the space 
    memset(
        ret->regions, 0, 
        ret->size * sizeof(struct AddressRegion_Region_s)
    );

    return ret;
}

static inline struct AddressRegion_Region_s * AddressRegion__currRegion(
    AddressRegion_t ar
){
   return & ar->regions[ar->occupied];
}

/**
 * @ret: the id to the inside managed strcuture 
 */
void AddressRegion__declare(
    AddressRegion_t ar,enum addressRegionTypes_e type,
    void * start, uint64_t size
){
    if (ar->size == ar->occupied)
    {
        // larger the array
        ar->regions = realloc(
            ar->regions, 
            ar->size * 2 * sizeof(struct AddressRegion_Region_s)
        );
        memset(
            & ar->regions[ar->size], 0,
            ar->size* sizeof(struct AddressRegion_Region_s) 
        );
        ar->size *= 2;
    }
    AddressRegion__currRegion(ar)->size  = size;
    AddressRegion__currRegion(ar)->type  = type;
    if (type == STACK)
    {
        // stack is grow downwards
        AddressRegion__currRegion(ar)->start = ((uint64_t)start) - size;
    } else {
        AddressRegion__currRegion(ar)->start = ((uint64_t)start);
    }
    
    if (type == STACK)
    {
        ar->stack_region = AddressRegion__currRegion(ar);
    }
    
    ar->occupied ++;
}

static inline bool AddressRegion__strictInRegion(
    struct AddressRegion_Region_s * arrs, void * index 
){
    return (
        arrs->start <= (uint64_t) index &&
        arrs->start + arrs->size >= (uint64_t) index
    );
}

enum addressRegionTypes_e AddressRegion__isInRegion(
    AddressRegion_t ar, void * index
){
    struct AddressRegion_Region_s * the_region; 
    // return null to indecate this item is delted 
    for (size_t i = 0; i < ar->occupied; i++)
    {
        the_region = & ar->regions[i];
        // this is how it's in the region
        if(AddressRegion__strictInRegion(the_region, index)){
            return the_region->type;
        }
    }
    // not in the region
    return 0;
}


static struct AddressRegion_Region_s * AddressRegion__getRegionByType(
    AddressRegion_t ar, enum addressRegionTypes_e type
){
       struct AddressRegion_Region_s * the_region; 
    // return null to indecate this item is delted 
    for (size_t i = 0; i < ar->occupied; i++)
    {
        the_region = & ar->regions[i];
        // this is how it's in the region
        if(the_region->type == type){
            return the_region;
        }

    }
    // not in the region
    return NULL;
}



/* This might be use by mmap */
void AddressRegion__regionAddSize(
    AddressRegion_t ar, enum addressRegionTypes_e type, size_t add_size
){
    // return null to indecate this item is delted 
    struct AddressRegion_Region_s * the_region; 
    // return null to indecate this item is delted 
    for (size_t i = 0; i < ar->occupied; i++)
    {
        the_region = & ar->regions[i];
        // found the region
        if(the_region->type == type) break;
    }
    // the region doesn't found 
    assert(the_region->type == type);
    if (type == STACK)
    {
        the_region->start -= add_size;
    }
    the_region->size += add_size;
}

static inline uint64_t AddressRegion__size4kAlign(uint64_t size){
    uint64_t mask = ~((1<<12) - 1);
    size = (size + ((1<<12) - 1)) & mask;
    return size;
}

bool AddressRegion__resizeByAddr(
    AddressRegion_t ar, enum addressRegionTypes_e type, void * index
){
    // struct AddressRegion_Region_s * arrs = AddressRegion__getRegionByPtr(ar, index);
    // if (arrs == NULL) return false;
    // valid region to resize 
    struct AddressRegion_Region_s * arrs = AddressRegion__getRegionByType(ar, type);
    if ( arrs == NULL || arrs->type != type ) return false;
    // already in malloced 
    if (AddressRegion__strictInRegion(arrs, index)) return true;
    uint64_t new_size;
    if (arrs->type == HEAP){
        new_size = ((uint64_t) index) - arrs->start;
        // to much for a single malloc 
        if(new_size > (1 << 17)) return false;
        arrs->size = AddressRegion__size4kAlign(new_size);
        return true;
    } else if (arrs->type == STACK){
        new_size = AddressRegion__size4kAlign(arrs->start - (uint64_t) index);
        // printf("hell %lu %p\n", arrs->start, index);
        if (new_size > ALLOW_STACK_MALLOC) return false;
        arrs->start -= new_size;
        arrs->size  += new_size;
        return true;
    }
    return false;
}


void AddressRegion__free(AddressRegion_t ar){
    free(ar->regions);
    free(ar);
}


// int main(int argc, char const *argv[])
// {
//     AddressRegion_t art = AddressRegion__init();
//     AddressRegion__declare(art, STACK, (void *) 0x500000, 0x1000);
//     AddressRegion__declare(art, HEAP, (void *) 0x100000, 0x1000);
//     for (size_t i = 0; i < 0x1000; i+= 0x100)
//     {
//         /* code */
//         assert(
//             AddressRegion__isInRegion(art,(void *) 0x500000 - i) == STACK
//         );
//         assert(
//             AddressRegion__isInRegion(art,(void *) 0x100000 + i) == HEAP
//         );
//     }
    
//     for (size_t i = 0; i < 0x1000; i+= 0x100)
//     {
//         /* code */
//         assert(AddressRegion__isInRegion(art,(void *) 0x501000 - i) == 0);
//         // assert(AddressRegion__isInRegion(art,(void *) 0x499000 - i) == STACK);
//         // assert(AddressRegion__isInStack(art,(void *) 0x501000 - i) == STACK);
//     }


//     for (size_t i = 1; i < 0x1000; i+= 0x100)
//     {
//         /* code */
//         assert(AddressRegion__isInRegion(art,(void *) 0x501000 - i) == 0);
//         assert(AddressRegion__isInRegion(art,(void *) 0x101000 + i) == 0);
//         assert(AddressRegion__isInRegion(art,(void *) 0xfff000 - i) == 0);
//         // assert(AddressRegion__isInStack(art,(void *) 0x501000 - i) == STACK);
//     }

//     // I malloc with stack 
//     assert(AddressRegion__resizeByAddr(art, STACK, (void *) 0x4fc000)==false);
//     assert(AddressRegion__resizeByAddr(art, STACK, (void *) 0x4fd000)==true);
//     assert(AddressRegion__resizeByAddr(art, HEAP , (void *) 0x102000)==true);

//     // now I have right to use these region 
//     assert(AddressRegion__isInRegion(art, (void *) 0x4fd000)== STACK);
//     assert(AddressRegion__isInRegion(art, (void *) 0x102000)== HEAP);

//     /* code */
//     return 0;
// }
