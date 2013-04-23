/*!
 *  @file    list.c
 *  @brief    Source for List Library (CDSL)
 */

#include <stdarg.h>    /* va_list, va_start, va_arg, va_end */
#include <stddef.h>    /* NULL, size_t */

#include "cbl/assert.h"    /* assert with exception support */
#include "cbl/memory.h"    /* MEM_NEW, MEM_FREE, MEM_ALLOC */
#include "list.h"


/*! @brief    pushes a new node to a list.
 *
 *  list_push() pushes a pointer value @p data to a list @p list with creating a new node. A null
 *  pointer given for @p list is considered to indicate an empty list, thus not treated as an
 *  error.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p list
 *
 *  @param[in]    list    list to which @p data pushed
 *  @param[in]    data    data to push into @p list
 *
 *  @warning    The return value of list_push() has to be used to update the variable for the list
 *              passed. list_push() takes a list and returns a modified list.
 *
 *  @return    modified list
 */
list_t *(list_push)(list_t *list, void *data)
{
    list_t *p;

    MEM_NEW(p);
    p->data = data;
    p->next = list;

    return p;
}


/*! @brief    constructs a new list using a sequence of data.
 *
 *  list_list() constructs a list whose nodes contain a sequence of data given as arguments; the
 *  first argument is stored in the head (first) node, the second argument in the second node and
 *  so on. There should be a way to mark the end of the argument list, which a null pointer is for.
 *  Any argument following a null pointer argument is not invalid, but ignored.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @warning    Calling list_list() with one argument, a null pointer, is not treated as an error.
 *              Such a call requests an empty list, so returned; note that a null pointer is an
 *              empty list.
 *
 *  @param[in]    data    data to store in head node of new list
 *  @param[in]    ...     other data to store in new list
 *
 *  @return    new list containing given sequence of data
 */
list_t *(list_list)(void *data, ...)
{
    va_list ap;
    list_t *list,
           **plist = &list;    /* points to pointer into which next data is to be stored */

    va_start(ap, data);
    /* if data is null pointer, following loop skipped and null pointer returned */
    for (; data; data = va_arg(ap, void *)) {
        MEM_NEW(*plist);
        (*plist)->data = data;
        plist = &(*plist)->next;
    }
    *plist = NULL;    /* next of last node should be null */
    va_end(ap);

    return list;
}


#if 0    /* non-pointer-to-pointer version of list_list() */
list_t *(list_list)(void *data, ...)
{
    va_list ap;
    list *list;

    /* consider three cases:
           1) empty list
           2) list has only one node
           3) list has two or more nodes */

    va_start(ap, data);
    list = NULL;
    for (; data; data = va_arg(ap, void *)) {
        if (!list) {    /* empty list */
            MEM_NEW(list);
        } else {    /* one or more nodes exist */
            MEM_NEW(list->next);
            list = list->next;
        }
        list->data = data;
    }
    if (list)    /* at least one node */
        list->next = NULL;
    va_end(ap);

    return list;
}
#endif    /* disabled */


/*! @brief    appends a list to another.
 *
 *  list_append() combines two lists by appending @p tail to @p list, which makes the next pointer
 *  of the last node in @p list point to the first node of @p tail.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p list and @p tail
 *
 *  @warning    Do not forget that a null pointer is considered an empty list, not an error.
 *
 *  @param[in,out]    list    list to which @p tail appended
 *  @param[in]        tail    list to append to @p list
 *
 *  @return    appended list whose (pointer) value should be the same as @p list
 *
 *  @todo    Improvements are possible and planned:
 *           - the time complexity of the current implementation is O(N) where N indicates the
 *             number of nodes in a list. With a circular list, where the next node of the last
 *             node set to the head, it is possible for both pushing and appending to be done in a
 *             constant time.
 */
list_t *(list_append)(list_t *list, list_t *tail)
{
    list_t **p = &list;    /* points to pointer whose next set to point to tail */

    while (*p)    /* find last node */
        p = &(*p)->next;
    *p = tail;    /* appending */

    return list;
}


