/**
 * Because this is mainly used in the page table, we want it
 * to not inlcude the 0 id
 */

#include "doubleLinkList.h"
// not required 
#include <stdio.h>

typedef DynamicArrOne_t DoubleLinkList_t;

#define NULL_SLOT ((uint32_t)0)

// this can be packed when you imported 
typedef struct Node_s
{
    uint32_t prev;
    uint32_t next;
    uint64_t data;
} * Node_t;

// void dump_node(Node_t node){
//     printf("prev:%u\tnext:%u\tdata:%lu\n", node->prev, node->next, node->data);
// }

DoubleLinkList_t DoubleLinkList__init(){
    return DynamicArrOne__init(sizeof(struct Node_s));
}
void DoubleLinkList__free(DoubleLinkList_t dll){
    DynamicArrOne__free(dll);
}

uint32_t DoubleLinkList__add(DoubleLinkList_t dll, uint64_t data){
    struct Node_s curr = {
        NULL_SLOT, NULL_SLOT, data
    };
    return (uint32_t) DynamicArrOne__add(dll, &curr);
}

/**
 * if success, return the second's id
 * We make sure second must be at the end of the list of first.
 */
uint32_t DoubleLinkList__link(
    DoubleLinkList_t dll, uint32_t first, uint32_t second
){
    // couldn't presive in these situation
    if (first == NULL_SLOT || second == NULL_SLOT) return 0;
    // already in the list
    if (first == second ) return second;
    
    Node_t first_node = DynamicArrOne__get(dll, first);
    // the second node is already in the list 
    if (first_node == NULL) return 0;
    Node_t second_node = DynamicArrOne__get(dll, second);
    if (second_node == NULL) return 0;
    size_t third = first_node ->next;
    Node_t third_node = DynamicArrOne__get(dll, first_node->next);

    first_node  -> next = second;
    second_node -> next = third;
    if (third_node) third_node -> prev = second;
    second_node->prev = first;
    return second;
}

uint32_t DoubleLinkList__delink(DoubleLinkList_t dll, uint32_t index){
    Node_t curr = DynamicArrOne__get(dll, index);
    // nothing I can do 
    if (curr == NULL) return NULL_SLOT;

    Node_t prev_node = DynamicArrOne__get(dll, curr->prev);    
    Node_t next_node = DynamicArrOne__get(dll, curr->next);
    uint32_t ret = NULL_SLOT; 
    if (next_node) {
        next_node->prev = curr->prev;
        // printf("next:\t");
        // dump_node(next_node);
        ret = curr->next;
    }
    if (prev_node) {
        prev_node->next = curr->next;
        // printf("prev:\t");
        // dump_node(prev_node);
        ret = curr->prev;
    }

    // delink in the list 
    curr->next = NULL_SLOT;
    curr->prev = NULL_SLOT;

    return ret;
}


/**
 * @ret: the candidate root 
 * if it deleted a root node, I will give back some track to let
 * to find this link
 */
uint32_t DoubleLinkList__del(DoubleLinkList_t dll, uint32_t index){
    Node_t curr = DynamicArrOne__get(dll, index);
    // nothing I can do 
    if (curr == NULL) return NULL_SLOT;
    uint32_t ret = DoubleLinkList__delink(dll, index);
    // this node can be savely deleted 
    DynamicArrOne__del(dll, index);
    return ret;
}

/**
 * get the data pointer 
 */
uint64_t * DoubleLinkList__getDataPtr(DoubleLinkList_t dll, uint32_t index){
    Node_t curr = DynamicArrOne__get(dll, index);
    if (curr == NULL) return NULL;
    return &(curr->data);
}


/**
 * Trivial read function that read the data save in the slot 
 */
uint64_t DoubleLinkList__get(DoubleLinkList_t dll, uint32_t index){
    Node_t curr = DynamicArrOne__get(dll, index);
    if (curr == NULL) return 0;
    return curr->data;
}

/**
 * Update the slot with new data 
 */
uint64_t DoubleLinkList__update(
    DoubleLinkList_t dll, uint32_t index, uint64_t data
){
    Node_t curr = DynamicArrOne__get(dll, index);
    if (curr == NULL) return 0;
    // printf("try to update \n");
    return curr->data = data;
}

uint32_t DoubleLinkList__getPrev(
    DoubleLinkList_t dll, uint32_t index
) {
    Node_t curr = DynamicArrOne__get(dll, index);
    if (curr == NULL) return 0;
    return curr->prev;
}

uint32_t DoubleLinkList__getNext(
    DoubleLinkList_t dll, uint32_t index
) {
    Node_t curr = DynamicArrOne__get(dll, index);
    if (curr == NULL) return 0;
    return curr->next;
}

uint32_t DoubleLinkList__getRoot(
    DoubleLinkList_t dll, uint32_t index
) {
    Node_t curr = DynamicArrOne__get(dll, index);
    if (curr == NULL) return 0;
    uint32_t curr_index, prev_index;
    curr_index = prev_index = index;

    while (prev_index!= NULL_SLOT){
        curr_index = prev_index;
        prev_index = DoubleLinkList__getPrev(dll, curr_index);    
    }
    return curr_index;
}

