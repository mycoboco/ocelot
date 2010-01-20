/*!
 *  @file    arena.c
 *  @brief    Source for Arena Library (CBL)
 */

#include <stddef.h>    /* size_t, NULL */
#include <stdlib.h>    /* malloc, free */
#include <string.h>    /* memset */
#if __STDC_VERSION__ >= 199901L    /* C99 supported */
#include <stdint.h>    /* uintptr_t */
#endif    /* __STDC_VERSION__ */

#include "cbl/assert.h"    /* assert with exception support */
#include "cbl/except.h"    /* EXCEPT_RAISE, except_raise */
#include "arena.h"


/*! @brief    exception for arena creation failure.
 */
const except_t arena_exceptfailNew = { "Arena creation failed" };

/*! @brief    exception for arena memory allocation failure.
 */
const except_t arena_exceptfailAlloc = { "Arena allocation failed" };


/* smallest multiple of y that is greater than or equal to x */
#define MULTIPLE(x, y) ((((x)+(y)-1)/(y)) * (y))

/* checks if pointer is aligned properly */
#define ALIGNED(p) ((uintptr_t)(p) % sizeof(union align) == 0)

/* max number of memory chunks in maintain in freelist */
#define FREE_THRESHOLD 10


#if __STDC_VERSION__ >= 199901L    /* C99 supported */
#    ifndef UINTPTR_MAX    /* C99, but uintptr_t not provided */
#    error "No integer type to contain pointers without loss of information!"
#    endif    /* UINTPTR_MAX */
#else    /* C90, uintptr_t surely not supported */
typedef unsigned long uintptr_t;
#endif    /* __STDC_VERSION__ */


/* @struct    arena_t    arena.c
 * @brief    implements an arena.
 *
 * An arena is consisted of a list of memory chunks. Each chuck has struct @c arena_t in its start
 * and a memory area that can be or is used by an application follows it.
 *
 * All three pointer members point to somewhere in the previous memory chunk. For example, suppose
 * that there is only one memory chunks allocated so far. The head node that is remembered by an
 * application and passed to, say, arena_free() has three pointers, each of which points to
 * somewhere in that single allocated chunk. And its @c arena_t parts are all set to a null pointer.
 * The following depicts this situation:
 *
 * @code
 *     +---+ <----\       +---+ <------ arena
 *     | 0 |       \------|   | prev
 *     +---+              +---+
 *     | 0 |         /----|   | avail
 *     +---+        /     +---+
 *     | 0 |       /  /---|   | limit
 *     +---+      /  /    +---+
 *     | . |     /  /
 *     | A | <--/  /
 *     | . |      /
 *     +---+ <---/
 * @endcode
 *
 * After one more chunk has been allocates, @c arena above points to it, and the @c arena_t parts
 * of the new chunk point to the previous chunk marked @c A as shown below:
 *
 * @code
 *     +---+ <----\       +---+ <----\       +---+ <------ arena
 *     | 0 |       \------|   |       \------|   | prev
 *     +---+              +---+              +---+
 *     | 0 |          /---|   |         /----|   | avail
 *     +---+         /    +---+        /     +---+
 *     | 0 |        / /---|   |       /  /---|   | limit
 *     +---+       / /    +---+      /  /    +---+
 *     | . |      / /     | . |     /  /
 *     | A |     / /      | B | <--/  /
 *     | . | <--/ /       | . |      /
 *     +---+ <---/        +---+ <---/
 * @endcode
 *
 * It can be thought as pushing a new memory chunk through the head node.
 */
struct arena_t {
    struct arena_t *prev;    /* points to previously allocated memory chunk */
    char *avail;             /* points to start of available area in previous chunk */
    char *limit;             /* points to end of previous memory chunk */
};

/* @union    align    arena.c
 * @brief    guesses the maximum alignment requirement.
 *
 * union @c align tries to automatically determine the maximum alignment requirement imposed by an
 * implementation; if you know the exact restriction, define @c MEM_MAXALIGN properly - a compiler
 * option like -D should be a proper place to put it. If the guess is wrong, the Memory Management
 * Library is not guaranteed to work properly, or more severely programs that use it may crash.
 *
 * Even if this library employs the prefix @c arena_ or @c ARENA_, the prefix of the Memory Library
 * @c MEM_ is used below because union @c align is introduced there.
 *
 * @warning    The identifier @c MEM_MAXALIGN is reserved even if the Memory Management Library is
 *             not used.
 */
union align {
#ifdef MEM_MAXALIGN
    char pad[MEM_MAXALIGN];
#else
    int i;
    long l;
    long *lp;
    void *p;
    void (*fp)(void);
    float f;
    double d;
    long double ld;
#endif
};

