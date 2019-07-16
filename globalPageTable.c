#include <stdint.h>


// for the null page
#define NULL_PAGE ((uint32_t)0)

typedef struct Page_s {
    uint32_t prevPage;
    uint32_t nextPage;
    uint32_t cap;
    uint32_t frame:20;
    uint32_t copyOnWrite:1;
}* Page_t;

