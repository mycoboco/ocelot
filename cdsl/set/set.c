/*!
 *  @file    set.c
 *  @brief    Source for Set Library (CDSL)
 */

#include <limits.h>    /* INT_MAX */
#include <stddef.h>    /* size_t, NULL */

#include "cbl/memory.h"    /* MEM_ALLOC, MEM_NEW, MEM_FREE */
#include "cbl/assert.h"    /* assert with exception support */
#include "set.h"


#define MAX(x, y) ((x) > (y)? (x): (y))    /*!< returns the larger of two */
#define MIN(x, y) ((x) > (y)? (y): (x))    /*!< returns the smaller of two */


/* @struct    set_t    set.c
 * @brief    implements a set with a hash table.
 *
 * struct @c set_t contains some information on a set and a hash table to contain set members. A
 * set looks like the following:
 *
 * @code
 *     +-----+ -+-
 *     |     |  |  length, timestamp, cmp, hash, size, bucket
 *     |     |  |
 *     |     |  | ------+
 *     +-----+ -+-      | bucket points to the start of hash table
 *     | [ ] | <--------+
 *     | [ ] |
 *     | [ ] |
 *     | [ ] |-[ ]-[ ]  linked list constructed by link member in struct member
 *     | [ ] |
 *     | [ ] |-[ ]-[ ]-[ ]-[ ]
 *     +-----+
 * @endcode
 *
 * The number of buckets in a hash table is determined based on a hint given when set_new() called.
 * When need to find a specific member in a set, functions @c cmp and @c hash are used to locate
 * it. The number of all buckets in a set is necessary when an array is generated to contain all
 * members in a set (see set_toarray()). @c timestamp is increased whenever a set is modified by
 * set_put() or set_remove(). It prevents a user-defined function invoked by set_map() from
 * modifying a set during traversing it.
 *
 * Note that a violation of the alignment restriction is possible because there is no guarantee
 * that the address to which @c bucket points is aligned properly for a pointer to struct
 * @c member, even if such a violation rarely happens in practice. Putting a dummy member of the
 * struct @c member pointer type at the beginning of struct @c set_t can solve the problem.
 *
 * @todo    Improvements are possible and planned:
 *          - some of the set operations can be improved if hash numbers are stored in struct
 *            @c member, which enables a user-provided hashing function called only once for each
 *            member and a user-provided comparison function called only when the hash numbers
 *            differ.
 */
struct set_t {
    /* struct binding *dummy; */                 /* dummy member to solve alignment problem */
    int size;                                    /* number of buckets in hash table described by
                                                    @c bucket */
    int (*cmp)(const void *x, const void *y);    /* comparison function */
    unsigned (*hash)(const void *x);             /* hash generating function */
    size_t length;                               /* number of members in set */
    unsigned timestamp;                          /* number of modification to set */

    /* @struct    member    set.c
     * @brief    implements a hash table to contain members.
     */
    struct member {
        struct member *link;    /* pointer to next node in same bucket */
        const void *member;     /* pointer to member */
    } **bucket;    /* array of struct member, so pointer-to-pointer */
};


/* @brief    default function for comparing members assumed to be hash strings.
 *
 * defhashCmp() assumes two given members to be hash strings and compares them by comparing their
 * addresses for equality (see the Hash Library for details).
 *
 * @warning    Even if defhashCmp() returns either zero or non-zero for equality check, a function
 *             provided to set_new() by a user is supposed to retrun a value less than, equal to
 *             or greater than zero to indicate that @p x is less than, equal to or greater than
 *             @p y, respectively. See set_new() for more explanation.
 *
 * @param[in]    x    member for comparision
 * @param[in]    y    member for comparision
 *
 * @return    whether or not arguments compare equal
 *
 * @retval    0    equal
 * @retval    1    not equal
 */
static int defhashCmp(const void *x, const void *y)
{
    return (x != y);
}


/* @brief    default function for generating a hash value for a member assumed to be a hash string.
 *
 * defhashGen() generates a hash value for a hash string (see the Hash Library for details). It
 * converts a given pointer value to an unsigned integer and cuts off its two low-order bits
 * because they are verly likely to be zero due to the alignement restriction. It returns an
 * integer resulting from converting to a possibly narrow type.
 *
 * @param[in]    x    member for which hash generated
 *
 * @return    hash value generated
 */
static unsigned defhashGen(const void *x)
{
    return (unsigned long)x >> 2;
}


