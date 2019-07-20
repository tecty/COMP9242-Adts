
// private 
#include <string.h>
#include <stdlib.h>

typedef struct Occupy_s * Occupy_t; 

struct Occupy_s
{
    long long int * occupy;
    long long int * data;
    unsigned long long occupied;
    unsigned long long size;
    size_t tail;
};
#define SLOT_SIZE (sizeof(long long int) * 8)


Occupy_t Occupy__init(){
    Occupy_t ret   = malloc(sizeof(struct Occupy_s));
    ret->occupy    = malloc(SLOT_SIZE);
    ret->data      = malloc(SLOT_SIZE);

    ret->occupied  = 0;
    ret->size      = SLOT_SIZE;
    ret->occupy[0] = ~0;
    ret->data[0]   = 0;
    return ret;
}

void Occupy__free(Occupy_t oct){
    free(oct->occupy);
    free(oct->data);
    free(oct);
} 

static inline void __Occupy__resize(Occupy_t oct){
    size_t newSize = oct->size * 2;
    oct->occupy    = realloc(oct->occupy, newSize);
    oct->data      = realloc(oct->data, newSize);

    for (size_t i = oct->size / SLOT_SIZE; i < newSize / SLOT_SIZE; i++)
    {
        oct->data[i]   =  0;
        oct->occupy[i] = ~0;
    }

    oct->size = newSize;
}

unsigned long long Occupy__alloc(Occupy_t oct){
    if(oct->occupied == oct->size -2)
    {
        __Occupy__resize(oct);
    }

    for (size_t i = 0; i < oct->size/SLOT_SIZE; i++)
    {
        size_t slot = (i * SLOT_SIZE + oct->tail) % oct->size / SLOT_SIZE;
        size_t empty = ffsll(oct->occupy[slot]);

        if (empty != 0) {
            oct->tail = empty + slot * SLOT_SIZE;
            oct->occupy[slot] &= ~(1UL << (empty -1));

            oct->occupied ++;
            return oct->tail;    
        }
    }
    return 0;
}

void Occupy__del(Occupy_t oct, unsigned long long pos){
    size_t slot = pos / SLOT_SIZE;
    size_t bit  = pos % SLOT_SIZE;
    oct->occupied --;
    oct->occupy[slot] |= (1UL << (bit - 1));
}

int Occupy__get(Occupy_t oct, unsigned long long pos){
    if (pos > oct->size ) return -1;
    size_t slot = pos / SLOT_SIZE;
    size_t bit  = pos % SLOT_SIZE;
    return (oct->data[slot] >> (bit - 1)) & 1U;
}

int Occupy__set(Occupy_t oct, unsigned long long pos, int val){
    if (pos > oct->size ) return -1;
    size_t slot = pos / SLOT_SIZE;
    size_t bit  = pos % SLOT_SIZE;
    // the bit is not allocated
    if ((oct->occupy[slot] >> (bit - 1)) & 1U) return - 2;
    if (val > 0)
    {
        // SET: true
        oct->data[slot] |= (1UL << (bit - 1));
        return 1;
    } else {
        // SET: false
        oct->data[slot] &= ~(1UL << (bit - 1));
        return 0;
    }
}

#include <stdio.h>
#include <assert.h>


int main(int argc, char const *argv[])
{
    /* code */
    // printf("I have got the bit is zero %d\n", ffsll(~0));
    // printf("I have got the bit is zero %d\n", ffsll(1<< 2));
    // oct test 
    Occupy_t oct = Occupy__init();
    size_t id = Occupy__alloc(oct);
    assert(id != 0);
    assert(Occupy__get(oct, id)== 0);
    assert(Occupy__set(oct, id, 1) == 1);
    assert(Occupy__get(oct, id)== 1);

    
    size_t ids[1 << 12];
    for (size_t i = 0; i < 1 << 12; i++)
    {
        ids[i] = Occupy__alloc(oct);
        Occupy__set(oct, ids[i] , i % 2);
    }
    
    for (size_t i = 0; i < 1 << 12; i++)
    {
        assert(Occupy__get(oct, ids[i] ) == (int) i % 2);
    }

    return 0;
}
