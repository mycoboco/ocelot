/*!
 *  @file    dlist.h
 *  @brief    Header for Doubly-Linked List Library (CDSL)
 */

#ifndef DLIST_H
#define DLIST_H


/*! @brief    represents a doubly-linked list.
 */
typedef struct dlist_t dlist_t;

/*! @name    list creating functions:
 */
/*@{*/
dlist_t *dlist_new(void);
dlist_t *dlist_list(void *, ...);
/*@}*/

/*! @name    list destroying functions:
 */
/*@{*/
void dlist_free(dlist_t **);
/*@}*/

/*! @name    node adding/deleting functions:
 */
/*@{*/
void *dlist_add(dlist_t *, long, void *);
void *dlist_addhead(dlist_t *, void *);
void *dlist_addtail(dlist_t *, void *);
void *dlist_remove(dlist_t *, long);
void *dlist_remhead(dlist_t *);
void *dlist_remtail(dlist_t *);
/*@}*/

/*! @name    data/information retrieving functions:
 */
/*@{*/
long dlist_length(const dlist_t *);
void *dlist_get(dlist_t *, long);
void *dlist_put(dlist_t *, long, void *);
/*@}*/

/*! @name    list handling functions:
 */
/*@{*/
void dlist_shift(dlist_t *, long);
/*@}*/


#endif    /* DLIST_H */

/* end of dlist.h */