/* @brief    creates a copy of a set.
 *
 * copy() makes a copy of a set by allocating new storage for it. copy() is used for various set
 * operations and since the size of sets resulting from such operations may differ from those of
 * the operand sets it takes a new hint for the copied set. See the set operation functions to see
 * how the hint for copy() is used. In order to avoid set_put()'s fruitless search it directly puts
 * members into a new set.
 *
 * @param[in]    t       set to copy
 * @param[in]    hint    hint for size of hash table
 *
 * @return    set copied
 */
static set_t *copy(set_t *t, int hint)
{
    set_t *set;

    assert(t);

    set = set_new(hint, t->cmp, t->hash);
    {
        int i;
        struct member *q;

        for (i = 0; i < t->size; i++)
            for (q = t->bucket[i]; q; q = q->link) {
                struct member *p;
                const void *member = q->member;
                int i = set->hash(member) % set->size;

                MEM_NEW(p);
                p->member = member;
                p->link = set->bucket[i];
                set->bucket[i] = p;
                set->length++;
            }
    }

    return set;
}


/*! @brief    creates a new set.
 *
 *  set_new() creates a new set. It takes some information on a set it will create:
 *  - @p hint: an estimate for the size of a set;
 *  - @p cmp: a user-provided function for comparing members;
 *  - @p hash: a user-provided function for generating a hash value from a member
 *
 *  set_new() determines the size of the internal hash table kept in a set based on @p hint. It
 *  never restricts the number of members one can put into a set, but a better estimate probably
 *  gives better performance.
 *
 *  A function given to @p cmp should be defined to take two arguments and to return a value less
 *  than, equal to or greater than zero to indicate that the first argument is less than, equal to
 *  or greater than the second argument, respectively.
 *
 *  A function given to @p hash takes a member and returns a hash value that is to be used as an
 *  index for internal hash table in a set. If the @p cmp function returns zero (which means they
 *  are equal) for some two members, the @p hash function must generate the same value for them.
 *
 *  If a null pointer is given for @p cmp or @p hash, the default comparison or hashing function
 *  is used; see defhashCmp() and defhashGen(), in which case members are assumed to be hash
 *  strings generated by the Hash Library. An example follows:
 *
 *  @code
 *      set_t *myset = set_new(hint, NULL, NULL);
 *      ...
 *      set_put(hash_string("member1"));
 *      set_put(hash_string("member2"));
 *      assert(set_member(hash_string("member1")));
 *  @endcode
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: invalid functions for @p cmp and @p hash
 *
 *  @param[in]    hint    hint for size of hash table
 *  @param[in]    cmp     user-defined comparison function
 *  @param[in]    hash    user-defined hash generating function
 *
 *  @return    new set created
 *
 *  @internal
 *
 *  Even if a function given to @p cmp should return a value that can distinguish three different
 *  cases for full generality, the current implementation needs only equal or non-equal conditions.
 *
 *  A complicated way to determine the size of the hash table is hired here since the hash number
 *  is computed by a user-provided callback. Compare this to the Hash Library implementation where
 *  the size has a less sophisticated form and the hash number is computed by the library.
 *
 *  The interface for the Set Library demands more information than a particular implementation of
 *  the interface might require. For example, an implementation using a hash table as given here
 *  need not require the comparison function to distinguish three different cases, and an
 *  implementation using a tree does not need @p hint and the hashing function passed through
 *  @p hash. They are all required in the interface to allow for vairous implementation approaches.
 */
set_t *(set_new)(int hint, int cmp(const void *, const void *), unsigned hash(const void *))
{
    /* candidates for size of hash table; all are primes probably except INT_MAX; max size of hash
       table is limited by INT_MAX */
    static int primes[] = { 509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX };

    int i;
    set_t *set;

    assert(hint >= 0);

    /* i starts with 1; size be largest smaller than hint */
    for (i = 1; primes[i] < hint; i++)    /* no valid range check; hint never larger than INT_MAX */
        continue;

    set = MEM_ALLOC(sizeof(*set) + primes[i-1]*sizeof(set->bucket[0]));
    set->size = primes[i-1];
    set->cmp  = cmp? cmp: defhashCmp;
    set->hash = hash? hash: defhashGen;
    set->length = 0;
    set->timestamp = 0;

    set->bucket = (struct member **)(set + 1);
    for (i = 0; i < set->size; i++)
        set->bucket[i] = NULL;

    return set;
}


/*! @brief    inspects if a set contains a member.
 *
 *  set_member() inspects a set to see if it contains a member.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in]    set       set to inspect
 *  @param[in]    member    member to find in a set
 *
 *  @return    found/not found indicator
 *
 *  @retval    0    member not found
 *  @retval    1    member found
 */
