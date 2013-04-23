/*!
 *  @file    dlist.c
 *  @brief    Source for Doubly-Linked List Library
 */

#include <limits.h>    /* LONG_MAX */
#include <stddef.h>    /* NULL */
#include <stdarg.h>    /* va_start, va_arg, va_end, va_list */

#include "cbl/assert.h"    /* assert with exception support */
#include "cbl/memory.h"    /* MEM_NEW0, MEM_FREE, MEM_NEW */
#include "dlist.h"


/* @struct    dlist_t    dlist.c
 * @brief    implements a doubly-linked list (a.k.a. a ring)
 *
 * Note that because a negative position is allowed when adding a node to a list, the length of a
 * list is represented as @c long rather than @c size_t.
 */
struct dlist_t {
    /* @struct    node    dlist.c
     * @brief    represents a node in a doubly-linked list
     */
    struct node {
        struct node *prev;    /* pointer to previous node */
        struct node *next;    /* pointer to next node */
        void *data;           /* pointer to data */
    } *head;                  /* pointer to start of list */
    long length;              /* length of list (number of nodes) */
    long lastidx;             /* index of last accessed node; meaningful only when lastp is not
                                 null pointer */
    struct node *lastnode;    /* pointer to last accessed node */
};


/*! @brief    creates an empty new list.
 *
 *  dlist_new() creates an empty list and returns it for further use.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @return    empty new list
 */
dlist_t *(dlist_new)(void)
{
    dlist_t *dlist;

    MEM_NEW0(dlist);
    dlist->head = NULL;

    dlist->lastidx = 0;
    dlist->lastnode = NULL;

    return dlist;
}


/*! @brief    constructs a new list using a given sequence of data.
 *
 *  dlist_list() constructs a doubly-linked list whose nodes contain a sequence of data given as
 *  arguments; the first argument is stored in the head (first) node, the second argument in the
 *  second node and so on. There should be a way to mark the end of the argument list, which a
 *  null pointer is for. Any argument following a null pointer argument is not invalid, but ignored.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @warning    Calling dlist_list() with one argument, a null pointer, is not treated as an error.
 *              Such a call request an empty list as calling dlist_new().
 *
 *  @param[in]    data    data to store in head node of new list
 *  @param[in]    ...     other data to store in new list
 *
 *  @return    new list containing given sequence of data
 */
dlist_t *(dlist_list)(void *data, ...)
{
    va_list ap;
    dlist_t *dlist = dlist_new();

    va_start(ap, data);
    /* if data is null pointer, following loop skipped and just dlist_new() returned */
    for (; data; data = va_arg(ap, void *))
        dlist_addtail(dlist, data);    /* dlist_addhi() does dirty job */
    va_end(ap);

    return dlist;
}


/*! @brief    destroys a list.
 *
 *  dlist_free() destroys a list by deallocating storages for each node and for the list itself.
 *  After the call, the list does not exist (do not confuse this with an empty list). If @p pdlist
 *  points to a null pointer, an assertion in dlist_free() fails; it's a checked error.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked error: pointer to foreign data structure given for @p plist
 *
 *  @param[in,out]    pdlist    pointer to list to destroy
 *
 *  @return    nothing
 */
void (dlist_free)(dlist_t **pdlist)
{
    struct node *p,    /* points to node to be freed */
                *q;    /* points to next to node freed */

    assert(pdlist);
    assert(*pdlist);

    if ((p = (*pdlist)->head) != NULL) {
        long n = (*pdlist)->length;
        for (; n-- > 0; p = q) {
            q = p->next;
            MEM_FREE(p);
        }
    }
    MEM_FREE(*pdlist);
}


/*! @brief    returns the length of a list.
 *
 *  dlist_length() returns the length of a list, the number of nodes in it.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in]    dlist    list whose length to be returned
 *
 *  @return    length of list (non-negative)
 */
long (dlist_length)(const dlist_t *dlist)
{
    assert(dlist);
    return dlist->length;
}


/*! @brief    retrieves data stored in the @p i-th node in a list.
 *
 *  dlist_get() brings and return data in the @p i-th node in a list. The first node has the index
 *  0 and the last has n-1 when there are n nodes in a list.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in,out]    dlist    list from which data is to be retrieved
 *  @param[in]        i        index for node
 *
 *  @return    data retrieved
 *
 *  @internal
 *
 *  The Doubly-Linked List Library remembers the last-accessed node in a list. If that node, or one
 *  of its immediately adjacent nodes is requested, dlist_get() returns the result without scanning
 *  the list from the head or the tail, which is dramatically faster than when not using the
 *  remembered node.
 */
