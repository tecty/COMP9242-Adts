#if !defined(DOUBLE_LINK_LIST_H)
#define DOUBLE_LINK_LIST_H


#include "dynamicArrOne.h"
#include <stdint.h>
typedef DynamicArrOne_t DoubleLinkList_t;
typedef uint64_t (* doubleLinkList_callback_t)(uint64_t data);
#define NULL_SLOT ((uint32_t)0)


DoubleLinkList_t DoubleLinkList__init();
void DoubleLinkList__free(DoubleLinkList_t dll);

uint32_t DoubleLinkList__add(DoubleLinkList_t dll, uint64_t data);
uint32_t DoubleLinkList__link(
    DoubleLinkList_t dll, uint32_t first, uint32_t second
);
uint32_t DoubleLinkList__del(DoubleLinkList_t dll, uint32_t index);
uint64_t * DoubleLinkList__getDataPtr(DoubleLinkList_t dll, uint32_t index);
uint64_t DoubleLinkList__get(DoubleLinkList_t dll, uint32_t index);
uint64_t DoubleLinkList__update(
    DoubleLinkList_t dll, uint32_t index, uint64_t data
);
uint32_t DoubleLinkList__getPrev(DoubleLinkList_t dll, uint32_t index);
uint32_t DoubleLinkList__getNext(DoubleLinkList_t dll, uint32_t index);
uint32_t DoubleLinkList__getRoot(DoubleLinkList_t dll, uint32_t index);
void DoubleLinkList__foreach(
    DoubleLinkList_t dll, uint32_t start, doubleLinkList_callback_t cb
);


#endif // DOUBLE_LINK_LIST_H
