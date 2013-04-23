/*!
 *  @file    bitv.c
 *  @brief    Source for Bit-vector Library (CDSL)
 */

#include <stddef.h>    /* size_t, NULL */
#include <string.h>    /* memcpy */

#include "cbl/memory.h"    /* MEM_ALLOC, MEM_NEW, MEM_FREE */
#include "cbl/assert.h"    /* assert with exception support */
#include "bitv.h"


#define BPW (8 * sizeof(unsigned long))    /*!< number of bits per word */

#define nword(len) (((len)+BPW-1) / BPW)    /*!< number of words for bit-vector of length @c len */
#define nbyte(len) (((len)+8-1) / 8)        /*!< number of bytes for bit-vector of length @c len */

#define BIT(set, n) (((set)->byte[(n)/8] >> ((n)%8)) & 1)    /*!< extracts bit from bit-vector */

/*!< body for functions that work on the range */
#define range(op, fcmp, cmp)                                   \
    do {                                                       \
        assert(set);                                           \
        assert(l <= h);                                        \
        assert(h < set->length);                               \
        if (l/8 < h/8) {                                       \
            set->byte[l/8] op##= cmp msb[l%8];                 \
            {                                                  \
                size_t i;                                      \
                for (i = l/8 + 1; i < h/8; i++)                \
                    set->byte[i] = fcmp 0U;                    \
                set->byte[h/8] op##= cmp lsb[l%8];             \
            }                                                  \
        } else                                                 \
            set->byte[l/8] op##= cmp (msb[l%8] & lsb[h%8]);    \
    } while(0)

/*!< function body for set operations */
#define setop(eq, sn, tn, op)                               \
    do {                                                    \
        assert(s || t);                                     \
        if (s == t) return (eq);                            \
        else if (!s) return (sn);                           \
        else if (!t) return (tn);                           \
        else {                                              \
            size_t i;                                       \
            bitv_t *set;                                    \
            assert(s->length == t->length);                 \
            set = bitv_new(s->length);                      \
            for (i = 0; i < nword(s->length); i++)          \
                set->word[i] = s->word[i] op t->word[i];    \
            return set;                                     \
        }                                                   \
    } while(0)


/* bit masks for msbs */
static unsigned msb[] = {
    0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80
};

/* bit masks for lsbs */
static unsigned lsb[] = {
    0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF
};

/* bit mask for paddings */
static unsigned pad[] = {
    0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F
};


/* @struct    bitv_t    bitv.c
 * @brief    implements a bit-vector.
 *
 * struct @c bitv_t contains a sequence of words to implement a bit-vector whose length in bits is
 * held in @c length. For table-driven approaches, @c byte provides access to an individual byte
 * in words for a bit-vector. Without bit-wise allocation of the storage, unused bits or bytes are
 * unavoidable to pad and they should have no effect on the result because they are always set to
 * zeros.
 */
struct bitv_t {
    size_t length;          /* length in bits for bit-vector */
    unsigned char *byte;    /* byte-wise access to bit-vector words */
    unsigned long *word;    /* words to contain bit-vector */
};


/* @brief    creates a copy of a bit-vector.
 *
 * copy() makes a copy of a given bit-vector by allocating new storage for it. copy() is used for
 * various set operations.
 *
 * @param[in]    t    bit-vector to copy
 *
 * @return    bit-vector copied
 */
static bitv_t *copy(const bitv_t *t)
{
    bitv_t *set;

    assert(t);

    set = bitv_new(t->length);
    if (t->length != 0)
        memcpy(set->byte, t->byte, nbyte(t->length));

    return set;
}


/*! @brief    creates a new bit-vector.
 *
 *  bitv_new() creates a new bit-vector. Since a bit-vector has a much simpler data structure than
 *  a set (provided by @c cdsl/set) does, the only information that bitv_new() needs to create a
 *  new instance is the length of the bit vector it will create; bitv_new() will create a
 *  bit-vector with @c length bits. The length cannot be changed after creation.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  @param[in]    len    length of bit-vector to create
 *
 *  @ret    new bit-vector created
 */
bitv_t *(bitv_new)(size_t len) {
    bitv_t *set;

    MEM_NEW(set);
    set->word = (len > 0)? MEM_CALLOC(nword(len), sizeof(unsigned long)): NULL;
    set->byte = (void *)set->word;
    set->length = len;

    return set;
}


