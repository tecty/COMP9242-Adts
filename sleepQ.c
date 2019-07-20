#include "sleepQ.h"

// private include 
#include "dynamicQ.h"

// Only need for main 
#include <stdio.h>

static struct
{
    DynamicQ_t sleeping;
} SleepQ_s;


void SleepQ__init(){
    SleepQ_s.sleeping = DynamicQ__init(sizeof(coro));
}

int SleepQ__tryWake(){
    if (! DynamicQ__isEmpty(SleepQ_s.sleeping)){
        // there's waiting thread 
        coro target = *(coro *) DynamicQ__first(SleepQ_s.sleeping);
        // printf("try to wake up target %p\n",target);
        // wake the thead and dequeue it 
        resume(target, NULL);
        DynamicQ__deQueue(SleepQ_s.sleeping);
        return 1;
    }
    return 0;
}

/**
 * @pre: the running is a coroutine thread
 */
void SleepQ__sleep(){
    // printf("the current thread is %p\n", get_running());
    coro curr = get_running();
    DynamicQ__enQueue(SleepQ_s.sleeping, &curr);
    yield(NULL);
}


/**
 * Main: mute while imported 
 */
void* child(void * args){
    printf("im child\n");
    void * receive= yield(NULL);
    printf("Child again: I have received %lu\n", (__uint64_t) receive);
    printf("I decide to die now \n");
    for (size_t i = 0; i < 5; i++)
    {
        SleepQ__sleep();
        printf("Child: I have waked up\n");
    }
    
    return (void *) 45678;
}

int main(int argc, char const *argv[])
{
    // init for the sleep queue 
    SleepQ__init();
    coro child_co= coroutine(child, (1 << 11));
    void * child_msg = resume(child_co, (void *) 12345);
    printf("I have wake up thread %p\n", child_co);
    printf("child is resumeable? %d\n", resumable(child_co));
    printf("go to child\n");
    resume(child_co, (void *) 12345);
    for (size_t i = 0; i < 10; i++)
    {
        printf("I want to wake up threads %u\n", i);
        if(!SleepQ__tryWake()) break;
    }
    printf("end early\n");

    return 0;
}
