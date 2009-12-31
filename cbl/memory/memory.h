/*!
 *  @file    memory.h
 *  @brief    Header for Memory Management Library (CBL)
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>    /* size_t */
#include <stdio.h>     /* FILE */

#include "cbl/except.h"    /* except_t */


/*! @brief    contains the information about invalid memory operations.
 *
 *  An object of the type @c mem_loginfo_t is used when the information about an invalid memory
 *  operation is delivered to a user-provided log function. As explained in mem_log(), such a
 *  function must be declared to accept a @c mem_loginfo_t arguments.
 *
 *  Its members contains three kinds of information:
 *  - the information about an invalid memory operation. For example, if mem_free() is invoked for
 *    the storage that is already deallocated, the pointer given to mem_free() is passed through
 *    @c p. In the case of mem_resize(), the requested size is also available in @c size.
 *  - the information to locate an invalid memory operation. The file name, function name and line
 *    number where a problem occurred are provided through @c ifile, @c ifunc and @c iline,
 *    respectively.
 *  - the information about the memory block for which an invalid memory operation is invoked. For
 *    example, the "free-free" case where trying to deallocate already deallocated storage means
 *    that the pointer value delivered to mem_free() was allocated before. @c afile, @c afunc,
 *    @c aline and @c asize provide where it was allocated and what its size was. This information
 *    is useful in tracking how such an invalid operation is invoked.
 *
 *  If any of them is not available, they are set to a null pointer (for @c ifile, @c ifunc,
 *  @c afile and @c afunc) or 0 (for @c size, @c iline, @c aline and @c asize).
 *
 *  @warning    Logging invalid memory operations is activated by mem_log() which is available only
 *              when the debugging version (not the production version) is used.
 */
typedef struct mem_loginfo_t {
    const void *p;        /*!< pointer value used in invalid memory operation */
    size_t size;          /*!< requested size; meaningful only when triggered by mem_resize() */
    const char *ifile;    /*!< file name in which invalid memory operation invoked */
    const char *ifunc;    /*!< function name in which invalid memory operation invoked */
    int iline;            /*!< line number on which invalid memory operation invoked */
    const char *afile;    /*!< file name in which storage in problem originally allocated */
    const char *afunc;    /*!< function name in which storage in problem originally allocated */
    int aline;            /*!< line number on which storage in problem originally allocated */
    size_t asize;         /*!< size of storage in problem when allocated before */
} mem_loginfo_t;


/* exception for memory allocation failure */
extern const except_t mem_exceptfail;


/*! @name    memory allocating functions:
 */
/*@{*/
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *mem_alloc(size_t, const char *, const char *, int);
void *mem_calloc(size_t, size_t, const char *, const char *, int);
#else    /* C90 version */
void *mem_alloc(size_t, const char *, int);
void *mem_calloc(size_t, size_t, const char *, int);
#endif    /* __STDC_VERSION__ */
/*@}*/

/*! @name    memory deallocating functions:
 */
/*@{*/
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void mem_free(void *, const char *, const char *, int);
#else    /* C90 version */
void mem_free(void *, const char *, int);
#endif    /* __STDC_VERSION__ */
/*@}*/

/*! @name    memory resizing functions:
 */
/*@{*/
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void *mem_resize(void *, size_t, const char *, const char *, int);
#else    /* C90 version */
void *mem_resize(void *, size_t, const char *, int);
#endif    /* __STDC_VERSION__ */
/*@}*/


/*! @name    memory debugging functions:
 */
/*@{*/
void mem_log(FILE *, void (FILE *, const mem_loginfo_t *), void (FILE *, const mem_loginfo_t *));
void mem_leak(void (const mem_loginfo_t *, void *), void *);
/*@}*/


