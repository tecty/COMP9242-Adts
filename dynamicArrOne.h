
#if !defined(DYNAMIC_ARR_ONE_H)
#define DYNAMIC_ARR_ONE_H

#include "dynamic.h"

typedef DynamicArr_t DynamicArrOne_t ;

DynamicArrOne_t DynamicArrOne__init(size_t item_size);
void * DynamicArrOne__alloc(DynamicArrOne_t da, size_t * id);
size_t DynamicArrOne__add(DynamicArrOne_t da,void * data);
void * DynamicArrOne__get(DynamicArrOne_t da, size_t index);
void DynamicArrOne__del(DynamicArrOne_t da, size_t index);
void DynamicArrOne__free(DynamicArrOne_t da);

#endif // DYNAMIC_ARR_ONE_H