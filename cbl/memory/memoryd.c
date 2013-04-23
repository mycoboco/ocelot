/*!
 *  @file    memoryd.c
 *  @brief    Source for Memory Management Library - Debugging Version (CBL)
 */

#include <stddef.h>    /* NULL, size_t */
#include <stdlib.h>    /* malloc */
#include <string.h>    /* memcpy, memset */
#include <stdio.h>     /* FILE, fflush, fputs, fprintf, stderr */
#if __STDC_VERSION__ >= 199901L    /* C99 supported */
#include <stdint.h>    /* uintptr_t */
#endif    /* __STDC_VERSION__ */

/* Because of direct references to assert_exceptfail, memoryd.c cannot be compiled with NDEBUG
   defined. NDEBUG makes "assert.h" include <assert.h> (the standard version) and do nothing, which
   prevents assert_exceptfail from being properly declared; using the module for debugging with
   NDEBUG that says "no debug" makes no sense anyway */
#ifdef NDEBUG
#error "This module cannot be compiled with NDEBUG defined!"
#endif

#include "cbl/assert.h"    /* assert with exception support */
#include "cbl/except.h"    /* except_raise, EXCEPT_RAISE */
#include "memory.h"


#define NELEMENT(array) (sizeof(array) / sizeof(*(array)))    /* number of elements in array */

#define HASH(p, t) (((uintptr_t)(p)>>3) % NELEMENT(t))    /* hashing given pointer */

#define NDESCRIPTORS 512    /* number of block descriptors; see descalloc() */

/* smallest multiple of y that is greater than or equal to x */
#define MULTIPLE(x, y) ((((x)+(y)-1)/(y)) * (y))

/* extra bytes allocated with new memory block; see mem_alloc() */
#define NALLOC MULTIPLE(4096, sizeof(union align))

/* checks if pointer is aligned properly */
#define ALIGNED(p) ((uintptr_t)(p) % sizeof(union align) == 0)

/* @brief    raises exception if pointer is invalid.
 *
 * @c RAISE_EXCEPT_IF_INVALID raises an exception if a given pointer @p p is not valid, where an
 * invalid pointer includes an unaligned, foreign or already freed pointer.
 *
 * Possible exceptions: assert_exceptfail
 *
 * Unchecked errors: none
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define RAISE_EXCEPT_IF_INVALID(p, n, type)                                             \
            do {                                                                        \
                if (!ALIGNED(p) || (bp=descfind(p)) == NULL || bp->free) {              \
                    if (logfile)                                                        \
                        logprint((p), (n), bp, file, func, line, (int (*)())(type));    \
                    else                                                                \
                        except_raise(&assert_exceptfail, file, func, line);             \
                }                                                                       \
            } while(0)
#else    /* C90 version */
#define RAISE_EXCEPT_IF_INVALID(p, n, type)                                       \
            do {                                                                  \
                if (!ALIGNED(p) || (bp=descfind(p)) == NULL || bp->free) {        \
                    if (logfile)                                                  \
                        logprint((p), (n), bp, file, line, (int (*)())(type));    \
                    else                                                          \
                        except_raise(&assert_exceptfail, file, line);             \
                }                                                                 \
            } while(0)
#endif    /* __STDC_VERSION__ */


#if __STDC_VERSION__ >= 199901L    /* C99 supported */
#    ifndef UINTPTR_MAX    /* C99, but uintptr_t not provided */
#    error "No integer type to contain pointers without loss of information!"
#    endif    /* UINTPTR_MAX */
#else    /* C90, uintptr_t surely not supported */
typedef unsigned long uintptr_t;
#endif    /* __STDC_VERSION__ */


/* @union    align    memoryd.c
 * @brief    guesses the maximum alignment requirement.
 *
 * union @c align tries to automatically determine the maximum alignment requirement imposed by an
 * implementation; if you know the exact restriction, define @c MEM_MAXALIGN properly - a compiler
 * option like -D should be a proper place to put it. If the guess is wrong, the Memory Management
 * Library is not guaranteed to work properly, or more severely programs that use it may crash. See
 * the warning section of mem_alloc().
 */
