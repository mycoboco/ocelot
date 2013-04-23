/*!
 *  @file    list.h
 *  @brief    Header for List Library (CDSL)
 */

#ifndef LIST_H
#define LIST_H

#include <stddef.h>    /* size_t */


/*! @struct    list_t    list.h
 *  @brief    represents a node in a list.
 *
 *  This implementation for a linked list does not employ a separate data structure for the head or
 *  tail node; see the implementation of the Doubly-Linked List Library for what this means. By
 *  imposing on its users the responsability to make sure that a list given to the library functions
 *  is appropriate for their tasks, it attains simpler implementation.
 *
 *  The detail of struct @c list_t is intentionally exposed to the users (as opposed to be hidden
 *  in an opaque type) because doing so is more useful. For example, a user does not need to
 *  complicate the code by calling, say, list_push() just in order to make a temporary list node.
 *  Declaring it as having the type of @c list_t (as opposed to @c list_t @c *) is enough. In
 *  addition, an @c list_t object can be embedded in a user-created data structure directly.
 */
typedef struct list_t {
    void *data;             /*!< pointer to data */
    struct list_t *next;    /*!< pointer to next node */
} list_t;


/*! @name    list creating functions:
 */
/*@{*/
list_t *list_list(void *, ...);
list_t *list_append(list_t *, list_t *);
list_t *list_push(list_t *, void *);
list_t *list_copy(const list_t *);
/*@}*/

/*! @name    data/information retrieving functions:
 */
/*@{*/
list_t *list_pop(list_t *, void **);
void **list_toarray(const list_t *, void *);
size_t list_length(const list_t *);
/*@}*/

/*! @name    list destroying functions:
 */
/*@{*/
void list_free(list_t **);
/*@}*/

/*! @name    list handling functions:
 */
/*@{*/
void list_map(list_t *, void (void **, void *), void *);
list_t *list_reverse(list_t *);
/*@}*/


/*! @brief    iterates for each node of a list
 *
 *  LIST_FOREACH() macro is useful when doing some task for every node of a list. For example, the
 *  following example deallocates storages for @c data of each node in a list named @c list and
 *  destroy @c list itself using list_free():
 *
 *  @code
 *      list_t *t;
 *
 *      LIST_FOREACH(t, list)
 *      {
 *          MEM_FREE(t->data);
 *      }
 *      list_free(list);
 *  @endcode
 *
 *  The braces enclosing the call to @c MEM_FREE are optional in this case as you may omit them in
 *  ordinary iterative statements. After the loop, @c list is untouched and @c t becomes
 *  indeterminate (if the loop finishes without jumping out of it, it should be a null pointer).
 *  There are cases where LIST_FOREACH() is more convenient than list_map() but the latter is
 *  recommended.
 *
 *  @warning    Be wraned that modification to a list like pushing and popping a node before
 *              finishing the loop must be done very carefully and highly discouraged. For example,
 *              pushing a new node with @c t may invalidate the internal state of the list, popping
 *              a node with @c list may invalidate @c t thus subsequent tasks depending on which
 *              node @c t points to and so on. It is possible to provide a safer version of
 *              LIST_FOREACH() as done by Linux kernel's list implementation, but not done by this
 *              implementation for such an operation is not regarded as appropriate to the list.
 */
#define LIST_FOREACH(pos, list) for ((pos) = (list); (pos); (pos)=(pos)->next)


#endif    /* LIST_H */

/* end of list.h */