/*! @brief    destroys a bit-vector.
 *
 *  bitv_free() destroys a bit-vector by deallocating the storage for it and set a given pointer to
 *  a null pointer.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: pointer to foreign data structure given for @p pset
 *
 *  @param[in,out]    pset    pointer to bit-vector to destroy
 *
 *  @return    nothing
 */
void (bitv_free)(bitv_t **pset) {
    assert(pset);
    assert(*pset);

    MEM_FREE((*pset)->word);
    MEM_FREE(*pset);
}


/*! @brief    returns the length of a bit-vector.
 *
 *  bitv_length() returns the length of a bit-vector which is the number of bits in a bit-vector.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in]    set    bit-vector whose legnth returned
 *
 *  @return    length of bit-vector
 */
size_t (bitv_length)(const bitv_t *set) {
    assert(set);

    return set->length;
}


/*! @brief    returns the number of bits set.
*
*  bitv_count() returns the number of bits set in a bit-vector.
*
*  Possible exceptions: assert_exceptfail
*
*  Unchecked errors: foreign data structure given for @p set
*
*  @param[in]    set    bit-vector to count
*
*  @return    number of bits set
*/
size_t (bitv_count)(const bitv_t *set)
{
    static char count[] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };

    size_t i, c;

    assert(set);

    for (i = 0; i < nbyte(set->length); i++) {
        unsigned char ch = set->byte[i];
        c += count[ch & 0x0F] + count[ch >> 4];
    }

    return c;
}


/*! @brief    gets a bit in a bit-vector.
 *
 *  bitv_get() inspects whether a bit in a bit-vector is set or not. The position of a bit to
 *  inspect, @p n starts at 0 and must be smaller than the length of the bit-vector to inspect.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in]    set    bit-vector to inspect
 *  @param[in]    n      bit position
 *
 *  @return    bit value (0 or 1)
 */
int (bitv_get)(const bitv_t *set, size_t n)
{
    assert(set);
    assert(n < set->length);

    return BIT(set, n);
}


/*! @brief    changes the value of a bit in a bit-vector.
 *
 *  bitv_put() changes the value of a bit in a bit-vector to 0 or 1 and returns its previous value.
 *  The position of a bit to change, @p n starts at 0 and must be smaller than the length of the
 *  bit-vector.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in,out]    set    bit-vector to set
 *  @param[in]        n      bit position
 *  @param[in]        bit    value
 *
 *  @return    previous value of bit
 */
int (bitv_put)(bitv_t *set, size_t n, int bit)
{
    int prev;

    assert(set);
    assert(bit == 0 || bit == 1);
    assert(n < set->length);

    prev = BIT(set, n);
    if (bit)
        set->byte[n/8] |= 1U << (n%8);
    else
        set->byte[n/8] &= ~(1U << (n%8));

    return prev;
}


/*! @brief    sets bits to 1 in a bit-vector.
 *
 *  bitv_set() sets bits in a specified range of a bit-vector to 1. The inclusive lower bound @p l
 *  and the inclusive upper bound @p h specify the range. @p l must be equal to or smaller than
 *  @p h and @p h must be smaller than the length of the bit-vector to set.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in,out]    set    bit-vector to set
 *  @param[in]        l      lower bound of range (inclusive)
 *  @param[in]        h      upper bound of range (inclusive)
 *
 *  @return    nothing
 */
void (bitv_set)(bitv_t *set, size_t l, size_t h)
{
    range(|, ~, +);
}


/*! @brief    clears bits in a bit-vector.
 *
 *  bitv_clear() clears bits in a specified range of a bit-vector. The inclusive lower bound @p l
 *  and the inclusive upper bound @p h specify the range. @p l must be equal to or smaller than
 *  @p h and @p h must be smaller than the length of the bit-vector to set.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in,out]    set    bit-vector to set
 *  @param[in]        l      lower bound of range (inclusive)
 *  @param[in]        h      upper bound of range (inclusive)
 *
 *  @return    nothing
 */
void (bitv_clear)(bitv_t *set, size_t l, size_t h)
{
    range(&, +, ~);
}