union align {
#ifdef MEM_MAXALIGN    /* information on max alignment provided */
    char pad[MEM_MAXALIGN];
#else    /* guesses max alignment with various types */
    int i;
    long l;
    long *lp;
    void *p;
    void (*fp)(void);
    float f;
    double d;
    long double ld;
#endif    /* MEM_MAXALIGN */
};

/* @struct    descriptor    memoryd.c
 * @brief    implements a descriptor for maintaining memory blocks.
 *
 * struct @c descriptor implements a descriptor that is used to maintain memory blocks by adding
 * the information of memory blocks into the descriptor table whenever they are allocated or
 * deallocated. For more detailed explanation on the members, see @c htab below.
 */
struct descriptor {
    struct descriptor *free;    /* pointer to next descriptor for free block; non-null iff freed */
    struct descriptor *link;    /* pointer to next descriptor in same hash entry */
    const void *ptr;            /* pointer to memory block maintained by descriptor */
    size_t size;                /* size of memory block */
    const char *file;           /* file name in which memory block allocated */
#if __STDC_VERSION__ >= 199901L    /* C99 supported */
    const char *func;           /* function name in which memory block allocated */
#endif    /* C99/C90 version */
    int line;                   /* line number on which memory block allocated */
};


/*! @brief    exception for memory allocation failure.
 */
const except_t mem_exceptfail = { "Allocation failed" };

/*  @brief    file pointer used to log invalid memory operations; see mem_log().
 */
static FILE *logfile;

/*  @brief    points to a user-provided free-free log function; see mem_log().
 */
void (*logfuncFreefree)(FILE *, const mem_loginfo_t *);

/*  @brief    points to a user-provided resize-free log function; see mem_log().
 */
void (*logfuncResizefree)(FILE *, const mem_loginfo_t *);

/* @brief    implements a hash table for memory block descriptors.
 *
 * @c htab is an array of pointers to implement a hash table and its pointer elements point to
 * lists for block descriptors. @c htab contains every memory block descriptor whether it describes
 * a freed or used block. For example, suppose there are 3 entries in a table and 3 descriptors,
 * each of which has its own allocated memory blocks. The following depicts this situation:
 *
 * @code
 *     +-+    +-+    +-+
 *     | | -> | | -> | | -> null
 *     +-+    +-+    +-+
 *     | |
 *     +-+    +-+
 *     | | -> | | -> null
 *     +-+    +-+
 *     htab
 * @endcode
 *
 * Now, when one of the memory blocks is freed by invoking mem_free(), the descriptor for that block
 * is marked as "freed" rather than releasing it with free(). This "freed" block is reused if
 * possible when allocation of a new memory block is requested. With two memory blocks are freed, we
 * have:
 *
 * @code
 *     +-+    +-+    +-+
 *     | | -> |F| -> | | -> null
 *     +-+    +-+    +-+
 *     | |
 *     +-+    +-+
 *     | | -> |F| -> null
 *     +-+    +-+
 *     htab
 * @endcode
 *
 * where @c F indicates "freed" blocks. With a separate list to thread the freed blocks (precisely,
 * those blocks marked as "freed"), looking for it when a new block requested can be more
 * efficient, which is what @c freelist defined below for.
 */
static struct descriptor *htab[2048];

/* @brief    threads descriptors for freed memory blocks.
 *
 * @c freelist is a tail dummy node of the list for free blocks, to be precise, of the list for
 * descriptors for free blocks. Its @c free member points to the head node of the list; it points
 * to itself initially as shown below. As explained above, the hash table for descriptors contains
 * all of the freed and allocated descriptors. @c freelist threads only freed blocks and helps to
 * traverse them when memory allocation requested.
 */
static struct descriptor freelist = { &freelist, };