void DoubleLinkList__foreach(
    DoubleLinkList_t dll, uint32_t start, doubleLinkList_callback_t cb,
    void* private
){
    uint32_t curr_index = start;
    Node_t curr =  DynamicArrOne__get(dll, start);
    while (curr != NULL) {
        // dump_node(curr);
        // printf("before %lu, after %lu\n", curr->data, cb(curr->data));
        curr->data = cb(curr->data, private);
        curr = DynamicArrOne__get(dll, curr->next);
    }
}
typedef struct dumpContext_s
{
    void * privateData;
    doubleLinkList_callback_t privateCb;
}* dumpContext_t;


void __dumpCB(void * data, void * contextPtr){
    Node_t node = data;
    dumpContext_t context = contextPtr;
    context->privateCb(node->data, context->privateData);
}

void DoubleLinkList__dumpEach(
    DoubleLinkList_t dll, doubleLinkList_callback_t cb, void* private
){
    struct dumpContext_s context;
    context.privateCb = cb;
    context.privateData = private;
    DynamicArrOne__foreach(dll, __dumpCB, & context);
}

// #include <assert.h>

// uint64_t times2(uint64_t in, void * unused){
//     return in * 2;
// }

// int main(int argc, char const *argv[])
// {
//     DoubleLinkList_t dll = DoubleLinkList__init();   
//     uint64_t ids[10];
//     uint64_t newIds[10];
//     for (size_t i = 0; i < 10; i++)
//     {
//         ids[i] = DoubleLinkList__add(dll,i *2 );
//         assert(DoubleLinkList__get(dll, ids[i]) == i * 2 );
//         if (i != 0) {
//             assert(
//                 DoubleLinkList__link(dll, ids[i-1], ids[i]) == ids[i]
//             );
//         }
//     }
    
//     for (size_t i = 0; i < 10; i++)
//     {
//         newIds[i] = DoubleLinkList__add(dll, i);
//     }

//     /**
//      * Linking Tests
//      */    
//     // for (size_t i = 0; i < 10; i++)
//     // {
//     //     size_t nextNext = DoubleLinkList__getNext(dll, ids[i]);
//     //     assert(DoubleLinkList__link(dll, ids[i], newIds[i]) == newIds[i]);
//     //     if (newIds[i] != DoubleLinkList__getNext(dll, ids[i]))
//     //     {
//     //         printf("I expect next %u ",newIds[i]);
//     //         printf("But I got %u\n",  DoubleLinkList__getNext(dll, ids[i]));
//     //     }
        
//     //     assert(newIds[i] == DoubleLinkList__getNext(dll, ids[i]));
//     //     assert(nextNext  == DoubleLinkList__getNext(dll, newIds[i]));
//     //     assert(ids[i]    == DoubleLinkList__getPrev(dll, newIds[i]));
//     //     if (nextNext != 0)
//     //     {
//     //         assert(newIds[i] == DoubleLinkList__getPrev(dll, nextNext));
//     //     }
        
//     // }


//     // deletion test 
//     for (size_t i = 0; i < 5; i++)
//     {
//         DoubleLinkList__del(dll,ids[i * 2]);
//         // deleted node couldn't be referenced anymore 
//         assert(DoubleLinkList__get(dll, ids[i * 2 ]) == 0);
//     }

//     for (size_t i = 0; i < 4; i++){
//         uint32_t id = (i * 2) + 1;
//         uint32_t next = DoubleLinkList__getNext(dll, ids[id]);
//         assert(next == ids[id + 2]);
//     }
    
//     for (size_t i = 1; i <= 4; i++){
//         uint32_t id = (i * 2) + 1;
//         uint32_t prev = DoubleLinkList__getPrev(dll, ids[id]);
 
//         assert(prev == ids[id - 2]);
//     }

//     // root and tail should will point to NULL
//     assert(DoubleLinkList__getPrev(dll, ids[0]) == NULL_SLOT);
//     assert(DoubleLinkList__getNext(dll, ids[9]) == NULL_SLOT);

//     // all the node to the same root 
//     for (size_t i = 0; i < 5; i++)
//     {
//         uint32_t id = (i * 2) + 1;
//         assert(DoubleLinkList__getRoot(dll, ids[id]) == ids[1]);
//     }
    
//     // reference to empty will get null
//     for (size_t i = 0; i < 5; i++)
//     {
//         uint32_t id = (i * 2) ;
//         assert(DoubleLinkList__getRoot(dll, ids[id]) ==NULL_SLOT);
//     }

//     // foreach will travel the list 
//     DoubleLinkList__foreach(dll, ids[1], times2, NULL);

//     for (size_t i = 0; i < 5; i++)
//     {
//         uint32_t id = (i * 2) + 1;
//         assert(DoubleLinkList__get(dll, ids[id]) == id * 4 );
//     }
    
//     return 0;
// }
