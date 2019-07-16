/**
 * Because this is mainly used in the page table, we want it
 * to not inlcude the 0 id
 */
#include "dynamicArrOne.h"
#include <stdint.h>

typedef DynamicArrOne_t DoubleLinkList_t;
#define NULL_SLOT ((uint32_t)0)

// this can be packed when you imported 
typedef struct Node_s
{
    uint32_t prev;
    uint32_t next;
    uint64_t data;
} * Node_t;

DoubleLinkList_t DoubleLinkList__init(){
    return DynamicArrOne__init(sizeof(struct Node_s));
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
    if (first_node->next == second) return second;
    // recursive to find the end of the list and add it there
    if (first_node->next != NULL_SLOT) 
        return DoubleLinkList__link(dll, first_node->next,second);

    Node_t second_node = DynamicArrOne__get(dll,second);

    if (second_node == NULL) return 0;
    /**
     * At here we do know these facts
     * @pre: first, second != NULL_SLOT
     * @pre: first_node, second_node != NULL
     * @pre: first_node-> next == NULL_SLOT 
     * @pre: first_node-> next != second
     * @post: second_node-> pre == fist
     * @post: first_node -> next == second
     * @ret success
     */
    first_node -> next = second;
    second_node -> prev = first;
    return second;
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

    Node_t prev_node = DynamicArrOne__get(dll, curr->prev);    
    Node_t next_node = DynamicArrOne__get(dll, curr->next);

    // the candidat root id;
    uint32_t ret = NULL_SLOT; 
    if (prev_node != NULL && next_node != NULL) 
    {
        prev_node -> next = curr->next;
        next_node -> prev == curr->prev;
        ret = curr->prev;
    } else if (prev_node != NULL)
    {
        prev_node->next = NULL_SLOT;
        ret = curr->prev;
    } else if (next_node != NULL){
        next_node->prev = NULL_SLOT;
        ret = curr->next;
    }
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
    return curr->data = data;
}


int main(int argc, char const *argv[])
{
    
    return 0;
}
