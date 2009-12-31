/*!
 *  @file    stack.h
 *  @brief    Header for Stack Library (CDSL)
 */

#ifndef STACK_H
#define STACK_H


/*! @brief    represents a stack.
 */
typedef struct stack_t stack_t;


/*! @name    stack creating and destroying functions:
 */
/*@{*/
stack_t *stack_new(void);
void stack_free(stack_t **);
/*@}*/

/*! @name    data handling functions:
 */
/*@{*/
void stack_push(stack_t *, void *);
void *stack_pop(stack_t *);
/*@}*/

/*! @name    misc. functions:
 */
/*@{*/
int stack_empty(const stack_t *);
/*@}*/


#endif    /* STACK_H */

/* end of stack.h */