/* @union    header    arena.c
 * @brief    makes the available memory area after struct @c arena_t aligned properly.
 *
 * As shown in the explanation of @c arena_t, the memory area that is to be used by an application
 * is attached after the arena_t part of a memory chunk. Because its starting address should be
 * properly aligned as returned by malloc(), it might be necessary to put some padding between the
 * @c arena_t part and the user memory area. union @c header do this job using union @c align. It
 * ensures that there is enough space for @c arena_t and the starting address of the following user
 * area is properly aligned; there should be some padding after the member @c a if necessary in
 * order to make the second element properly aligned in an array of the union @c header type.
 */
union header {
    arena_t b;
    union align a;
};


/* @brief    threads memory chunks that have been deallocated.
 *
 * @c freelist is a list of free memory chunks; to be precise, it points to the first chunk in the
 * free list. When arena_free() called, before it really releases the storage with free(), it set
 * aside that chunk into @c freelist. That reduces necessity of calling malloc(). On the other hand,
 * if @c freelist maintains too many instances of free chunks, invocations for allocation using
 * other memory allocator (for example, mem_alloc()) would too often fail. That is why arena_free()
 * keeps no more than @c FREE_THRESHOLD chunks in @c freelist.
 *
 * Differently from those memory chunks described by @c arena_t, chunks in the free list have their
 * @c limit member point to their own limit (actually no other choices); see @c arena_t for
 * comparison.
 */
static arena_t *freelist;

/* @brief    number of free chunks left in @c freelist.
 *
 * @c freenum is incresed when a memory chunk is pushed to @c freelist in arena_free() and decresed
 * when it is pushed back to the @c arena_t list in arena_alloc().
 */
static int freenum;


/*! @brief    creates a new arena.
 *
 *  arena_new() creates a new arena and initialize it to indicate an empty arena.
 *
 *  Possible exceptions: arena_exceptfailNew
 *
 *  Unchecked errors: none
 *
 *  @return    new arena created
 *
 *  @internal
 *
 *  mlim_add() is invoked after initialization of fields for a new arena. This is a (probably
 *  meaningless) try to retain consistency of an internal data structure. As warned in the
 *  Exception Handling Library, however, a caller of arena_new() loses control over the new arena if
 *  an exception raised by mlim_add(), which is why mlim_add is called with the last argument 1.
 */
arena_t *(arena_new)(void)
{
    arena_t *arena;

    arena = malloc(sizeof(*arena));    /* use malloc() to separate Arena Library from a specific
                                          memory allocator like Memory Library */
    if (!arena)
        EXCEPT_RAISE(arena_exceptfailNew);

    arena->prev = NULL;
    arena->limit = arena->avail = NULL;

    return arena;
}


/*! @brief    disposes an arena.
 *
 *  arena_dispose() releases storages belonging to a given arena and disposes it. Do not confuse
 *  with arena_free() which gets all storages of an arena deallocated but does not destroy the arena
 *  itself.
 *
 *  Note that storages belonging to @c freelist is not deallcated by arena_dispose() because it is
 *  possibly used by other arenas. Thus, a tool detecting the memory leakage problem might say there
 *  is leakage caused by the library, but that is intended not a bug.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p parena
 *
 *  @param[in,out]    parena    pointer to arena to dispose
 *
 *  @return    nothing
 */
void (arena_dispose)(arena_t **parena)
{
    assert(parena);
    assert(*parena);

    arena_free(*parena);
    free(*parena);
    *parena = NULL;
}


