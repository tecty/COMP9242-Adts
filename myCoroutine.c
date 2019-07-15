#include "sleepQ.h"


void* child(void * args){
    printf("im child\n");
    void * receive= yield(NULL);
    printf("Child again: I have received %lu\n", (__uint64_t) receive);
    printf("I decide to die now \n");
    for (size_t i = 0; i < 5; i++)
    {
        
    }
    
    return (void *) 45678;
}

int main(int argc, char const *argv[])
{
    coro child_co= coroutine(child, (1 << 11));
    void * child_msg = resume(child_co, (void *) 12345);
    printf("im main herer\n");
    printf("child is resumeable? %d\n", resumable(child_co));
    printf("go to child\n");
    child_msg = resume(child_co, (void *) 12345);
    printf("child send me %p and maybe died\n", child_msg);

    return 0;
}
