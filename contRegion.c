#include "contRegion.h"
#include <stdio.h>

#define DEFAULT_SIZE 4
// #define DEFAULT_SIZE 16

struct ContinueRegion_s
{
    bool *occupied;
    uint64_t size;
    uint64_t allocated;
    uint64_t last_index;
};

/**
 * A small read-only tuple
 */
struct ContinueRegion_Region_s
{
    uint64_t start;
    uint64_t size;
};

uint64_t ContinueRegionRegion__getStart(ContinueRegion_Region_t crrt)
{
    return crrt->start;
}

uint64_t ContinueRegionRegion__getSize(ContinueRegion_Region_t crrt)
{
    return crrt->size;
}

static inline void ContinueRegion__modIndex(
    ContinueRegion_t cr, uint64_t *index)
{
    *index = (*index) % cr->size;
}

static inline void __incIndex(
    ContinueRegion_t cr, uint64_t *index)
{
    (*index)++;
    ContinueRegion__modIndex(cr, index);
}

// only can be larger
void __resize(ContinueRegion_t cr, uint64_t new_size)
{
    if (new_size > cr->size)
    {
        cr->occupied = realloc(cr->occupied, sizeof(bool) * new_size);
        for (size_t i = cr->size; i < new_size; i++)
        {
            // zero out the new space
            cr->occupied[i] = false;
        }
        printf("I have resize from %lu to %lu allocated %lu\n", cr->size, new_size, cr->allocated);
        cr->size = new_size;
    }
}

void __alloc(ContinueRegion_t cr, uint64_t start, uint64_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        assert(i + start != cr->size);
        assert(cr->occupied[i + start] == false);
        cr->occupied[i + start] = true;
    }
    cr->allocated += size;
}

ContinueRegion_t ContinueRegion__init()
{
    ContinueRegion_t cr = malloc(sizeof(struct ContinueRegion_s));
    cr->size = DEFAULT_SIZE;
    cr->allocated = 0;
    cr->last_index = 0;
    cr->occupied = malloc(sizeof(bool) * cr->size);
    for (size_t i = 0; i < cr->size; i++)
    {
        // zero out the new space
        cr->occupied[i] = false;
    }

    return cr;
}

void ContinueRegion__free(ContinueRegion_t cr)
{
    free(cr->occupied);
    free(cr);
}

void ContinueRegion__release(
    ContinueRegion_t cr, ContinueRegion_Region_t crrt)
{

    for (size_t i = 0; i < crrt->size; i++)
    {
        assert(cr->occupied[i + crrt->start] == true);
        cr->occupied[i + crrt->start] = false;
    }
    cr->allocated -= crrt->size;
    printf("I have release %lu\n", cr->allocated);

    // free the struct
    free(crrt);
}

ContinueRegion_Region_t ContinueRegion__requestRegion(
    ContinueRegion_t cr, uint64_t size)
{
    if (size + cr->allocated > cr->size)
    {
        __resize(cr, (size + cr->size) * 2);
    }

    size_t this_start = cr->last_index;
    size_t clean = 0;
    for (size_t touched = 0; touched < cr->size; touched++)
    {
        if (clean == size)
        {
            // found the region
            ContinueRegion_Region_t crrt =
                malloc(sizeof(struct ContinueRegion_Region_s));

            crrt->start = this_start;
            crrt->size = size;

            __alloc(cr, this_start, size);
            cr->last_index = this_start + size;
            return crrt;
        }

        if (
            // over flow prevention
            this_start + clean == cr->size ||
            // occupied
            cr->occupied[this_start + clean] == true)
        {
            // this is know not clean
            // printf("I got here \n");
            this_start = (this_start + clean + 1) % cr->size;
            clean = 0;
            continue;
        }
        clean++;
    }
    __resize(cr, (size + cr->size) * 2);
    return ContinueRegion__requestRegion(cr, size);
}

// int main(int argc, char const *argv[])
// {
//     ContinueRegion_t cr =  ContinueRegion__init();

//     ContinueRegion_Region_t allocated[10];
//     for (size_t i = 0; i < 10; i++){
//         allocated[0] = ContinueRegion__requestRegion(cr, i);

//         for (size_t j = 0; j < i; j++)
//         {
//             assert(
//                 cr->occupied[
//                     ContinueRegionRegion__getStart(
//                         allocated[0]
//                     )+ j
//                 ] == true
//             );
//         }
//         ContinueRegion__release(cr, allocated[0]);
//         // printf("I have alloced %lu \n",i);
//     }

//     for (size_t j = 0; j < cr->size; j++)
//     {
//         assert(cr->occupied[j] == false);
//     }

//     // return 0;
//     for (size_t i = 0; i < 10; i++)
//     {
//         allocated[i] = ContinueRegion__requestRegion(cr, 20);
//         for (size_t j = 0; j < 20; j++)
//         {
//             assert(
//                 cr->occupied[
//                     ContinueRegionRegion__getStart(
//                         allocated[i]
//                     )+ j
//                 ] == true
//             );
//         }
//     }

//     for (size_t i = 0; i < 10; i++)
//     {
//         ContinueRegion__release(cr, allocated[i]);
//     }

//     for (size_t j = 0; j < cr->size; j++)
//     {
//         assert(cr->occupied[j] == false);
//     }
//     return 0;
// }