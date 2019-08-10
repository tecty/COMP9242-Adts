#if !defined(ADDRESS_SPACE_H)
#define ADDRESS_SPACE_H

#include "addressRegion.h"
#include "dynamicQ.h"
#include <stdlib.h>

#define PAGE_SLOT (512)

typedef struct addressSpace_s* addressSpace_t;

addressSpace_t AddressSpace__init();
void AddressSpace__mapVaddr(addressSpace_t ast, void* paddr, void* vaddr);
void* AddressSpace__getPaddrByVaddr(addressSpace_t ast, void* vaddr);
bool AddressSpace__isInAdddrSpace(addressSpace_t ast, void* vaddr);
void AddressSpace__free(addressSpace_t ast);

void* AddressSpace__mmap(addressSpace_t ast, size_t size);
bool AddressSpace__unmap(
    addressSpace_t ast, void* start, size_t size, DynamicArr_t needFreeArr);
DynamicQ_t AddressSpace__delPageHelper(
    DynamicQ_t pageList, DynamicArr_t needFreeArr);

#endif // ADDRESS_SPACE_H