/* @brief    finds a descriptor for a memory block.
 *
 * descfind() finds a descriptor for a memory block in the hash table containing descriptors.
 *
 * @param[in]    p    pointer for which descriptor to find
 *
 * @return    descriptor or null pointer
 *
 * @retval    non-null    descriptor for given memory block
 * @retval    NULL        descriptor not found
 */
static struct descriptor *descfind(const void *p)
{
    struct descriptor *bp = htab[HASH(p, htab)];    /* finds hash entry */

    while (bp && bp->ptr != p)
        bp = bp->link;

    return bp;    /* if not found, bp is null here */
}


/*! @brief    prints a log message to @c logfile properly.
 *
 *  logprint() prints a proper log message depending on the type of errors. This function is
 *  activated when @c logfile is set to a non-null pointer by mem_log() and deactivated when
 *  it is set to a null pointer by the same function. If a user output function is registered by
 *  mem_log(), logprint() calls it with proper arguments. See mem_log() for details.
 *
 *  @param[in]    p       pointer given to mem_free() or mem_resize()
 *  @param[in]    n       size given to mem_resize(); unused when invoked by mem_free()
 *  @param[in]    bp      memory block descriptor found in mem_free() or mem_resize() (if any)
 *  @param[in]    file    file name in which mem_free() or mem_resize() is invoked
 *  @param[in]    func    function name in which mem_free() or mem_resize() is invoked (if C99
 *                        supported)
 *  @param[in]    line    line number on which mem_free() or mem_resize() is invoked
 *  @param[in]    type    type of invalid operation; should be either @c mem_free or @c mem_resize
 *
 *  @return    nothing
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
static void logprint(void *p, size_t n, struct descriptor *bp, const char *file, const char *func,
                     int line, int (*type)())
#else    /* C90 version */
static void logprint(void *p, size_t n, struct descriptor *bp, const char *file, int line,
                     int (*type)())
#endif    /* __STDC_VERSION__ */
{
    enum {
        TYPE_FREE = 0x10,     /* invoked by mem_free() - free-free case */
        TYPE_RESIZE = 0x20    /* invoked by mem_resize() - resize-free case */
    };
    unsigned int logtype;

    assert(logfile);

    logtype = (type == (int (*)())mem_free)? TYPE_FREE:
              (type == (int (*)())mem_resize)? TYPE_RESIZE: 0;

    logtype |= ((logtype == TYPE_FREE && logfuncFreefree) ||
                (logtype == TYPE_RESIZE && logfuncResizefree))? 1: 0;

    assert(logtype != 0);    /* type should be either mem_free or mem_resize */

    if (logtype & 1) {    /* user print function provided */
        mem_loginfo_t loginfo = { 0, };

        loginfo.p = p;
        loginfo.size = n;
        loginfo.ifile = file;
#if __STDC_VERSION__ >= 199901L    /* C99 version */
        loginfo.ifunc = func;
#endif    /* __STDC_VERSION__ */
        if (file && line > 0)
            loginfo.iline = line;
        if (bp) {
            loginfo.afile = bp->file;
#if __STDC_VERSION__ >= 199901L    /* C99 version */
            loginfo.afunc = bp->func;
#endif    /* __STDC_VERSION__ */
            if (bp->file && bp->line > 0)
                loginfo.aline = bp->line;
            loginfo.asize = bp->size;
        }

        if ((logtype & 0xF0) == TYPE_FREE)
            logfuncFreefree(logfile, &loginfo);
        else    /* (logtype & 0xF0) == TYPE_RESIZE */
            logfuncResizefree(logfile, &loginfo);
    } else {    /* defulat message used */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
        const char *resizefree_msg = "** resizing unallocated memory\n"
                                     "mem_resize(%p, %zu) called from %s() %s:%d\n";
        const char *freefree_msg = "** freeing free memory\n"
                                   "mem_free(%p) called from %s() %s:%d\n";
        const char *bpinfo_msg = "this block is %zu bytes long and was allocated from %s() %s:%d\n";
#else    /* C90 version */
        const char *resizefree_msg = "** resizing unallocated memory\n"
                                     "mem_resize(%p, %lu) called from %s:%d\n";
        const char *freefree_msg = "** freeing free memory\n"
                                   "mem_free(%p) called from %s:%d\n";
        const char *bpinfo_msg = "this block is %lu bytes long and was allocated from %s:%d\n";
#endif    /* __STDC_VERSION__ */

        if (!file) {
            file = "unknown file";
        }
#if __STDC_VERSION__ >= 199901L    /* C99 version */
        if (!func)
            func = "unknown function";
#endif    /* __STDC_VERSION__ */

#if __STDC_VERSION__ >= 199901L    /* C99 version */
        if ((logtype & 0xF0) == TYPE_FREE)
            fprintf(logfile, freefree_msg, p, func, file, line);
        else    /* (logtype & 0xF0) == TYPE_RESIZE */
            fprintf(logfile, resizefree_msg, p, n, func, file, line);
        if (bp && bp->file) {
            const char *bpfunc = (bp->func)? bp->func: "unknow function";
            fprintf(logfile, bpinfo_msg, bp->size, bpfunc, bp->file, bp->line);
        }
#else
        if ((logtype & 0xF0) == TYPE_FREE)
            fprintf(logfile, freefree_msg, p, file, line);
        else    /* (logtype & 0xF0) == TYPE_RESIZE */
            fprintf(logfile, resizefree_msg, p, (unsigned long)n, file, line);
        if (bp && bp->file) {
            fprintf(logfile, bpinfo_msg, (unsigned long)bp->size, bp->file, bp->line);
        }
#endif
        fflush(logfile);
    }
}