/*! @brief    allocates storage of the size @p n in bytes.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define MEM_ALLOC(n) (mem_alloc((n), __FILE__, __func__, __LINE__))
#else    /* C90 version */
#define MEM_ALLOC(n) (mem_alloc((n), __FILE__, __LINE__))
#endif    /* __STDC_VERSION__ */

/*! @brief    allocation zero-filled storage of the size @p c * @p n in bytes.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define MEM_CALLOC(c, n) (mem_calloc((c), (n), __FILE__, __func__, __LINE__))
#else    /* C90 version */
#define MEM_CALLOC(c, n) (mem_calloc((c), (n), __FILE__, __LINE__))
#endif    /* __STDC_VERSION__ */

/*! @brief    allocates to @p p storage whose size is determined by the size of the pointed-to type
 *            by @p p.
 *
 *  A common way to allocate storage to a pointer @c p is as follows:
 *
 *  @code
 *      type *p;
 *      p = malloc(sizeof(type));
 *  @endcode
 *
 *  However, this is error-prone; it might cause the memory corrupted if one forget to change every
 *  instance of @c type when the type of @c p changes to, say, @c another_type. To preclude problems
 *  like this a proposed way to allocate storage for a pointer @p p is:
 *
 *  @code
 *      p = malloc(sizeof(*p));
 *  @endcode
 *
 *  In this code, changing the type of @c p is automatically reflected to the allocation code above.
 *  Note that the expression given in the @c sizeof expression is not evaluated, so the validity of
 *  @c p's value does not matter here.
 *
 *  The macro MEM_NEW() is provided to facilitate such usage. It takes a pointer as an argument and
 *  allocates to it storage whose size is the size of the referrenced type. Therefore it makes an
 *  invalid call to invoke MEM_NEW() with a pointer to an imconplete type like a pointer to @c void
 *  and a pointer to a structure whose type definition is not visible.
 *
 *  Note that the @c sizeof operator does not evaluate its operand, which makes MEM_NEW() evaluate
 *  its argument exactly once as an actual function does. Embedding a side effect in the argument,
 *  however, is discouraged.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: none
 */
#define MEM_NEW(p) ((p) = MEM_ALLOC(sizeof *(p)))

/*! @brief    allocates to @p p zero-filled storage whose size is determined by the size of the
 *            pointed-to type by @p p.
 *
 *  The same explanation for MEM_NEW() applies. See MEM_NEW() for details.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: none
 */
#define MEM_NEW0(p) ((p) = MEM_CALLOC(1, sizeof *(p)))

/*! @brief    deallocates storage pointed to by @p p and set it to a null pointer.
 *
 *  See mem_free() for details.
 *
 *  @warning    @p p must be a modifiable lvalue; a rvalue expression or non-modifiable lvalue like
 *              one qualified by @c const is not allowed. Also, MEM_FREE() evaluates its argument
 *              twice, so an argument containing side effects results in an unpredictable result.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: foreign value given for @p p
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define MEM_FREE(p) ((void)(mem_free((p), __FILE__, __func__, __LINE__), (p)=0))
#else    /* C90 version */
#define MEM_FREE(p) ((void)(mem_free((p), __FILE__, __LINE__), (p)=0))
#endif    /* __STDC_VERSION__ */

/*! @brief    adjusts the size of storage pointed to by @p p to @p n bytes.
 *
 *  See mem_resize() for details.
 *
 *  Possible exceptions: mem_exceptfail, assert_exceptfail
 *
 *  Unchecked errors: foreign value given for @p p
 *
 *  @warning    MEM_RESIZE() evaluates its argument twice. An argument containing side effects
 *              results in an unpredictable result.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define MEM_RESIZE(p, n) ((p) = mem_resize((p), (n), __FILE__, __func__, __LINE__))
#else    /* C90 version */
#define MEM_RESIZE(p, n) ((p) = mem_resize((p), (n), __FILE__, __LINE__))
#endif    /* __STDC_VERSION__ */


#endif    /* MEMORY_H */

/* end of memory.h */