int (set_member)(set_t *set, const void *member)
{
    int i;
    struct member *p;

    assert(set);
    assert(member);

    i = set->hash(member) % set->size;
    for (p = set->bucket[i]; p; p = p->link)
        if (set->cmp(member, p->member) == 0)
            break;

    return (p != NULL);
}


/*! @brief    puts a member to a set.
 *
 *  set_put() inserts a member to a set. If it fails to find a member matched to a given member, it
 *  inserts the member to a set after proper storage allocation. If it finds a matched one, it
 *  replaces an existing member with a new one. Because they has different representations even if
 *  they compare equal by a user-defined comparison function, replacing the old one with the new
 *  one makes sense.
 *
 *  Note that a member is a pointer. If members are, say, integers in an application, objects to
 *  contain them are necessary to put them into a set.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in,out]    set       set to which member inserted
 *  @param[in]        member    member to insert
 *
 *  @return    nothing
 */
void (set_put)(set_t *set, const void *member)
{
    int i;
    struct member *p;

    assert(set);
    assert(member);

    i = set->hash(member) % set->size;

    for (p = set->bucket[i]; p; p = p->link)
        if (set->cmp(member, p->member) == 0)
            break;

    if (!p) {    /* member not found, so allocate new one */
        MEM_NEW(p);
        p->member = member;
        p->link = set->bucket[i];    /* push to hash table */
        set->bucket[i] = p;
        set->length++;
    } else
        p->member = member;
    set->timestamp++;    /* set modified */
}


/*! @brief    removes a member from a set.
 *
 *  set_remove() gets rid of a member from a set. Note that set_remove() does not deallocate any
 *  storage for the member to remove, which must be done by a user.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in,out]    set       set from whcih member removed
 *  @param[in]        member    member to remove
 *
 *  @return    previous member or null pointer
 *
 *  @retval    non-null    previous member
 *  @retval    NULL        member not found
 *
 *  @warning    If the stored member is a null pointer, an ambiguous situation may occur.
 */
void *(set_remove)(set_t *set, const void *member)
{
    int i;
    struct member **pp;    /* pointer-to-pointer idiom used */

    assert(set);
    assert(member);

    set->timestamp++;    /* set modified */

    i = set->hash(member) % set->size;

    for (pp = &set->bucket[i]; *pp; pp = &(*pp)->link)
        if (set->cmp(member, (*pp)->member) == 0) {    /* member found */
            struct member *p = *pp;
            *pp = p->link;
            member = p->member;
            MEM_FREE(p);
            set->length--;
            return (void *)member;    /* remove constness */
        }

    /* member not found */
    return NULL;
}


/*! @brief    returns the length of a set.
 *
 *  set_length() returns the length of a set which is the number of members in a set.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @param[in]    set    set whose length to be returned
 *
 *  @return    length of set
 */
size_t (set_length)(set_t *set)
{
    assert(set);

    return set->length;
}


/*! @brief    destroys a set.
 *
 *  set_free() destroys a set by deallocating the storage for it and set a given pointer to a null
 *  pointer. As always, set_free() does not deallocate any storage for members in the set, which
 *  must be done by a user.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: pointer to foreign data structure given for @p pset
 *
 *  @param[in,out]    pset    pointer to set to destroy
 *
 *  @return    nothing
 */
void (set_free)(set_t **pset)
{
    assert(pset);
    assert(*pset);

    if ((*pset)->length > 0) {    /* at least one member */
        int i;
        struct member *p, *q;
        for (i = 0; i < (*pset)->size; i++)
            for (p = (*pset)->bucket[i]; p; p = q) {
                q = p->link;
                MEM_FREE(p);
            }
    }

    MEM_FREE(*pset);
}


/*! @brief    calls a user-provided function for each member in a set.
 *
 *  For each member in a set, set_map() calls a user-provided callback function; it is useful when
 *  doing some common task for each member. The pointer given in @p cl is passed to the third
 *  parameter of a user callback function, so can be used as a communication channel between the
 *  caller of set_map() and the callback. Differently from table_map(), set_map() gives a member
 *  (which is a pointer value) to apply() rather than a pointer to a member (which is a pointer to
 *  pointer value); compare the type of the @c member parameter of apply() to that of the @c value
 *  parameter of apply() in table_map(). This is very natural since a member plays a role of a
 *  hashing key in a set as a key does in a table. Also note that apply() is not able to modify a
 *  member itself but can touch a value pointed by the member.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @warning    The order in which a user-provided function is called for each member is
 *              unspecified; a set is an unordered collection of members, so this is not a limiting
 *              factor, however.
 *
 *  @param[in,out]    set      set with which @p apply called
 *  @param[in]        apply    user-provided function (callback)
 *  @param[in]        cl       passing-by argument to @p apply
 *
 *  @return    nothing
 *
 *  @internal
 *
 *  @c timestamp is checked in order to prevent a user-provided function from modifying a set by
 *  calling set_put() or set_remove() that can touch the internal information of a set. If those
 *  functions are called in a callback, it results in assertion failure (which may raise
 *  @c assert_exceptfail).
 */