/*! @brief    deallocates a memory block.
 *
 *  mem_free() releases a given memory block.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    p       pointer to memory block to release (to mark as "freed")
 *  @param[in]    file    file name in which deallocation requested
 *  @param[in]    func    function name in which deallocation requested (if C99 supported)
 *  @param[in]    line    line number on which deallocation is requested
 *
 *  @return    nothing
 *
 *  @internal
 *
 *  mem_free() in the debugging version frees a memory block by adding it to the free block list,
 *  which is marking it as "freed." Note that changing the @c free member in a descriptor structure
 *  from a null pointer to a non-null pointer indicates that it is a freed block when inspecting
 *  the block descriptors.

 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void (mem_free)(void *p, const char *file, const char *func, int line)
#else    /* C90 version */
void (mem_free)(void *p, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    if (p) {
        struct descriptor *bp;

        RAISE_EXCEPT_IF_INVALID(p, 0, mem_free);
        bp->free = freelist.free;    /* pushes to free list */
        freelist.free = bp;
    }
}


/*! @brief    adjusts the size of a memory block pointed to by @p p to @p n.
 *
 *  mem_resize() does the main job of realloc(); adjusting the size of the memory block already
 *  allocated by mem_alloc() or mem_calloc(). While realloc() deallocates like free() when the given
 *  size is 0 and allocates like malloc() when the given pointer is a null pointer, mem_resize()
 *  accepts neither a null pointer nor zero as its arguments. The similar explanation as for
 *  mem_alloc() also applies to mem_resize(). See mem_alloc() for details.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    p       pointer to memory block whose size to be adjusted
 *  @param[in]    n       new size for memory block in bytes
 *  @param[in]    file    file name in which adjustment requested
 *  @param[in]    func    function name in which adjustment requested (if C99 supported)
 *  @param[in]    line    line number on which adjustment requested
 *
 *  @return    pointer to size-adjusted memory block
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *(mem_resize)(void *p, size_t n, const char *file, const char *func, int line)
#else    /* C90 version */
void *(mem_resize)(void *p, size_t n, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    struct descriptor *bp;
    void *np;

    assert(p);
    assert(n > 0);

    RAISE_EXCEPT_IF_INVALID(p, n, mem_resize);
#if __STDC_VERSION__ >= 199901L    /* C99 version */
    np = mem_alloc(n, file, func, line);
    memcpy(np, p, (n < bp->size)? n: bp->size);
    mem_free(p, file, func, line);
#else    /* C90 version */
    np = mem_alloc(n, file, line);
    memcpy(np, p, (n < bp->size)? n: bp->size);
    mem_free(p, file, line);
#endif    /* __STDC_VERSION__ */

    return np;
}


