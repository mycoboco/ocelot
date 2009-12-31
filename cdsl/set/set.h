/*!
 *  @file    set.h
 *  @brief    Header for Set Library (CDSL)
 */

#ifndef SET_H
#define SET_H

#include <stddef.h>    /* size_t */


/*! @brief    represents a set.
 */
typedef struct set_t set_t;


/*! @name    set creating/destroying functions:
 */
/*@{*/
set_t *set_new (int, int (const void *, const void *), unsigned (const void *));
void set_free(set_t **);
/*@}*/

/*! @name    data/information retrieving functions:
 */
/*@{*/
size_t set_length(set_t *);
int set_member(set_t *, const void *);
void set_put(set_t *, const void *);
void *set_remove(set_t *, const void *);
/*@}*/

/*! @name    set handling functions:
 */
/*@{*/
void set_map(set_t *, void (const void *, void *), void *);
void **set_toarray(set_t *, void *);
/*@}*/

/*! @name    set operation functions:
 */
/*@{*/
set_t *set_union(set_t *, set_t *);
set_t *set_inter(set_t *, set_t *);
set_t *set_minus(set_t *, set_t *);
set_t *set_diff(set_t *, set_t *);
/*@}*/


#endif    /* SET_H */

/* end of set.h */
