/*!
 *  @file    assert.h
 *  @brief    Header for Assertion Library (CBL)
 */

#ifndef ASSERT_H
#define ASSERT_H

#if defined(NDEBUG) || defined(ASSERT_STDC_VER)    /* request the standard version */

#include <assert.h>

#else    /* use "assert.h" supporting an exception */

#include "cbl/except.h"    /* EXCEPT_RAISE */


/*! @brief    replaces the standard assert() with a version supporting an exception.
 *
 *  An activated assert() raises an exception named @c assert_exceptfail that is defined in
 *  assert.c. The differences between this exception version and the standard's version are
 *  - The exception version does not print the given expression, @c e;
 *  - The exception version does not abort; it merely raise an exception and let the exception
 *    handler decide what to do.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: none
 */
#define assert(e) ((void)((e) || (EXCEPT_RAISE(assert_exceptfail), 0)))


/* exception for assertion failure */
extern const except_t assert_exceptfail;


#endif    /* ASSERT_STDC_VER */

#endif    /* ASSERT_H */

/* end of assert.h */
