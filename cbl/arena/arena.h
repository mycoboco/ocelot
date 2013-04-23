/*!
 *  @file    arena.h
 *  @brief    Header for Arena Library (CBL)
 */

#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>    /* size_t */

#include "cbl/except.h"    /* except_t */


/*! @brief    represents an arena.
 *
 *  @c arena_t represents an arena to which storages belongs.
 */
typedef struct arena_t arena_t;


/* exceptions for arena creation/allocation failure */
extern const except_t arena_exceptfailNew;
extern const except_t arena_exceptfailAlloc;


/*! @name    arena creating functions:
 */
/*@{*/
arena_t *arena_new(void);
/*@}*/

/*! @name    memory allocating/deallocating functions:
 */
/*@{*/
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *arena_alloc(arena_t *, size_t n, const char *, const char *, int);
void *arena_calloc(arena_t *, size_t c, size_t n, const char *, const char *, int);
#else    /* C90 version */
void *arena_alloc(arena_t *, size_t n, const char *, int);
void *arena_calloc(arena_t *, size_t c, size_t n, const char *, int);
#endif    /* __STDC_VERSION__ */
void arena_free(arena_t *arena);
/*@}*/

/*! @name    arena destroying functions:
 */
/*@{*/
void arena_dispose(arena_t **);
/*@}*/


/*! @brief    allocates a new arena.
 */
#define ARENA_NEW() (arena_new())

/*! @brief    destroys an arena pointed to by @p pa.
 */
#define ARENA_DISPOSE(pa) (arena_dispose(pa))

/*! @brief    allocates storage whose byte length is @p n for an arena @p a.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define ARENA_ALLOC(a, n) (arena_alloc((a), (n), __FILE__, __func__, __LINE__))
#else    /* C90 version */
#define ARENA_ALLOC(a, n) (arena_alloc((a), (n), __FILE__, __LINE__))
#endif    /* __STDC_VERSION__ */

/*! @brief    allocates zero-filled storage of the size @p c * @p n for an arena @p a.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define ARENA_CALLOC(a, c, n) (arena_calloc((a), (c), (n), __FILE__, __func__, __LINE__))
#else    /* C90 version */
#define ARENA_CALLOC(a, c, n) (arena_calloc((a), (c), (n), __FILE__, __LINE__))
#endif    /* __STDC_VERSION__ */

/*! @brief    deallocates strorages belonging to an arena @p a.
 */
#define ARENA_FREE(a) (arena_free(a))


#endif    /* ARENA_H */

/* end of arena.h */
