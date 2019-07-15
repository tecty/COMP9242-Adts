#include <setjmp.h>
#include <stdio.h>

jmp_buf mainTask, childTask, en;

void call_with_cushion(void);
void child(void);


void end_coroutine(){
    char stack[0x1000];
    stack[0x1000 -1] = 1;
    
}


int main(void) {
    if (!setjmp(mainTask)) {
        call_with_cushion(); /* child never returns */ /* yield */
    } /* execution resumes after this "}" after first time that child yields */
    for (;;) {
        printf("Parent\n");
        if (!setjmp(mainTask)) {
            /* yield - note that this is undefined under C99 */
            longjmp(childTask, 1); 
        }
    }
}

void call_with_cushion (void) {
    char space[0x1000]; /* Reserve enough space for main to run */
    space[0x1000 - 1] = 1; /* Do not optimize array out of existence */
    child();
}

void child (void) {
    printf("Child loop begin\n");
    if (!setjmp(childTask)) longjmp(mainTask, 1); /* yield - invalidates childTask in C99 */

    printf("Child loop end\n");
    if (!setjmp(childTask)) longjmp(mainTask, 1); /* yield - invalidates childTask in C99 */
    /* Don't return. Instead we should set a flag to indicate that main()
       should stop yielding to us and then longjmp(mainTask, 1) */
}