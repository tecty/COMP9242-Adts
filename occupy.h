#if !defined(OCCUPY_H)
#define OCCUPY_H

/**
 * An dynamic bit array, you can "trivialy" set a bit,
 * The bit is belong to a virtually big array; 
 */

typedef struct Occupy_s * Occupy_t; 

Occupy_t Occupy__init();
void Occupy__free(Occupy_t oct);
unsigned long long Occupy__alloc(Occupy_t oct);
void Occupy__del(Occupy_t oct, unsigned long long pos);
int Occupy__get(Occupy_t oct, unsigned long long pos);
int Occupy__set(Occupy_t oct, unsigned long long pos, int val);

#endif // OCCUPY_H
