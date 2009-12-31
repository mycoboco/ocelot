/*!
 *  @file    memory.c
 *  @brief    Source for Memory Management Library - Production Version (CBL)
 */

#include <stddef.h>    /* size_t */
#include <stdlib.h>    /* malloc, calloc, realloc, free */

#include "cbl/assert.h"    /* assert with exception support */
#include "cbl/except.h"    /* EXCEPT_RAISE, except_raise */
#include "memory.h"


/*! @brief    exception for memory allocation failure.
 */
const except_t mem_exceptfail = { "Allocation failed" };


/*! @brief    allocates storage of the size @p n in bytes.
 *
 *  mem_alloc() does the same job as malloc() except:
 *  - mem_alloc() raises an exception when fails the requested allocation;
 *  - mem_alloc() does not take 0 as the byte length to preclude the possibility of returning a null
 *    pointer;
 *  - mem_alloc() never returns a null pointer.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    n       size in bytes for storage to be allocated
 *  @param[in]    file    file name in which storage requested
 *  @param[in]    func    function name in which strage requested (if C99 supported)
 *  @param[in]    line    line number on which storage requested
 *
 *  @return    pointer to allocated storage
 *
 *  @warning    mam_alloc() returns no null pointer in any case. Allocation failure triggers an
 *              exception, so no need to handle an exceptional case with the return value.
 *
 *  @internal
 *
 *  In the original implementation, all memory allocation functions take a signed integer as the
 *  size for the requested storage in order to prevent a negative value passed mistakenly from
 *  resulting in plausible allocation, but that practice dropped here.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *(mem_alloc)(size_t n, const char *file, const char *func, int line)
#else    /* C90 version */
void *(mem_alloc)(size_t n, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    void *p;

    assert(n > 0);    /* precludes zero-sized allocation */

    p = malloc(n);
    if (!p)
    {
        if (!file)
            EXCEPT_RAISE(mem_exceptfail);
        else
#if __STDC_VERSION__ >= 199901L    /* C99 version */
            except_raise(&mem_exceptfail, file, func, line);
#else    /* C90 version */
            except_raise(&mem_exceptfail, file, line);
#endif    /* __STDC_VERSION__ */
    }

    return p;
}


/*! @brief    allocates zero-filled storage of the size @p c * @p n in bytes.
 *
 *  mem_calloc() does the same job as mem_alloc() except that the storage it allocates are zero-
 *  filled. The similar explanation as for mem_alloc() applies to mem_calloc() too; see mem_alloc()
 *  for details.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    c       number of items to be allocated
 *  @param[in]    n       size in bytes for one item
 *  @param[in]    file    file name in which storage requested
 *  @param[in]    func    function name in which strage requested (if C99 supported)
 *  @param[in]    line    line number on which storage requested
 *
 *  @return    pointer to allocated (zero-filled) storage
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

    p = calloc(c, n);
    if (!p)
    {
        if (!file)
            EXCEPT_RAISE(mem_exceptfail);
        else
#if __STDC_VERSION__ >= 199901L    /* C99 version */
            except_raise(&mem_exceptfail, file, func, line);
#else    /* C90 version */
            except_raise(&mem_exceptfail, file, line);
#endif    /* __STDC_VERSION__ */
    }

    return p;
}


/*! @brief    deallocates storage pointed to by @p p.
 *
 *  mem_free() is a simple wrapper function for free().
 *
 *  The additional parameters, @p file, @p func (if C99 supported), @p line are for the consistent
 *  form in the calling sites; the debugging version of this library takes advantage of them to
 *  raise an exception when something goes wrong in mem_free(). When using the debugging version,
 *  some of the following unchecked errors are to be detected.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: foreign value given for @p p
 *
 *  @warning    A "foreign" value also includes a pointer value which points to storage already
 *              moved to a different address by, say, mem_resize().
 *
 *  @param[in]    p       pointer to storage to release
 *  @param[in]    file    file name in which deallocation requested
 *  @param[in]    func    function name in which deallocation requested (if C99 supported)
 *  @param[in]    line    line number on which deallocation requested
 *
 *  @return    nothing
 *
 *  @internal
 *
 *  Differently from the original code it does not check if @p p is a null pointer since it is not
 *  invalid to pass a null pointer to free(); free() does nothing for it.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void (mem_free)(void *p, const char *file, const char *func, int line)
#else    /* C90 version */
void (mem_free)(void *p, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    /* no need to test if p is null pointer */
    free(p);
}


/*! @brief    adjust the size of storage pointed to by @p p to @p n.
 *
 *  mem_resize() does the main job of realloc(); adjusting the size of storage already allocated by
 *  mem_alloc() or mem_calloc(). While realloc() deallocates like free() when the given size is 0
 *  and allocates like malloc() when the given pointer is a null pointer, mem_resize() accepts
 *  neither a null pointer nor zero as its arguments. The similar explanation as for mem_alloc()
 *  also applies to mem_resize(). See mem_alloc() for details.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: foreign value given for @p p
 *
 *  @param[in]    p       pointer to storage whose size to be adjusted
 *  @param[in]    n       new size for storage
 *  @param[in]    file    file name in which adjustment requested
 *  @param[in]    func    function name in which adjustment requested (if C99 supported)
 *  @param[in]    line    line number on which adjustment requested
 *
 *  @return    pointer to size-adjusted storage
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *(mem_resize)(void *p, size_t n, const char *file, const char *func, int line)
#else    /* C90 version */
void *(mem_resize)(void *p, size_t n, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    assert(p);
    assert(n > 0);    /* precludes zero-sized allocation; for realloc() this means free() */

    p = realloc(p, n);
    if (!p)
    {
        if (!file)
            EXCEPT_RAISE(mem_exceptfail);
        else
#if __STDC_VERSION__ >= 199901L    /* C99 version */
            except_raise(&mem_exceptfail, file, func, line);
#else    /* C90 version */
            except_raise(&mem_exceptfail, file, line);
#endif    /* __STDC_VERSION__ */
    }

    return p;
}


/* @brief    provides a dummy function for mem_log() that is activated in the debugging version.
 *
 * This function is a place-holder (dummy function) for mem_log() that is activated in the
 * debugging version. Without this a user would be obliged to comment out calls to mem_log() when
 * switching to the production version of the library. The cost for calling this dummy function
 * would not matter since mem_log() is called at most once in general.
 *
 * @return    nothing
 */
void (mem_log)(FILE *fp, void freefunc(FILE *, const mem_loginfo_t *),
             void resizefunc(FILE *, const mem_loginfo_t *))
{
    /* do nothing in production version */
}


/* @brief    provides a dummy function for mem_leak() that is activated in the debugging version.
 *
 * This function is a place-holder (dummy function) for mem_leak() that is activaed in the
 * debugging version. Without this a user would be obliged to comment out calls to mem_leak() when
 * switching to the production version of the library. The cost for calling this dummy function
 * would not matter since mem_leak() is called at most once in general.
 *
 * @return    nothing
 */
void (mem_leak)(void apply(const mem_loginfo_t *, void *), void *cl)
{
    /* do nothing in production version */
}

/* end of memory.c */
