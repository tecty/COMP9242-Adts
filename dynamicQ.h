#if !defined(DYNAMIC_Q)
#define DYNAMIC_Q

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct DynamicQ_s* DynamicQ_t;
typedef void (*dynamicQ_callback_t)(void* data);
typedef bool (*dynamicQ_compare_t)(void* data, void* private_data);

DynamicQ_t DynamicQ__init(uint64_t item_size);
void DynamicQ__enQueue(DynamicQ_t dq, void* data);
bool DynamicQ__isEmpty(DynamicQ_t dq);
void* DynamicQ__first(DynamicQ_t dq);
void DynamicQ__deQueue(DynamicQ_t dq);
size_t DynamicQ__getAlloced(DynamicQ_t dq);
void DynamicQ__free(DynamicQ_t dq);
void DynamicQ__foreach(DynamicQ_t dq, dynamicQ_callback_t callback);
DynamicQ_t DynamicQ__dup(DynamicQ_t dq);
DynamicQ_t DynamicQ__filter(
    DynamicQ_t dq, dynamicQ_compare_t cmp, void* private_data);

#endif // DYNAMIC_Q