/*! @brief    allocates storage associated with an arena.
 *
 *  arena_alloc() allocates storage for an arena as malloc() or mem_alloc() does.
 *
 *  Possible exceptions: assert_exceptfail, arena_exceptfailAlloc
 *
 *  Unchecked errors: foreign data structure given for @p arena
 *
 *  @param[in,out]    arena    arena for which storage to be allocated
 *  @param[in]        n        size of storage requested
 *  @param[in]        file     file name in which storage requested
 *  @param[in]        func     function name in which storage requested (if C99 supported)
 *  @param[in]        line     line number on which storage requested
 *
 *  @return    storage allocated for given arena
 *
 *  @internal
 *
 *  How arena_alloc() works is explained in @c arena_t.
 *
 *  There are three cases where arena_alloc() successfully returns storage:
 *  - if the first chunk in the @c arena_t list has enough space for the request, it is returned;
 *  - otherwise, the first chunk in the free list (if any) is pushed back to the @c arena_t list and
 *    go to the first step;
 *  - if there is nothing in the free list, new storage allocated by malloc() and pushed to the
 *    @c arena_t list and go to the first step.
 *
 *  Note that the second case is not able to make a sure find with enough space while the third is,
 *  which is why arena_alloc() contains a loop.
 *
 *  The original code in the book does not check if the @c limit or @c avail member of @c arena is
 *  a null pointer; by definition, operations like comparing two pointers, subtracting a pointer
 *  from another and adding an integer to a pointer result in undefined behavior if they are
 *  involved with a null pointer, thus fixed here.
 *
 *  An invocation to mlim_add() is performed at the end of arena_alloc(). This is a try to retain
 *  consistency of an internal data structure. As warned in the Exception Handling Library, however,
 *  this does not always guarantee that you have no problem in recovering from a limit exceeded.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *(arena_alloc)(arena_t *arena, size_t n, const char *file, const char *func, int line)
#else    /* C90 version */
void *(arena_alloc)(arena_t *arena, size_t n, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    assert(arena);
    assert(n > 0);

    n = MULTIPLE(n, sizeof(union align));

    assert(arena->limit >= arena->avail);
    /* first request or requested size > left size of first chunk */
    while (!arena->limit || n > arena->limit - arena->avail) {
        arena_t *p;
        char *limit;

        if ((p = freelist) != NULL) {    /* free chunks exist in freelist */
            freelist = freelist->prev;
            freenum--;    /* chunk to be pushed back to arena_t list, so decresed */
            limit = p->limit;
        } else {    /* allocation needed */
            size_t m = sizeof(union header) + n + 10*1024;    /* enough to save arena_t + requested
                                                                 size + extra (10Kb) */
            p = malloc(m);
            if (!p) {
                if (!file)
                    EXCEPT_RAISE(arena_exceptfailAlloc);
                else
#if __STDC_VERSION__ >= 199901L    /* C99 version */
                    except_raise(&arena_exceptfailAlloc, file, func, line);
#else    /* C90 version */
                    except_raise(&arena_exceptfailAlloc, file, line);
#endif    /* __STDC_VERSION__ */
            }
            assert(ALIGNED(p));    /* checks if guess at alignment restriction holds - if fails,
                                      define MEM_MAXALIGN properly */
            limit = (char *)p + m;
        }
        *p = *arena;    /* copies previous arena info to newly allocated chunk */
        /* makes head point to newly pushed chunk */
        arena->avail = (char *)((union header *)p + 1);
        arena->limit = limit;
        arena->prev  = p;
    }
    /* chunk having free space with enough size found */
    arena->avail += n;

    return arena->avail - n;
}


/*! @brief    allocates zero-filled storage associated with an arena.
 *
 *  arena_calloc() does the same as arena_alloc() except that it returns zero-filled storage.
 *
 *  Possible exceptions: assert_exceptfail, arena_exceptfailAlloc
 *
 *  Unchecked errors: foreign data structure given for @p arena
 *
 *  @param[in,out]    arena    arena for which zero-filled storage is to be allocated
 *  @param[in]        c        number of items to be allocated
 *  @param[in]        n        size in bytes for one item
 *  @param[in]        file     file name in which storage requested
 *  @param[in]        func     function name in which storage requested (if C99 supported)
 *  @param[in]        line     line number on which storage requested
 *
 *  @return    zero-filled storage allocated for arena
 *
 *  @todo    Some improvements are possible and planned:
 *           - the C standard requires calloc() return a null pointer if it cannot allocates storage
 *             of the size @p c * @p n in bytes, which allows no overflow in computing the
 *             multiplication. So overflow checking is necessary to mimic the behavior of calloc().
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *(arena_calloc)(arena_t *arena, size_t c, size_t n, const char *file, const char *func,
                     int line)
#else    /* C90 version */
void *(arena_calloc)(arena_t *arena, size_t c, size_t n, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    void *p;

    assert(c > 0);

#if __STDC_VERSION__ >= 199901L    /* C99 version */
    p = arena_alloc(arena, c*n, file, func, line);
#else    /* C90 version */
    p = arena_alloc(arena, c*n, file, line);
#endif    /* __STDC_VERSION__ */
    memset(p, '\0', c*n);

    return p;
}


/*! @brief    deallocates all storages belonging to an arena.
 *
 *  arena_free() releases all storages belonging to a given arena.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p arena
 *
 *  @param[in,out]    arena    arena whose storages to be deallocated
 *
 *  @return    nothing
 *
 *  @internal
 *
 *  arena_free() does its job by by popping the belonging memory chunks until the arena gets empty.
 *  Those popped chunks are released by free() if there are already @c FREE_THRESHOLD chunks in
 *  @c freelist, or pushed to @c freelist otherwise.
 */
void (arena_free)(arena_t *arena)
{
    assert(arena);

    while (arena->prev) {
        arena_t tmp = *arena->prev;
        if (freenum < FREE_THRESHOLD) {    /* need to set aside to freelist */
            arena->prev->prev = freelist;    /* prev of to-be-freed = existing freelist */
            freelist = arena->prev;          /* freelist = to-be-freed */
            freenum++;
            freelist->limit = arena->limit;    /* in the free list, each chunk has the @c limit
                                                  member point to its own limit; see @c arena_t for
                                                  comparison */
        } else    /* freelist is full; deallocate */
            free(arena->prev);
        *arena = tmp;
    }

    /* all are freed here */
    assert(!arena->limit);
    assert(!arena->avail);
}

/* end of arena.c */