void (set_map)(set_t *set, void apply(const void *member, void *cl), void *cl)
{
    int i;
    unsigned stamp;
    struct member *p;

    assert(set);
    assert(apply);

    stamp = set->timestamp;
    for (i = 0; i < set->size; i++)
        for (p = set->bucket[i]; p; p = p->link) {
            apply(p->member, cl);
            assert(set->timestamp == stamp);
        }
}


/*! @brief    converts a set to an array.
 *
 *  set_toarray() converts members stored in a set to an array. The last element of the constructed
 *  array is assigned by @p end, which is a null pointer in most cases. Do not forget deallocate
 *  the array when it is unnecessary.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p set
 *
 *  @warning    The size of an array generated from an empty set cannot be zero, since there is
 *              always an end-mark value.
 *
 *  @warning    As in set_map(), the order in which an array is created for each member is
 *              unspecified.
 *
 *  @param[in]    set    set for which array generated
 *  @param[in]    end    end-mark to save in last element of array
 *
 *  @return    array generated from set
 */
void **(set_toarray)(set_t *set, void *end)
{
    int i, j;
    void **array;
    struct member *p;

    assert(set);

    array = MEM_ALLOC((set->length + 1) * sizeof(*array));

    j = 0;
    for (i = 0; i < set->size; i++)
        for (p = set->bucket[i]; p; p = p->link)
            array[j++] = (void *)p->member;    /* cast removes constness */
    array[j] = end;

    return array;
}


/*! @brief    returns a union set of two sets.
 *
 *  set_union() creates a union set of two sets and returns it. One of those may be a null pointer,
 *  in which case it is considered an empty set. For every operation, its result constitutes a
 *  distinct set from the operand sets, which means the set operations always allocate storage for
 *  their results even when one of the operands is empty.
 *
 *  Note that the inferface of the Set Library does not assume the concept of a universe, which is
 *  the set of all possible members. Thus, a set operation like the complement is not defined and
 *  not implemented.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @warning    To create a union of two sets, they have to share the same comparison and hashing
 *              functions.
 *
 *  @param[in]    s    operand of set union operation
 *  @param[in]    t    operand of set union operation
 *
 *  @return    union set
 *
 *  @todo    Improvements are possible and planned:
 *           - the code can be modified so that the operation is performed on each pair of
 *             corresponding buckets when two given sets have the same number of buckets.
 */
set_t *(set_union)(set_t *s, set_t *t)
{
    if (!s) {    /* s is empty, t not empty */
        assert(t);
        return copy(t, t->size);
    } else if (!t) {    /* t is empty, s not empty */
        return copy(s, s->size);
    } else {    /* both not empty */
        /* makes copy of s; resulting set has at least as many members as bigger set does */
        set_t *set = copy(s, MAX(s->size, t->size));    /* makes copy of s */

        assert(s->cmp == t->cmp && s->hash == t->hash);    /* same comparison/hashing method */
        {    /* inserts member of t to union set */
            int i;
            struct member *q;
            for (i = 0; i < t->size; i++)
                for (q = t->bucket[i]; q; q = q->link)
                    set_put(set, q->member);
        }

        return set;
    }
}


/*! @brief    returns an intersection of two sets.
 *
 *  set_inter() returns an intersection of two sets, that is a set with only members that both have
 *  in common. See set_union() for more detailed explanation commonly applied to the set operation
 *  functions.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s    operand of set intersection operation
 *  @param[in]    t    operand of set intersection operation
 *
 *  @return    intersection set
 *
 *  @todo    Improvements are possible and planned:
 *           - the code can be modified so that the operation is performed on each pair of
 *             corresponding buckets when two given sets have the same number of buckets.
 */