void *(dlist_get)(dlist_t *dlist, long i)
{
    long n;
    struct node *q;

    assert(dlist);
    assert(i >= 0 && i < dlist->length);

    q = NULL;

    if (dlist->lastnode) {    /* tries to use last access information */
        if (dlist->lastidx == i)
            q = dlist->lastnode;
        else if (dlist->lastidx == i - 1 || (i == 0 && dlist->lastidx == dlist->length))
            q = dlist->lastnode->next;
        else if (dlist->lastidx - 1 == i || (dlist->lastidx == 0 && i == dlist->length))
            q = dlist->lastnode->prev;
    }

    if (!q) {
        q = dlist->head;
        if (i <= dlist->length / 2)
            for (n = i; n-- > 0; q = q->next)
                continue;
        else    /* ex: length==5 && i==3 => n=2 */
            for (n = dlist->length-i; n-- > 0; q = q->prev)
                continue;
    }

    dlist->lastidx = i;
    dlist->lastnode = q;

    return q->data;
}


/*! @brief    replaces data stored in a node with new given data.
 *
 *  dlist_put() replaces the data stored in the @p i-th node with new given data and retrieves the
 *  old data. For indexing, see dlist_get().
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in,out]    dlist    list whose data to be replaced
 *  @param[in]        i        index for noded
 *  @param[in]        data     new data for substitution
 *
 *  @return    old data
 */
void *(dlist_put)(dlist_t *dlist, long i, void *data)
{
    long n;
    void *prev;
    struct node *q;

    assert(dlist);
    assert(i >= 0 && i < dlist->length);

    q = NULL;

    if (dlist->lastnode) {    /* tries to use last access information */
        if (dlist->lastidx == i)
            q = dlist->lastnode;
        else if (dlist->lastidx == i - 1 || (i == 0 && dlist->lastidx == dlist->length))
            q = dlist->lastnode->next;
        else if (dlist->lastidx - 1 == i || (dlist->lastidx == 0 && i == dlist->length))
            q = dlist->lastnode->prev;
    }

    if (!q) {
        q = dlist->head;
        if (i <= dlist->length / 2)
            for (n = i; n-- > 0; q = q->next)
                continue;
        else
            for (n = dlist->length-i; n-- > 0; q = q->prev)
                continue;
    }

    prev = q->data;
    q->data = data;

    return prev;
}


/*! @brief    adds a node after the last node.
 *
 *  dlist_addtail() inserts a new node after the last node; the index for the new node will be N if
 *  there are N nodes before the insertion. If a list is empty, dlist_addtail() and dlist_addhead()
 *  do the same job. dlist_addtail() is equivalent to dlist_add() with 0 given for the position.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in,out]    dlist    list to which new node to be inserted
 *  @param[in]        data     data for new node
 *
 *  @return   data for new node
 */
void *(dlist_addtail)(dlist_t *dlist, void *data)
{
    struct node *p,    /* points to new node */
                *head;

    assert(dlist);
    assert(dlist->length < LONG_MAX);

    MEM_NEW(p);
    if ((head = dlist->head) != NULL) {    /* there is at least one node */
        p->prev = head->prev;    /* new->prev = head->prev == tail */
        head->prev->next = p;    /* head->prev->next == tail->next = new */
        p->next = head;          /* new->next = head */
        head->prev = p;          /* head->prev = new */
    } else    /* empty list */
        dlist->head = p->prev = p->next = p;

    dlist->length++;
    p->data = data;

    return data;
}


/*! @brief    adds a new node before the head node.
 *
 *  dlist_addhead() inserts a new node before the head node; the new node will be the head node.
 *  dlist_addhead() is equivalent to dlist_add() with 1 given for the position.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in,out]    dlist    list to which new node to be inserted
 *  @param[in]        data     data for new node
 *
 *  @return    data for new node
 */
void *(dlist_addhead)(dlist_t *dlist, void *data)
{
    assert(dlist);

    dlist_addtail(dlist, data);
    dlist->head = dlist->head->prev;    /* moves head node */

    dlist->lastidx++;    /* no need to check dlist->lastnode */

    return data;
}