/*! @brief    complements bits in a bit-vector.
 *
 *  bitv_not() flips bits in a specified range of a bit-vector. The inclusive lower bound @p l
 *  and the inclusive upper bound @p h specify the range. @p l must be equal to or smaller than
 *  @p h and @p h must be smaller than the length of the bit-vector to set.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in,out]    set    bit-vector to set
 *  @param[in]        l      lower bound of range (inclusive)
 *  @param[in]        h      upper bound of range (inclusive)
 *
 *  @return    nothing
 */
void (bitv_not)(bitv_t *set, size_t l, size_t h)
{
    range(^, ~, +);
}


/*! @brief    sets bits in a bit-vector with bit patterns.
 *
 *  bitv_setv() copies bit patterns from an array of bytes to a bit vector. Because only 8 bits in
 *  a byte are used to represent a bit-vector for table-driven approaches, any excess bits are
 *  masked out before copying, which explains why bitv_setv() needs to modify the array, @p v.
 *
 *  Be careful with how to count bit positions in a bit vector. Within a byte, the first bit (the
 *  bit position 0) indicates the least significant bit of a byte and the last bit (the bit
 *  position 7) does the most significant bit. Therefore, the array
 *
 *  @code
 *      { 0x01, 0x02, 0x03, 0x04 }
 *  @endcode
 *
 *  can be used to set bits of a bit-vector as follows:
 *
 *  @code
 *      0        8        16       24     31
 *      |        |        |        |      |
 *      10000000 01000000 11000000 00100000
 *  @endcode
 *
 *  where the bit position is shown on the first line.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set, invalid pointer given for @p v,
 *                    invalid size given for @p n
 *
 *  @param[in,out]    set    bit-vector to set
 *  @param[in,out]    v      bit patterns to use
 *  @param[in]        n      size of @p v in bytes
 *
 *  @return    nothing
 */
void (bitv_setv)(bitv_t *set, unsigned char *v, size_t n)
{
    size_t i;

    assert(set);
    assert(v);
    assert(n > 0);
    assert(n <= nbyte(set->length));

    if (n == nbyte(set->length))
        v[n-1] &= pad[set->length % 8];
    for (i = 0; i < n; i++)
        set->byte[i] = v[i] & 0xFF;
}


/*! @brief    calls a user-provided function for each bit in a bit-vector.
 *
 *  For each bit in a bit-vector, bitv_map() calls a user-provided callback function; it is useful
 *  when doing some common task for each bit. The pointer given in @p cl is passed to the third
 *  parameter of a user callback function, so can be used as a communication channel between the
 *  caller of bitv_map() and the callback. Differently from mapping functions for other data
 *  structures (e.g., tables and sets), changes made in an earlier invocation to @p apply are
 *  visible to later invocations.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in,out]    set      bit-vector with which @p apply called
 *  @param[in]        apply    user-provided function (callback)
 *  @param[in]        cl       passing-by argument to @p apply
 *
 *  @return    nothing
 */
void (bitv_map)(bitv_t *set, void apply(size_t, int, void *), void *cl)
{
    size_t i;

    assert(set);

    for (i = 0; i < set->length; i++)
        apply(i, BIT(set, i), cl);
}


/*! @brief    compares two bit-vectors for equality.
 *
 *  bitv_eq() compares two bit-vectors to check whether they are equal. Two bit-vectors must be of
 *  the same length.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s      bit-vector to compare
 *  @param[in]    t      bit-vector to compare
 *
 *  @return    whether or not two bit-vectors compare equal
 *
 *  @retval    0    not equal
 *  @retval    1    equal
 */
int (bitv_eq)(const bitv_t *s, const bitv_t *t)
{
    size_t i;

    assert(s);
    assert(t);
    assert(s->length == t->length);

    for (i = 0; i < nword(s->length); i++)
        if (s->word[i] != t->word[i])
            return 0;
    return 1;
}


/*! @brief    compares two bit-vectors for subset.
 *
 *  bitv_leq() compares two bit-vectors to check whether a bit-vector is a subset of the other;
 *  note that two bit-vectors have a subset relationship for each other when they compare equal.
 *  Two bit-vectors must be of the same length.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s      bit-vector to compare
 *  @param[in]    t      bit-vector to compare
 *
 *  @return    whether @p s is a subset of @p t
 *
 *  @retval    0    @p s is not a subset of @p t
 *  @retval    1    @p s is a subset of @p t
 */
