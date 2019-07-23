#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(DYNAMIC_H)
#define DYNAMIC_H
typedef void (* dynamicArr_callback_t)(void * each, void * data);

typedef struct DynamicArr_s * DynamicArr_t ;
DynamicArr_t DynamicArr__init(size_t item_size);
void * DynamicArr__alloc(DynamicArr_t da, size_t * id);
size_t DynamicArr__add(DynamicArr_t da,void * data);
void * DynamicArr__get(DynamicArr_t da, size_t index);
void DynamicArr__del(DynamicArr_t da, size_t index);
void DynamicArr__free(DynamicArr_t da);
size_t DynamicArr__getAlloced(DynamicArr_t da);
void DynamicArr__foreach(
    DynamicArr_t da, dynamicArr_callback_t cb, void * privateData
);
#endif // DYNAMIC_H
