/*!
 *  @file    stack.c
 *  @brief    Source for Stack Library (CDSL)
 */

#include <stddef.h>    /* NULL */

#include "cbl/assert.h"    /* assert with exception support */
#include "cbl/memory.h"    /* MEM_NEW, MEM_FREE */
#include "stack.h"


/* @struct    stack_t    stack.c
 * @brief    implements a stack with a linked list.
 */
struct stack_t {
    /* @struct    node    stack.c
     * @brief    represents a node in a stack.
     */
    struct node {
        void *data;           /* data contained in node */
        struct node *next;    /* pointer to next node */
    } *head;                  /* head node of stack */
};


/*! @brief    creates a stack.
 *
 *  stack_new() creates a new stack and sets its relevant information to the initial.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @return    created stack
 */
stack_t *(stack_new)(void)
{
    stack_t *stk;

    MEM_NEW(stk);
    stk->head = NULL;

    return stk;
}


/*! @brief    inspects if a stack is empty.
 *
 *  stack_empty() inspects if a given stack is empty.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked erros: foreign data structure given for @p stk
 *
 *  @param[in]    stk    stack to inspect
 *
 *  @return    whether stack is empty or not
 *
 *  @retval    1    empty
 *  @retval    0    not empty
 */
int (stack_empty)(const stack_t *stk)
{
    assert(stk);
    return (!stk->head);
}


/*! @brief    pushes data into a stack.
 *
 *  stack_push() pushes data into the top of a stack. There is no explicit limit on the maximum
 *  number of data that can be pushed into a stack.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p stk
 *
 *  @param[in,out]    stk     stack into which given data pushed
 *  @param[in]        data    data to push
 *
 *  @return    nothing
 */
void (stack_push)(stack_t *stk, void *data)
{
    struct node *t;

    assert(stk);
    MEM_NEW(t);
    t->data = data;
    t->next = stk->head;
    stk->head = t;
}


/*! @brief    pops data from a stack.
 *
 *  stack_pop() pops data from a given stack. If the stack is empty, an exception is raised due to
 *  the assertion failure, so popping all data without knowing the number of nodes remained in the
 *  stack needs to use stack_empty() to decide when to stop.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p stk
 *
 *  @param[in,out]    stk    stack from which data popped
 *
 *  @return    data popped from stack
 */
void *(stack_pop)(stack_t *stk)
{
    void *data;
    struct node *t;

    assert(stk);
    assert(stk->head);

    t = stk->head;
    stk->head = t->next;
    data = t->data;
    MEM_FREE(t);

    return data;
}


/*! @brief    peeks the top-most data in a stack.
 *
 *  stack_peek() provides a way to inspect the top-most data in a stack without popping it up.
 *
 *  Possible exceptions:
 *
 *  Unchecked errors:
 *
 *  @param[in]    stk    stack to peek
 *
 *  @return    top-most data in stack
 */
void *(stack_peek)(const stack_t *stk)
{
    void *data;
    struct node *t;

    assert(stk);
    assert(stk->head);

    return (void *)((struct node *)stk->head)->data;
}


/*! @brief    destroys a stack.
 *
 *  stack_free() deallocates all storages for a stack and set the pointer passed through @p stk to a
 *  null pointer. Note that stack_free() does not deallocate any storage for the data in the stack
 *  to destroy, which must be done by a user.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p stk
 *
 *  @warning    The storage allocated for data (whose address a stack's node possesses) is never
 *              touched; its allocation and deallocation is entirely up to the user.
 *
 *  @param[in,out]    stk    pointer to stack to destroy
 *
 *  @return    nothing
 */
void (stack_free)(stack_t **stk)
{
    struct node *t, *u;

    assert(stk);
    assert(*stk);

    for (t = (*stk)->head; t; t = u) {
        u = t->next;
        MEM_FREE(t);
    }
    MEM_FREE(*stk);
}

/* end of stack.c */