int (bitv_leq)(const bitv_t *s, const bitv_t *t)
{
    size_t i;

    assert(s);
    assert(t);
    assert(s->length == t->length);

    for (i = 0; i < nword(s->length); i++)
        if ((s->word[i] & ~t->word[i]) != 0)
            return 0;
    return 1;
}


/*! @brief    compares two bit-vectors for proper subset.
 *
 *  bitv_lt() compares two bit-vectors to check whether a bit-vector is a proper subset of the
 *  other. Two bit-vectors must be of the same length.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s      bit-vector to compare
 *  @param[in]    t      bit-vector to compare
 *
 *  @return    whether @p s is a proper subset of @p t
 *
 *  @retval    0    @p s is not a proper subset of @p t
 *  @retval    1    @p s is a proper subset of @p t
 */
int (bitv_lt)(const bitv_t *s, const bitv_t *t)
{
    size_t i;
    int lt = 0;

    assert(s);
    assert(t);
    assert(s->length == t->length);

    for (i = 0; i < nword(s->length); i++)
        if ((s->word[i] & ~t->word[i]) != 0)
            return 0;
        else if (s->word[i] != t->word[i])
            lt = 1;
    return lt;
}


/*! @brief    returns a union of two bit-vectors.
 *
 *  bitv_union() creates a union of two given bit-vectors of the same length and returns it. One of
 *  those may be a null pointer, in which case it is considered an empty (all-cleared) bit-vector.
 *  bitv_union() constitutes a distinct bit-vector from its operands as a result, which means it
 *  always allocates storage for its results even when one of the operands is empty. This property
 *  can be used to make a copy of a bit-vector as follows:
 *
 *  @code
 *      bitv_t *v *w;
 *      v = bitv_new(n);
 *      ...
 *      w = bitv_union(v, NULL);
 *  @endcode
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s    operand of union operation
 *  @param[in]    t    operand of union operation
 *
 *  @return    union of bit-vectors
 */
bitv_t *(bitv_union)(const bitv_t *t, const bitv_t *s)
{
    setop(copy(t), copy(t), copy(s), |);
}


/*! @brief    returns an intersection of two bit-vectors.
 *
 *  bitv_inter() creates an intersection of two bit-vectors of the same length and returns it. One
 *  of those may be a null pointer, in which case it is considered an empty (all-cleared)
 *  bit-vector. bitv_inter() constitutes a distinct bit-vector from its operands as a result, which
 *  means it always allocates storage for its result even when one of the operands is empty.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s    operand of intersection operation
 *  @param[in]    t    operand of intersection operation
 *
 *  @return    intersection of bit-vectors
 */
bitv_t *(bitv_inter)(const bitv_t *t, const bitv_t *s)
{
    setop(copy(t), bitv_new(t->length), bitv_new(s->length), &);
}


/*! @brief    returns a difference of two bit-vectors.
 *
 *  bitv_minus() returns a difference of two bit-vectors of the same length; the bit in the
 *  resulting bit-vector is set if and only if the corresponding bit in the first operand is set
 *  and the corresponding bit in the second operand is not set. One of those may be a null
 *  pointer, in which case it is considered an empty (all-cleared) bit-vector. bitv_minus()
 *  constitutes a distinct bit-vector from its operands as a result, which means it always
 *  allocates storage for its result even when one of the operands is empty.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s    operand of difference operation
 *  @param[in]    t    operand of difference operation
 *
 *  @return    difference of bit-vectors, @p s - @p t
 */
bitv_t *(bitv_minus)(const bitv_t *t, const bitv_t *s)
{
    setop(bitv_new(s->length), bitv_new(t->length), copy(s), & ~);
}


/*! @brief    returns a symmetric difference of two bit-vectors.
 *
 *  bitv_diff() returns a symmetric difference of two bit-vectors of the same length; the bit in
 *  the resulting bit-vector is set if only one of the corresponding bits in the operands is set.
 *  One of those may be a null pointer, in which case it is considered an empty (all-cleared)
 *  bit-vector. bitv_diff() constitutes a distinct bit-vector from its operands as a result,
 *  which means it always allocates storage for its result even when one of the operands is
 *  empty.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s    operand of difference operation
 *  @param[in]    t    operand of difference operation
 *
 *  @return    symmetric difference of bit-vectors
 */
bitv_t *(bitv_diff)(const bitv_t *t, const bitv_t *s)
{
    setop(bitv_new(s->length), copy(t), copy(s), ^);
}

/* end of bitv.c */
