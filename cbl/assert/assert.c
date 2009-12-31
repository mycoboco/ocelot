/*!
 *  @file    assert.c
 *  @brief    Source for Assertion Library (CBL)
 */

#include "cbl/assert.h"
#include "cbl/except.h"    /* except_t */

/*! @brief    exception for assertion failure.
 */
const except_t assert_exceptfail = { "Assertion failed" };

/* end of assert.c */