#if 0    /* non-pointer-to-pointer version of list_append() */
list_t *(list_append)(list_t *list, list_t *tail)
{
    /* consider four cases:
           1) empty + empty
           2) empty + one or more nodes
           3) one or more nodes + empty
           4) one or more nodes + one or more nodes */

    if (!list)    /* empty + tail */
        list = tail;
    else {    /* non-empty + tail */
        while (list->next)
            list = list->next;
        list->next = tail;
    }

    return list;
}
#endif    /* disabled */


/*! @brief    duplicates a list.
 *
 *  list_copy() creates a new list by copying nodes of @p list.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p list
 *
 *  @warning    Note that the data pointed by nodes in @p list are not duplicated. An empty list
 *              results in returning a null pointer, which means an empty list.
 *
 *  @param[in]    list    list to duplicate
 *
 *  @return    duplicated list
 */
list_t *(list_copy)(const list_t *list)
{
    list_t *head,
           **plist = &head;    /* points to pointer into which node copied */

    for (; list; list = list->next) {
        MEM_NEW(*plist);
        (*plist)->data = list->data;
        plist = &(*plist)->next;
    }
    *plist = NULL;    /* next of last node should be null */

    return head;
}


#if 0    /* non-pointer-to-pointer version of list_copy() */
list_t *(list_copy)(const list_t *list)
{
    list_t *head, *tlist;

    head = NULL;
    for (; list; list = list->next) {
        if (!head) {    /* for first node */
            MEM_NEW(head);
            tlist = head;
        } else {    /* for rest */
            MEM_NEW(tlist->next);
            tlist = tlist->next;
        }
        tlist->data = list->data;
    }
    if (head)    /* at least one node */
        tlist->next = NULL;

    return head;
}
#endif    /* disabled */


/*! @brief    pops a node from a list and save its data (pointer) into a pointer object.
 *
 *  list_pop() copies a pointer value stored in the head node of @p list to a pointer object
 *  pointed to by @p pdata and pops the node. If the list is empty, list_pop() does nothing and
 *  just returns @p list. If @p pdata is a null pointer list_pop() just pops without saving any
 *  data.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: foreign data structure given for @p list
 *
 *  @param[in]     list     list from which head node popped
 *  @param[out]    pdata    points to pointer into which data (pointer value) stored
 *
 *  @warning    The return value of list_pop() has to be used to update the variable for the list
 *              passed. list_pop() takes a list and returns a modified list.
 *
 *  @return    list with its head node popped
 */
list_t *(list_pop)(list_t *list, void **pdata)
{
    if (list)
    {
        list_t *head = list->next;
        if (pdata)
            *pdata = list->data;
        MEM_FREE(list);
        return head;
    } else
        return list;
}


/*! @brief    reverses a list.
 *
 *  list_reverse() reverses a list, which eventually makes its first node the last and vice versa.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: foreign data structure given for @p list
 *
 *  @param[in]    list    list to reverse
 *
 *  @warning    The return value of list_reverse() has to be used to update the variable for the
 *              list passed. list_reverse() takes a list and returns a reversed list.
 *
 *  @return    reversed list
 */
list_t *(list_reverse)(list_t *list)
{
    list_t *head, *next;

    head = NULL;
    for (; list; list = next) {
        next = list->next;
        list->next = head;
        head = list;
    }

    return head;
}


#if 0    /* pointer-to-pointer version of list_reverse() */
list_t *(list_reverse)(list_t *list)
{
    list_t *head, *next, **plist;

    /* basic idea is to repeat: pick head and push to another list where
                                    list: picked head
                                    next: head after moving head
                                    head: head of another list */

    head = NULL;
    while (list) {
        /* perpare to move head */
        plist = &list->next;    /* A */
        next = list->next;
        /* make head another list's head */
        *plist = head;          /* B */
        head = list;
        /* pick next node */
        list = next;
    }

    /* since no modification to list->next between A and B plist is unnecessary;
       removing it and using for statement result in original version above */

    return head;
}
#endif    /* disabled */