/*! @brief    allocates a zero-filled memory block of the size @p c * @p n in bytes.
 *
 *  mem_calloc() returns a zero-filled memory block whose size is at least @p n. mem_calloc()
 *  allocates a memory block by invoking mem_malloc() and set its every byte to zero by memset().
 *  The similar explanation as for mem_alloc() applies to mem_calloc() too; see mem_alloc().
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    c       number of items to be allocated
 *  @param[in]    n       size in bytes for one item
 *  @param[in]    file    file name in which allocation requested
 *  @param[in]    func    function name in which allocation requested (if C99 supported)
 *  @param[in]    line    line number on which allocation requested
 *
 *  @return    pointer to allocated (zero-filled) memory block
 *
 *  @todo    Improvements are possible and planned:
 *           - the C standard requires calloc() return a null pointer if it can allocates no storage
 *             of the size @p c * @p n in bytes, which allows no overflow in computing the
 *             multiplication. Overflow checking is necessary to mimic the behavior of calloc().
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *(mem_calloc)(size_t c, size_t n, const char *file, const char *func, int line)
#else    /* C90 version */
void *(mem_calloc)(size_t c, size_t n, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    void *p;

    assert(c > 0);    /* precludes zero-sized allocation and deallocation */
    assert(n > 0);

#if __STDC_VERSION__ >= 199901L    /* C99 version */
    p = mem_alloc(c*n, file, func, line);
#else    /* C90 version */
    p = mem_alloc(c*n, file, line);
#endif    /* __STDC_VERSION__ */
    memset(p, '\0', c*n);

    return p;
}


/* @brief    returns an available descriptor.
 *
 * descalloc() returns an available descriptor from its descriptor pool if any. If none, it
 * allocates new @c NDESCRIPTORS descriptors and returns one of them.
 *
 * @warning    The storage for descriptors allocated by descalloc() is never deallocated. For
 *             example, when 512 descriptors had been consumed and there is a request for new one,
 *             descalloc() gives up tracking of the former 512 descriptors and allocates new 512
 *             descriptors to @c avail. This surely causes memory leak, but is not a big deal
 *             since the purpose of the library is to debug. A tool to check memory leak like
 *             Valgrind would report that descalloc() does not return to an OS storages it
 *             allocated, but it is intentional and never means that a program in debugging with
 *             the library has a bug.
 *
 * @param[in]    p       pointer to memory block for which descriptor to be returned
 * @param[in]    n       size of memory block pointed by @p p
 * @param[in]    file    file name in which memory block allocated
 * @param[in]    func    function name in which memory block allocated (if C99 supported)
 * @param[in]    line    line number on which memory block allocated
 *
 * @return    new descriptor for given memory block or null pointer
 *
 * @retval    non-null    descriptor for given memory block
 * @retval    NULL        descriptor creation failed
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
static struct descriptor *descalloc(void *p, size_t n, const char *file, const char *func, int line)
#else    /* C90 version */
static struct descriptor *descalloc(void *p, size_t n, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    static struct descriptor *avail;    /* array of available descriptors */
    static int nleft;    /* number of descriptors left in pool */

    if (nleft <= 0) {    /* additional allocation for descriptors */
        avail = malloc(NDESCRIPTORS * sizeof(*avail));
        if (!avail)
            return NULL;
        nleft = NDESCRIPTORS;
    }

    /* fills up descriptor */
    avail->ptr = p;
    avail->size = n;
    avail->file = file;
#if __STDC_VERSION__ >= 199901L    /* C99 version */
    avail->func = func;
#endif    /* __STDC_VERSION__ */
    avail->line = line;
    avail->free = avail->link = NULL;

    nleft--;

    return avail++;    /* prepare to return next descriptor on next request */
}