set_t *(set_inter)(set_t *s, set_t *t)
{
    if (!s) {    /* s is empty, t not empty */
        assert(t);
        return set_new(t->size, t->cmp, t->hash);    /* null set */
    } else if (!t) {    /* t is empty, s not empty */
        return set_new(s->size, s->cmp, s->hash);    /* null set */
    } else if (s->length < t->length) {    /* use of <= here results in infinite recursion */
        /* since smaller t->size means better performance below, makes t->size smaller */
        return set_inter(t, s);
    } else {    /* both not empty */
        /* resulting set has at most as many members as smaller set does */
        set_t *set = set_new(MIN(s->size, t->size), s->cmp, s->hash);

        assert(s->cmp == t->cmp && s->hash == t->hash);
        {    /* pick member in t and put it to result set if s also has it */
            int i;
            struct member *q;
            for (i = 0; i < t->size; i++)    /* smaller t->size, better performance */
                for (q = t->bucket[i]; q; q = q->link)
                    if (set_member(s, q->member)) {
                        struct member *p;
                        const void *member = q->member;
                        int i = set->hash(member) % set->size;

                        MEM_NEW(p);
                        p->member = member;
                        p->link = set->bucket[i];
                        set->bucket[i] = p;
                        set->length++;
                    }
        }

        return set;
    }
}


/*! @brief    returns a difference set of two sets
 *
 *  set_minus() returns a difference of two sets, that is a set with members that @p t has but @p
 *  does not. See set_union() for more detailed explanation commonly applied to the set operation
 *  functions.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s    operand of set difference operation
 *  @param[in]    t    operand of set difference operation
 *
 *  @return    difference set, @p s - @p t
 *
 *  @todo    Improvements are possible and planned:
 *           - the code can be modified so that the operation is performed on each pair of
 *             corresponding buckets when two given sets have the same number of buckets.
 */
set_t *(set_minus)(set_t *s, set_t *t) {
    if (!s) {    /* s is empty, t not empty */
        assert(t);
        return set_new(t->size, t->cmp, t->hash);    /* (null set) - t = (null set) */
    } else if (!t) {    /* t is empty, s not empty */
        return copy(s, s->size);    /* s - (null set) = s */
    } else {    /* both not empty */
        set_t *set = set_new(MIN(s->size, t->size), s->cmp, s->hash);

        assert(s->cmp == t->cmp && s->hash == t->hash);
        {    /* pick member in s and put it to result set if t does not has it */
            int i;
            struct member *q;
            for (i = 0; i < s->size; i++)
                for (q = s->bucket[i]; q; q = q->link)
                    if (!set_member(t, q->member)) {
                        struct member *p;
                        const void *member = q->member;
                        int i = set->hash(member) % set->size;

                        MEM_NEW(p);
                        p->member = member;
                        p->link = set->bucket[i];
                        set->bucket[i] = p;
                        set->length++;
                    }
        }

        return set;
    }
}


/*! @brief    returns a symmetric difference of two sets.
 *
 *  set_diff() returns a symmetric difference of two sets, that is a set with members only one of
 *  two operand sets has. A symmetric difference set is identical to the union set of (@p s - @p t)
 *  and (@p t - @p s). See set_union() for more detailed explanation commonly applied to the set
 *  operation functions.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p s or @p t
 *
 *  @param[in]    s    operand of set difference operation
 *  @param[in]    t    operand of set difference operation
 *
 *  @return    symmetric difference set
 *
 *  @todo    Improvements are possible and planned:
 *           - the code can be modified so that the operation is performed on each pair of
 *             corresponding buckets when two given sets have the same number of buckets.
 */

set_t *(set_diff)(set_t *s, set_t *t)
{
    if (!s) {    /* s is empty, t not empty */
        assert(t);
        return copy(t, t->size);    /* (null set)-t U t-(null set) = (null set) U t = t */
    } else if (!t) {    /* t is empty, s not empty */
        return copy(s, s->size);    /* s-(null set) U (null set)-s = s U (null set) = s */
    } else {    /* both not empty */
        set_t *set = set_new(MIN(s->size, t->size), s->cmp, s->hash);

        assert(s->cmp == t->cmp && s->hash == t->hash);
        {    /* set = t - s */
            int i;
            struct member *q;

            for (i = 0; i < t->size; i++)
                for (q = t->bucket[i]; q; q = q->link)
                    if (!set_member(s, q->member)) {
                        struct member *p;
                        const void *member = q->member;
                        int i = set->hash(member) % set->size;

                        MEM_NEW(p);
                        p->member = member;
                        p->link = set->bucket[i];
                        set->bucket[i] = p;
                        set->length++;
                    }
        }

        {    /* set += (s - t) */
            int i;
            struct member *q;

            for (i = 0; i < s->size; i++)
                for (q = s->bucket[i]; q; q = q->link)
                    if (!set_member(t, q->member)) {
                        struct member *p;
                        const void *member = q->member;
                        int i = set->hash(member) % set->size;

                        MEM_NEW(p);
                        p->member = member;
                        p->link = set->bucket[i];
                        set->bucket[i] = p;
                        set->length++;
                    }
        }

        return set;
    }
}

/* end of set.c */
