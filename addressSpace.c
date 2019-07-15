#include "addressSpace.h"

// this can be imported
#include <assert.h>
#include <time.h>
#include <stdio.h>
#define BIT(n) (1ul << (n))
#define MASK(n) (BIT(n)-1ul)

struct addressSpace_s
{
    uint64_t * rootPageTable;
    DynamicQ_t pageList;
    AddressRegion_t regions;
};


/* Private */
uint64_t * AddressSpace__allocPageTable(addressSpace_t ast){
    // TODO: this map be redifine by frametable 
    uint64_t * ret =  malloc(BIT(12));
    memset(ret, 0, BIT(12));
    // we need the pointer to be recorded 
    DynamicQ__enQueue(ast->pageList , &ret);
    return ret;
}

addressSpace_t AddressSpace__init(){
    addressSpace_t ret = malloc(sizeof(struct addressSpace_s));
    // this need to change the size
    ret->pageList = DynamicQ__init(sizeof(void *));
    ret->rootPageTable = AddressSpace__allocPageTable(ret);
    // init region 
    ret->regions = AddressRegion__init();
    return ret;
}

// private
static inline uint16_t AddressSpace__shiftBits(uint16_t depth){
    return (12 + 9 * depth);
}

static inline uint16_t AddressSpace__getIndexByVaddr(void * vaddr, uint16_t depth){
    depth = 3 - depth;
    // calculate the mask
    uint64_t mask = MASK(9) << AddressSpace__shiftBits(depth);
    // calculate this index 
    uint64_t this_index = (uint64_t) vaddr;
    this_index &= mask;
    this_index >>= AddressSpace__shiftBits(depth);
    return this_index;
}

void AddressSpace__mapVaddr(
    addressSpace_t ast, void * paddr, void * vaddr 
){
    uint64_t this_index;
    uint64_t * this_pageTable =  ast->rootPageTable;
    printf("Insert: got vaddr %lu\n", (uint64_t) vaddr);

    for (size_t i = 0; i <= 3; i++)
    {
        // calculate the mask
        this_index = AddressSpace__getIndexByVaddr(vaddr, i);
        printf("Insert: got Index %lu\n", this_index);

        assert(this_index < PAGE_SLOT);       
        if (i ==3)
        {
            // instead of mapping a page table, map a paddr 
            this_pageTable[this_index] = (uint64_t) paddr;
            return;
        }
        if (this_pageTable[this_index] == 0){
            this_pageTable[this_index] = (uint64_t) AddressSpace__allocPageTable(ast);
        }
        // go deep one level 
        this_pageTable = (uint64_t *) this_pageTable[this_index];
    }
}

void *AddressSpace__getPaddrByVaddr(addressSpace_t ast, void * vaddr){
    uint64_t this_index;
    uint64_t * this_pageTable =  ast->rootPageTable;
    printf("Select: got vaddr %lu\n", (uint64_t) vaddr);


    for (size_t i = 0; i <= 3; i++)
    {
        this_index = AddressSpace__getIndexByVaddr(vaddr, i);
        // printf("Select: got Index %lu\n", this_index);

        assert(this_index < PAGE_SLOT);       
        if (i ==3)
        {
            // instead of mapping a page table, map a paddr 
            // TODO: type change
            return (void *) this_pageTable[this_index];
        }
        if (this_pageTable[this_index] == 0){
            this_pageTable[this_index] = 
                (uint64_t) AddressSpace__allocPageTable(ast);
        }
        // go deep one level 
        this_pageTable = (uint64_t *) this_pageTable[this_index];
    }
    return 0;
}

bool AddressSpace__isInAdddrSpace(addressSpace_t ast, void* vaddr){
    return AddressRegion__isInRegion(ast->regions, vaddr) > 0;
}

void AddressSpace__free(addressSpace_t ast){
    uint64_t * page_addr = 0;
    void * queue_first = NULL;

    /* free all the data in the queue */
    while( 
        (queue_first = DynamicQ__first(ast->pageList)) &&
        queue_first != NULL
    ){
        page_addr = * (void **) queue_first;

        // TODO: change to free frame
        free(page_addr);
        DynamicQ__deQueue(ast->pageList);
    }

    DynamicQ__free(ast->pageList);
    AddressRegion__free(ast->regions);
    free(ast);
}

int main(int argc, char const *argv[])
{
    srand(time(NULL));
    uint64_t vaddrs[10];
    uint64_t paddrs[10];
    
    addressSpace_t ast = AddressSpace__init();
    
    
    for (size_t i = 0; i < 10; i++)
    {
        vaddrs[i]= rand();
        paddrs[i]= rand();
        AddressSpace__mapVaddr(ast, (void *)paddrs[i], (void *)vaddrs[i]);
        printf("V: %lu \tP: %lu \n", vaddrs[i], paddrs[i]);
    }

    for (size_t i = 0; i < 10; i++)
    {
        printf("V: %lu \tP: %lu \tGot: %lu\n",
            vaddrs[i], paddrs[i],
            (uint64_t) AddressSpace__getPaddrByVaddr(ast, (void *)vaddrs[i])
        );
        
        assert (
            (uint64_t) AddressSpace__getPaddrByVaddr(ast, (void *)vaddrs[i]) ==
                paddrs[i]
        );
    }
    AddressSpace__mapVaddr(ast, (void *) 12345,  (void *) 0x8ffff000);

    return 0;
}