/*! @brief    allocates a new memory block of the size @p n in bytes.
 *
 *  Some general explanation on mem_alloc() can be found on the production version of the library.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    n       size of memory block requested in bytes
 *  @param[in]    file    file name in which allocation requested
 *  @param[in]    func    function name in which allocation requested (if C99 supported)
 *  @param[in]    line    linu number on which allocation requested
 *
 *  @return    memory block requested
 *
 *  @internal
 *
 *  The following explains properties only specific to the debugging version.
 *
 *  mem_alloc() first tries to find a memory block whose size is greater than @p n in the free list
 *  maintained by both the hash table @c htab and @c freelist. If there is no one, it allocates a
 *  new block by invoking malloc() and pushes it to the free list to make it a sure find, and then
 *  gives it a shot again.
 *
 *  One of most important things in this debugging version is that mem_alloc() must not return an
 *  address that has been returned by a previous call. To achieve this, mem_alloc() looks for a
 *  memory block whose size is *greater* than @p n, therefore even if there is one of the exactly
 *  same size as @p n, it is left unused; it is enough to guarantee at least one byte whose address
 *  has been returned before not to be returned again. Besides, if the found block is large enough
 *  to satisfy the specified size @p n, it is split and the *bottom* part is returned as the result
 *  preserving the top address (which has been returned before) in the list. When such split
 *  occurs, a new descriptor given by descalloc() is assigned to that new block.
 *
 *  Differently from the original code, this implementation checks with union @c align if the
 *  storage allocated by malloc() is properly aligned and makes assert() fail if not. This signals
 *  to define @c MEM_MAXALIGN proplery to match the real alignment restriction of an implementation
 *  in use. If this alarm ignored, a program using this library is not guaranteed to work.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *(mem_alloc)(size_t n, const char *file, const char *func, int line)
#else    /* C90 version */
void *(mem_alloc)(size_t n, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    struct descriptor *bp;
    void *p;

    assert(n > 0);

    n = MULTIPLE(n, sizeof(union align));    /* n adjusted here, thus at least MULTIPLE(n, align)
                                                bytes never reused */

    for (bp = freelist.free; bp; bp = bp->free) {
        if (bp->size > n) {    /* enough large block found - first fit;
                                  note equality comparison is excluded */
            bp->size -= n;
            p = (char *)bp->ptr + bp->size;    /* allocates from bottom up */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
            if ((bp = descalloc(p, n, file, func, line)) != NULL) {
#else    /* C90 version */
            if ((bp = descalloc(p, n, file, line)) != NULL) {
#endif    /* __STDC_VERSION__ */
                /* pushes to hash table and return */
                unsigned h = HASH(p, htab);
                bp->link = htab[h];
                htab[h] = bp;
                return p;
            } else {    /* descriptor allocation failed */
                if (!file)
                    EXCEPT_RAISE(mem_exceptfail);
                else
#if __STDC_VERSION__ >= 199901L    /* C99 version */
                    except_raise(&mem_exceptfail, file, func, line);
#else    /* C90 version */
                    except_raise(&mem_exceptfail, file, line);
#endif    /* __STDC_VERSION__ */
            }
        }

        if (bp == &freelist) {    /* proper block not found in free list */
            struct descriptor *np;
            if ((p = malloc(n + NALLOC)) == NULL ||
#if __STDC_VERSION__ >= 199901L    /* C99 version */
                (np = descalloc(p, n+NALLOC, __FILE__, __func__, __LINE__)) == NULL) {
#else    /* C90 version */
                (np = descalloc(p, n+NALLOC, __FILE__, __LINE__)) == NULL) {
#endif    /* __STDC_VERSION__ */
                free(p);
                if (!file)
                    EXCEPT_RAISE(mem_exceptfail);
                else
#if __STDC_VERSION__ >= 199901L    /* C99 version */
                    except_raise(&mem_exceptfail, file, func, line);
#else    /* C90 version */
                    except_raise(&mem_exceptfail, file, line);
#endif    /* __STDC_VERSION__ */
            }
            /* checks if guess at alignment restriction holds */
            assert(ALIGNED(p));    /* if fails, define MEM_MAXALIGN properly */
            /* pushes new block to free list to make it sure find and repeat */
            np->free = freelist.free;
            freelist.free = np;
        }
    }
    assert(!"bp is null - should never reach here");

    return NULL;
}


/*! @brief    starts to log invalid memory usage.
 *
 *  mem_log() starts to log invalid memory usage; deallocating an already released memory called
 *  "free-free" or "double free" and resizing a non-allocated memory called "resize-free" here.
 *  mem_log() provides two ways to log them. A user can register his/her own log function for the
 *  free-free or resize-free case by providing a function to @p freefunc or @p resizefunc. The
 *  necessary information is provided to the registered function via a @c mem_loginfo_t object. A
 *  user-provided log function must be defined as follows:
 *
 *  @code
 *      void user_freefree(FILE *fp, const mem_loginfo_t *info)
 *      {
 *          ...
 *      }
 *  @endcode
 *
 *  See the explanation of @c mem_loginfo_t for the information provided to a user-registered
 *  function. The file pointer given to mem_log()'s @p fp is passed to the first parameter of an
 *  user-registered log function.
 *
 *  If @p freefunc or @p resizefunc are given a null pointer, the default log messages are printed
 *  to the file specified by @p fp. The message looks like:
 *
 *  @code
 *      ** freeing free memory
 *      mem_free(0x6418) called from table_mgr() table.c:461
 *      this block is 48 bytes long and was allocated table_init() table.c:233
 *      ** resizing unallocated memory
 *      mem_resize(0xf7ff, 640) called from table_mgr() table.c:468
 *      this block is 32 bytes long and was allocated table_init() table.c:230
 *  @endcode
 *
 *  Invoking mem_log() with a null pointer for @p fp stops logging.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: invalid file pointer given for @p fp, invalid function pointer given for
 *                    @p freefunc or @p resizefunc
 *
 *  @param[in]    fp            file to which log message printed out
 *  @param[in]    freefunc      user-provided function to log free-free case; default message used
 *                              when null pointer given
 *  @param[in]    resizefunc    user-provided function to log resize-free case; default message used
 *                              when null pointer given
 *
 *  @return    nothing
 */
void (mem_log)(FILE *fp, void freefunc(FILE *, const mem_loginfo_t *),
             void resizefunc(FILE *, const mem_loginfo_t *))
{
    logfile = fp;
    logfuncFreefree = freefunc;
    logfuncResizefree = resizefunc;
}


/* @brief    prints the information for memory leak to a proper file.
 *
 * leakinuse() is invoked by mem_leak() if no user-callback function is provided. It prints the
 * memory leak information in a similar form to what logprint() does. A user can specify a file
 * to which the information goes using the last parameter @p cl of mem_leak() which is passed to
 * leakinuse(). If @p cl is a null pointer, leakinuse() inspects a file possibly set by mem_log() to
 * see if it can use that file, before it prints to @c stderr.
 *
 * @param[in]    loginfo    information for memory leak
 * @param[in]    cl         file to which information printed out
 *
 * @return    nothing
 */
static void leakinuse(const mem_loginfo_t *loginfo, void *cl)
{
    FILE *log = (cl)? cl:
                (logfile)? logfile: stderr;
#if __STDC_VERSION__ >= 199901L    /* C99 version */
    const char *file = (loginfo->afile)? loginfo->afile: "unknown file",
               *func = (loginfo->afunc)? loginfo->afunc: "unknown function";
    const char *msg = "** memory in use at %p\n"
                      "this block is %zu bytes long and was allocated from %s() %s:%d\n";

    fprintf(log, msg, loginfo->p, loginfo->size, func, file, loginfo->aline);
#else    /* C90 version */
    const char *file = (loginfo->afile)? loginfo->afile: "unknown file";
    const char *msg = "** memory in use at %p\n"
                      "this block is %ld bytes long and was allocated from %s:%d\n";

    fprintf(log, msg, loginfo->p, (unsigned long)loginfo->size, file, loginfo->aline);
#endif    /* __STDC_VERSION__ */

    fflush(log);
}


/*! @brief    calls a user-provided function for each memory block in use.
 *
 *  mem_leak() is useful when detecting memory leakage. Before terminating a program, calling it
 *  with a callback function which are passed to @p apply makes the callback function called with
 *  the information of every memory block still in use (or not deallocated). Among the member of
 *  @c mem_loginfo_t, @c p, @c size, @c afile, @c afunc and @c aline are filled; if some of them are
 *  unavailable, they are set to a null pointer for pointer members or 0 for integer members. An
 *  information that a user needs to give to a callback function can be passed through @p cl. The
 *  following shows an example of a callback function:
 *
 *  @code
 *      void inuse(const mem_loginfo_t *loginfo, void *cl)
 *      {
 *          FILE *logfile = cl;
 *          const char *file, func;
 *
 *          file = (loginfo->afile)? loginfo->afile: "unknown file";
 *          func = (loginfo->afunc)? loginfo->afunc: "unknown function";
 *
 *          fprintf(logfile, "** memory in use at %p\n", loginfo->p);
 *          fprintf(logfile, "this block is %ld bytes long and was allocated from %s() %s:%d\n",
 *                  (unsigned long)loginfo->size, func, file, loginfo->aline);
 *
 *          fflush(logfile);
 *      }
 *  @endcode
 *
 *  If this callback function is invoked by calling mem_leak() as follows:
 *
 *  @code
 *      mem_leak(inuse, stderr);
 *  @endcode
 *
 *  it prints out a list of memory blocks still in use to @c stderr as follows:
 *
 *  @code
 *      ** memory in use at 0xfff7
 *      this block is 2048 bytes long and was allocated from table_init() table.c:235
 *  @endcode
 *
 *  If a null pointer is given to @p apply, the pre-defined internal callback function is invoked
 *  to print the information for memory leak to a file given through @p cl (after converted to a
 *  pointer to @c FILE). If @c cl is also a null pointer, a file possibly set by mem_log() is
 *  inspected to see if it is available, before the information printed finally goes to @c stderr.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: invalid function pointer given for @p apply, invalid file pointer given for
 *                    @p cl when @p apply is given a null pointer
 *
 *  @param[in]    apply         user-provided function to be called for each memory block in use
 *  @param[in]    cl            passing-by argument to @p apply
 *
 *  @return    nothing
 */
void (mem_leak)(void apply(const mem_loginfo_t *, void *), void *cl)
{
    size_t i;
    struct descriptor *bp;
    mem_loginfo_t loginfo = { 0, };

    for (i = 0; i < NELEMENT(htab); i++)
        for (bp = htab[i]; bp; bp = bp->link)
            if (!bp->free) {    /* in use */
                loginfo.p = bp->ptr;
                loginfo.size = bp->size;
                loginfo.afile = bp->file;
#if __STDC_VERSION__ >= 199901L    /* C99 version */
                loginfo.afunc = bp->func;
#endif    /* __STDC_VERSION__ */
                if (bp->file && bp->line > 0)
                    loginfo.aline = bp->line;
                if (apply == NULL)
                    leakinuse(&loginfo, cl);
                else
                    apply(&loginfo, cl);
            }
}

/* end of memoryd.c */