/*! @brief    adds a new node to a specified position in a list.
 *
 *  dlist_add() inserts a new node to a position specified by @p pos. The position is interpreted
 *  as follows: (5 nodes assumed to be in a list)
 *
 *  @code
 *       1   2    3    4    5    6    positive position values
 *        +-+  +-+  +-+  +-+  +-+
 *        | |--| |--| |--| |--| |
 *        +-+  +-+  +-+  +-+  +-+
 *      -5   -4   -3   -2   -1   0    non-positive position values
 *  @endcode
 *
 *  Non-positive positions are useful when to locate without knowing the length of a list. If a
 *  list is empty both 0 and 1 are the valid values for a new node. Note that @p pos -
 *  (dlist_length()+1) gives a non-negative value for a positive @p pos, and @p pos +
 *  (dlist_length()+1) gives a positive value for a negative @p pos.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in,out]    dlist    list to which new node inserted
 *  @param[in]        pos      position for new node
 *  @param[in]        data     data for new node
 *
 *  @return    data for new node
 */
void *(dlist_add)(dlist_t *dlist, long pos, void *data)
{
    assert(dlist);
    assert(pos >= -dlist->length);
    assert(pos <= dlist->length+1);
    assert(dlist->length < LONG_MAX);

    /* note that first branch below deals with empty list case; code revised to call dlist_addtail()
       rather than dlist_addhead() since former is simpler even if both do same */
    if (pos == 0 || pos == dlist->length+1)
        return dlist_addtail(dlist, data);
    else if (pos == 1 || pos == -dlist->length)
        return dlist_addhead(dlist, data);
    else {    /* inserting node to middle of list */
        long i;
        struct node *p,    /* points to new node */
                    *q;    /* points to node to be next to new node */

        /* find index of node that will become next of new node;
           if pos < 0, pos+(length+1) == positive value for same position */
        pos = (pos < 0)? pos+(dlist->length+1)-1: pos-1;

        q = NULL;

        if (dlist->lastnode) {    /* tries to use last access information */
            if (dlist->lastidx == pos)
                q = dlist->lastnode;
            else if (dlist->lastidx == pos - 1 || (pos == 0 && dlist->lastidx == dlist->length))
                q = dlist->lastnode->next;
            else if (dlist->lastidx - 1 == pos || (dlist->lastidx == 0 && pos == dlist->length))
                q = dlist->lastnode->prev;

            /* adjusts last access information */
            if (dlist->lastidx >= pos)
                dlist->lastidx++;
        }

        if (!q) {
            q = dlist->head;
            if (pos <= dlist->length / 2)
                for (i = pos; i-- > 0; q = q->next)
                    continue;
            else
                for (i = dlist->length-pos; i-- > 0; q = q->prev)
                    continue;
        }

        MEM_NEW(p);
        p->prev = q->prev;    /* new->prev = to_be_next->prev */
        q->prev->next = p;    /* to_be_next->prev->next == to_be_prev->next = new */
        p->next = q;          /* new->next = to_be_next */
        q->prev = p;          /* to_be_next->prev = new */
        dlist->length++;

        p->data = data;

        return data;
    }
}


/*! @brief    removes a node with a specific index from a list.
 *
 *  dlist_remove() removes the @p i-th node from a list. For indexing, see dlist_get().
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in,out]    dlist    list from which node to be removed
 *  @param[in]        i        index for node to remove
 *
 *  @return    data of removed node
 */
void *(dlist_remove)(dlist_t *dlist, long i)
{
    long n;
    void *data;
    struct node *q;    /* points to node to be removed */

    assert(dlist);
    assert(dlist->length > 0);
    assert(i >= 0 && i < dlist->length);

    q = NULL;

    if (dlist->lastnode) {    /* tries to use last access information */
        if (dlist->lastidx == i)
            q = dlist->lastnode;
        else if (dlist->lastidx == i - 1 || (i == 0 && dlist->lastidx == dlist->length))
            q = dlist->lastnode->next;
        else if (dlist->lastidx - 1 == i || (dlist->lastidx == 0 && i == dlist->length))
            q = dlist->lastnode->prev;

        /* adjusts last access information */
        if (dlist->lastidx > i)
            dlist->lastidx--;
        else if (dlist->lastidx == i) {
            dlist->lastnode = dlist->lastnode->next;
            if (dlist->lastidx == dlist->length - 1)
                dlist->lastidx = 0;
        }
    }

    if (!q) {
        q = dlist->head;
        if (i <= dlist->length/2)
            for (n = i; n-- > 0; q = q->next)
                continue;
        else
            for (n = dlist->length-i; n-- > 0; q = q->prev)
                continue;
    }

    if (i == 0)    /* remove head */
        dlist->head = dlist->head->next;
    data = q->data;
    q->prev->next = q->next;
    q->next->prev = q->prev;
    MEM_FREE(q);
    if (--dlist->length == 0)    /* empty */
        dlist->head = dlist->lastnode = NULL;    /* also invalidates dlist->lastnode */

    return data;
}