/*! @brief    counts the length of a list.
 *
 *  list_length() counts the number of nodes in @p list.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: foreign data structure given for @p list
 *
 *  @param[in]    list    list whose nodes counted
 *
 *  @return    length of list
 */
size_t (list_length)(const list_t *list)
{
    size_t n;

    for (n = 0; list; list = list->next)
        n++;

    return n;
}


/*! @brief    destroys a list.
 *
 *  list_free() destroys a list by deallocating storages for each node. After the call, the list is
 *  empty, which means that it makes a null pointer. If @p plist points to a null pointer,
 *  list_free() does nothing since it is already empty.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: pointer to foreign data structure given for @p plist
 *
 *  @warning    Note that the type of @p plist is a double pointer to @c list_t.
 *
 *  @param[in,out]    plist    pointer to list to destroy
 *
 *  @return    nothing
 */
void (list_free)(list_t **plist)
{
    list_t *next;

    assert(plist);
    for (; *plist; *plist = next) {
        next = (*plist)->next;
        MEM_FREE(*plist);
    }
}


/*! @brief   calls a user-provided function for each node in a list.
 *
 *  For each node in a list, list_map() calls a user-provided callback function; it is useful when
 *  doing some common task for each node. The pointer given in @p cl is passed to the second
 *  parameter of a user callback function, so can be used as a communication channel between the
 *  caller of list_map() and the callback. Since the callback has the address of @c data (as
 *  opposed to the value of @c data) through the first parameter, it is free to change its content.
 *  One of cases where list_map() is useful is to deallocate storages given for @c data of each
 *  node. Differently from the original implementation, this library also provides a marco named
 *  LIST_FOREACH() that can be used in the similar situation.
 *
 *  Possible exceptions: none (user-provided function may raise some)
 *
 *  Unchecked errors: foreign data structure given for @p list, user callback function doing
 *                    something wrong (see the warning below)
 *
 *  @warning    Be wraned that modification to a list like pushing and popping a node before
 *              finishing list_map() must be done very carefully and highly discouraged. For
 *              example, in a callback function popping a node from the same list list_map() is
 *              applying to may spoil subsequent tasks depending on which node list_map() is
 *              dealing with. It is possible to provide a safer version, but not done because such
 *              an operation is not regarded as appropriate to the list.
 *
 *  @param[in,out]    list     list with which @p apply called
 *  @param[in]        apply    user-provided function (callback)
 *  @param[in]        cl       passing-by argument to @p apply
 *
 *  @return    nothing
 */
void (list_map)(list_t *list, void apply(void **, void *), void *cl)
{
    assert(apply);

    for (; list; list = list->next)
        apply(&list->data, cl);
}


/*! @brief    converts a list to an array.
 *
 *  list_toarray() converts a list to an array whose elements correspond to the data stored in
 *  nodes of the list. This is useful when, say, sorting a list. A function like array_tolist() is
 *  not necessary because it is easy to construct a list scanning elements of an array, for
 *  example:
 *
 *  @code
 *      for (i = 0; i < ARRAY_SIZE; i++)
 *          list = list_push(list, array[i]);
 *  @endcode
 *
 *  The last element of the constructed array is assigned by @p end, which is a null pointer in
 *  most cases. Do not forget to deallocate the array when it is unnecessary.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p list
 *
 *  @warning    The size of an array generated for an empty list is not zero, since there is
 *              always an end-mark value.
 *
 *  @param[in]    list    list to convert to array
 *  @param[in]    end     end-mark to save in last element of array
 *
 *  @return    array converted from list
 */
void **(list_toarray)(const list_t *list, void *end)
{
    void **array;
    size_t i, n;

    n = list_length(list);
    array = MEM_ALLOC((n+1)*sizeof(*array));
    for (i = 0; i < n; i++) {
        array[i] = list->data;
        list = list->next;
    }
    array[i] = end;

    return array;
}

/* end of list.c */