/*! @brief    removes the last node of a list.
 *
 *  dlist_remtail() removes the last (tail) node of a list. dlist_remtail() is equivalent to
 *  dlist_remove() with dlist_length()-1 for the index.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in,out]    dlist    list from which last node to be removed
 *
 *  @return    data of deleted node
 *
 *  @internal
 *
 *  Note that the last access information is invalidated only when it refers to the tail node which
 *  will get removed. In this case, moving @c lastnode to the tail's next or previous node is not
 *  necessary because it will be the head or the tail after the removal and an access to the head or
 *  the tail node does not entail performance degradation.
 */
void *(dlist_remtail)(dlist_t *dlist)
{
    void *data;
    struct node *tail;

    assert(dlist);
    assert(dlist->length > 0);

    if (dlist->lastnode == dlist->head->prev)    /* implies dlist->lastnode != NULL */
        dlist->lastnode = NULL;    /* invalidates dlist->lastnode */

    tail = dlist->head->prev;
    data = tail->data;
    tail->prev->next = tail->next;    /* tail->next == head */
    tail->next->prev = tail->prev;    /* tail->next->prev == head->prev */
    MEM_FREE(tail);
    if (--dlist->length == 0)    /* empty */
        dlist->head = NULL;

    return data;
}


/*! @brief    removes the first node from a list.
 *
 *  dlist_remhead() removes the first (head) node from a list. dlist_remhead() is equivalent to
 *  dlist_remove() with 0 for the position.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @param[in,out]    dlist    list from which first node to be removed
 *
 *  @return    data of deleted node
 */
void *(dlist_remhead)(dlist_t *dlist)
{
    assert(dlist);
    assert(dlist->length > 0);

    /* dlist->lastnode adjusted in dlist_remtail() */

    dlist->head = dlist->head->prev;    /* turns head to tail */

    return dlist_remtail(dlist);
}


/*! @brief    shifts a list to right or left.
 *
 *  dlist_shift() shifts a list to right or left according to the value of @p n. A positive value
 *  indicates shift to right; for example shift by 1 means to make the tail node become the head
 *  node. Similarly, a negative value indicates shift to left; for example shift by -1 means to
 *  make the head node become the tail node.
 *
 *  The absolute value of the shift distance specified by @p n should be equal to or less than the
 *  length of a list. For exmple, dlist_shift(..., 7) or dlist_shift(..., -7) is not allowed when
 *  there are only 6 nodes in a list. In such a case, dlist_shift(..., 6) or dlist_shift(..., -6)
 *  has no effect as dlist_shift(..., 0) has none.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p dlist
 *
 *  @warning    Note that it is a list itself that dlist_shift() shifts, not the head pointer of a
 *              list.
 *
 *  @param[in,out]    dlist    list to shift
 *  @param[in]        n        direction and distance of shift
 *
 *  @return    nothing
 */
void (dlist_shift)(dlist_t *dlist, long n)
{
    long i;
    struct node *q;    /* points to new head node after shift */

    assert(dlist);
    assert(n >= -dlist->length);
    assert(n <= dlist->length);

    /* following adjustment to i differs from original code */
    if (n >= 0)    /* shift to right: head goes to left */
        i = dlist->length - n;
    else    /* shift to left; head goes to right */
        i = -n;    /* possibility of overflow in 2sC representation */

    q = NULL;

    if (dlist->lastnode) {    /* tries to use last access information */
        if (dlist->lastidx == i)
            q = dlist->lastnode;
        else if (dlist->lastidx == i - 1 || (i == 0 && dlist->lastidx == dlist->length))
            q = dlist->lastnode->next;
        else if (dlist->lastidx - 1 == i || (dlist->lastidx == 0 && i == dlist->length))
            q = dlist->lastnode->prev;

        /* adjusts last access information */
        if (dlist->lastidx < i)
            dlist->lastidx += (dlist->length - i);
        else
            dlist->lastidx -= i;
    }

    if (!q) {
        q = dlist->head;
        if (i <= dlist->length/2)
            for (n = i; n-- > 0; q = q->next)
                continue;
        else
            for (n = dlist->length-i; n-- > 0; q = q->prev)
                continue;
    }

    dlist->head = q;
}

/* end of dlist.c */
