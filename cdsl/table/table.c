/*!
 *  @file    table.c
 *  @brief    Source for Table Library (CDSL)
 */

#include <limits.h>    /* INT_MAX */
#include <stddef.h>    /* size_t, NULL */

#include "cbl/memory.h"    /* MEM_ALLOC, MEM_FREE */
#include "cbl/assert.h"    /* assert with exception support */
#include "table.h"


/* @struct    table_t    table.c
 * @brief    implements a table with a hash table.
 *
 * struct @c table_t contains some information on a table and a hash table to contain key-value
 * pairs. A table looks like the following:
 *
 * @code
 *     +-----+ -+-
 *     |     |  |  size, cmp, hash, length, bucket
 *     |     |  |
 *     |     |  | ------+
 *     +-----+ -+-      | bucket points to the start of hash table
 *     | [ ] | <--------+
 *     | [ ] |
 *     | [ ] |
 *     | [ ] |-[ ]-[ ]  linked list constructed by link member in struct binding
 *     | [ ] |
 *     | [ ] |-[ ]-[ ]-[ ]-[ ]
 *     +-----+
 * @endcode
 *
 * The number of buckets in a hash table is determined based on a hint given when table_new()
 * called. When need to find a specific key in a table, functions @c cmp and @c hash are used to
 * locate it. The number of all buckets in a table is necessary when an array is generated to
 * contain all key-value pairs in a table (see table_toarray()). @c timestamp is increased whenever
 * a table is modified by table_put() or table_remove(). It prevents a user-defined function
 * invoked by table_map() from modifying a table during traversing it.
 *
 * Note that a violation of the alignment restriction is possible because there is no guarantee
 * that the address to which @c bucket points is aligned properly for the pointer to struct
 * @c binding, even if such a violation does not happen in practice. Putting a dummy member of the
 * struct @c binding pointer type at the beginning of struct @c table_t can solve the problem.
 */
struct table_t {
    /* struct binding *dummy; */               /* dummy member to solve alignment problem */
    int size;                                  /* number of buckets in hash table described by
                                                  @c bucket */
    int (*cmp)(const void *, const void *);    /* comparison function */
    unsigned (*hash)(const void *);            /* hash generating function */
    size_t length;                             /* number of key-value pairs in table */
    unsigned timestamp;                        /* number of modification to table */

    /* @struct    binding    table.c
     * @brief    implements a hash table to contain key-value pairs.
     */
    struct binding {
        struct binding *link;    /* pointer to next node in same bucket */
        const void *key;         /* pointer to key */
        void *value;             /* pointer to value */
    } **bucket;    /* array of struct binding, so pointer-to-pointer */
};


/* @brief    default function for comparing keys assumed to be hash strings.
 *
 * defhashCmp() assumes two given keys to be hash strings and compares them by comparing their
 * addresses for equality (see the Hash Library for details).
 *
 * @warning    Even if defhashCmp() returns either zero or non-zero for equality check, a function
 *             provided to table_new() by a user is supposed to retrun a value less than, equal to
 *             or greater than zero to indicate that @p x is less than, equal to or greater than
 *             @p y, respectively. See table_new() for more explanation.
 *
 * @param[in]    x    key for comparision
 * @param[in]    y    key for comparision
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


/* @brief    default function for generating a hash value for a key assumed to be a hash string.
 *
 * defhashGen() generates a hash value for a given hash string (see the Hash Library for details).
 * It converts a given pointer value to an unsigned integer and cuts off its two low-order bits
 * because they are verly likely to be zero due to the alignement restriction. It returns the
 * reulsting integer converting to a possibly narrow type.
 *
 * @param[in]    key    key for which hash generated
 *
 * @return    hash value generated
 */
static unsigned defhashGen(const void *key)
{
    return (unsigned long)key >> 2;
}


/*! @brief    creates a new table.
 *
 *  table_new() creates a new table. It takes some information on a table it will create:
 *  - @p hint: an estimate for the size of a table;
 *  - @p cmp: a user-provided function for comparing keys;
 *  - @p hash: a user-provided function for generating a hash value from a key
 *
 *  table_new() determines the size of the internal hash table kept in a table based on @p hint. It
 *  never restricts the number of entries one can put into a table, but a better estimate probably
 *  bring better performance.
 *
 *  A function given to @p cmp should be defined to take two arguments and to return a value less
 *  than, equal to or greater than zero to indicate that the first argument is less than, equal to
 *  or greater than the second argument, respectively.
 *
 *  A function given to @p hash takes a key and returns a hash value that is to be used as an index
 *  for the internal hash table in a table. If the @p cmp function returns zero (which means they
 *  are equal) for some two keys, the @p hash function must generate the same value for them.
 *
 *  If a null pointer is given for @p cmp or @p hash, the default comparison or hashing function
 *  is used; see defhashCmp() and defhashGen(), in which case keys are assumed to be hash strings
 *  generated by the Hash Library. An example is given below:
 *
 *  @code
 *      table_t *mytable = table_new(hint, NULL, NULL);
 *      int *pi, val1, val2;
 *      ...
 *      table_put(hash_string("key1"), &val1);
 *      table_put(hash_string("key2"), &val2);
 *      pi = table_get(hash_string(key1));
 *      assert(pi == &val1);
 *  @endcode
 *
 *  @p hash. They are all required in the interface to allow for vairous implementation approaches.
 *
 *  Possible exceptions: mem_exceptfail
 *
 *  Unchecked errors: invalid functions for @p cmp and @p hash
 *
 *  @param[in]    hint    hint for size of hash table
 *  @param[in]    cmp     user-defined comparison function
 *  @param[in]    hash    user-defined hash generating function
 *
 *  @return    new table created
 *
 *  @internal
 *
 *  Even if a function given to @p cmp should return a value that can distinguish three different
 *  cases for full generality, the current implementation needs only equal or non-equal conditions.
 *
 *  A complicated way to determine the size of the hash table is hired here since the hash number is
 *  computed by a user-provided callback. Compare this to the Hash Library implementation where the
 *  size has a less sophisticated form and the hash number is computed by the library.
 *
 *  The interface for the Table Library demands more information than a particular implementation
 *  of the interface might require. For example, an implementation using a hash table as given here
 *  need not require the comparison function to distinguish three different cases, and an
 *  implementation using a tree does not need @p hint and the hashing function passed through
 *  @p hash. They are all required in the interface to allow for vairous implementation approaches.
 */
table_t *(table_new)(int hint, int cmp(const void *, const void *), unsigned hash(const void *))
{
    /* candidates for size of hash table; all are primes probably except INT_MAX; max size of hash
       table is limited by INT_MAX */
    static int prime[] = { 509, 509, 1021, 2053, 4093, 8191, 16381, 32771, 65521, INT_MAX };

    int i;
    table_t *table;

    assert(hint >= 0);

    /* i starts with 1; size be largest smaller than hint */
    for (i = 1; prime[i] < hint; i++)    /* no valid range check; hint never larger than INT_MAX */
        continue;

    table = MEM_ALLOC(sizeof(*table) + prime[i-1]*sizeof(table->bucket[0]));
    table->size = prime[i-1];
    table->cmp = cmp? cmp: defhashCmp;
    table->hash = hash? hash: defhashGen;
    table->length = 0;
    table->timestamp = 0;

    table->bucket = (struct binding **)(table + 1);
    for (i = 0; i < table->size; i++)
        table->bucket[i] = NULL;

    return table;
}


/*! @brief    gets data for a key from a table.
 *
 *  table_get() returns a value associated with a given key.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p table
 *
 *  @param[in]    table    table in which key-value pair to be found
 *  @param[in]    key      key for which value to be returned
 *
 *  @return    value for given key or null pointer
 *
 *  @retval    non-null    value found
 *  @retval    NULL        value not found
 *
 *  @warning    If the stored value was a null pointer, an ambiguous situation may occur.
 */
void *(table_get)(const table_t *table, const void *key)
{
    int i;
    struct binding *p;

    assert(table);
    assert(key);

    i = table->hash(key) % table->size;
    for (p = table->bucket[i]; p; p = p->link)
        if (table->cmp(key, p->key) == 0)    /* same key found */
            break;

    return p? p->value: NULL;
}


/*! @brief    puts a value for a key to a table.
 *
 *  table_put() replaces an existing value for a given key with given one and returns the previous
 *  value. If there is no existing one, the given value is saved for the key and returns a null
 *  pointer.
 *
 *  Note that both a key and a value are pointers. If values are, say, integers in an application,
 *  objects to contain them are necessary to put them into a table.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p table
 *
 *  @param[in,out]    table    table to which value to be stored
 *  @param[in]        key      key for which value to be stored
 *  @param[in]        value    value to store to table
 *
 *  @return    previous value for key or null pointer
 *
 *  @retval    non-null    previous value
 *  @retval    NULL        no previous value found
 *
 *  @warning    If the stored value was a null pointer, an ambiguous situation may occur.
 */
void *(table_put)(table_t *table, const void *key, void *value)
{
    int i;
    struct binding *p;
    void *prev;

    assert(table);
    assert(key);

    i = table->hash(key) % table->size;

    for (p = table->bucket[i]; p; p = p->link)
        if ((*table->cmp)(key, p->key) == 0)
            break;

    if (!p) {    /* key not found, so allocate new one */
        MEM_NEW(p);
        p->key = key;
        p->link = table->bucket[i];    /* push to hash table */
        table->bucket[i] = p;
        table->length++;
        prev = NULL;    /* no previous value */
    } else
        prev = p->value;

    p->value = value;
    table->timestamp++;    /* table modified */

    return prev;
}


/*! @brief    returns the length of a table.
 *
 *  table_length() returns the length of a table which is the number of all key-value pairs in a
 *  table.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p table
 *
 *  @param[in]    table    table whose length returned
 *
 *  @return    length of table
 */
size_t (table_length)(const table_t *table)
{
    assert(table);

    return table->length;
}


/*! @brief    calls a user-provided function for each key-value pair in a table.
 *
 *  For each key-value pair in a table, table_map() calls a user-provided callback function; it is
 *  useful when doing some common task for each node. The pointer given in @p cl is passed to the
 *  third parameter of a user callback function, so can be used as a communication channel between
 *  the caller of table_map() and the callback. Since the callback has the address of @c value (as
 *  opposed to the value of @c value) through the second parameter, it is free to change its
 *  content. A macro like LIST_FOREACH() provided in the List Library is not provided for a table.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p table
 *
 *  @warning    The order in which a user-provided function is called for each key-value pair is
 *              unspecified.
 *
 *  @param[in,out]    table    table with which @p apply called
 *  @param[in]        apply    user-provided function (callback)
 *  @param[in]        cl       passing-by argument to @p apply
 *
 *  @return    nothing
 *
 *  @todo    Improvements are possible and planned:
 *           - it sometimes serves better to call a user callback for key-value pairs in order they
 *             are stored.
 *
 *  @internal
 *
 *  @c timestamp is checked in order to prevent a user-provided function from modifying a table by
 *  calling table_put() or table_remove() that can touch the internal information of a table. If
 *  those functions are called in a callback, it results in assertion failure (which may raise
 *  @c assert_exceptfail).
 */
void (table_map)(table_t *table, void apply(const void *key, void **value, void *cl), void *cl)
{
    int i;
    unsigned stamp;
    struct binding *p;

    assert(table);
    assert(apply);

    stamp = table->timestamp;
    for (i = 0; i < table->size; i++)
        for (p = table->bucket[i]; p; p = p->link) {
            apply(p->key, &p->value, cl);
            assert(table->timestamp == stamp);
        }
}


/*! @brief    removes a key-value pair from a table.
 *
 *  table_remove() gets rid of a key-value pair for a given key from a table. Note that
 *  table_remove() does not deallocate any storage for the pair to remove, which must be done by an
 *  user.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p table
 *
 *  @param[in,out]    table    table from which key-value pair removed
 *  @param[in]        key      key for which key-value pair removed
 *
 *  @return    previous value for given key or null pointer
 *
 *  @retval    non-null    previous value for key
 *  @retval    NULL        key not found
 *
 *  @warning    If the stored value is a null pointer, an ambiguous situation may occur.
 */
void *(table_remove)(table_t *table, const void *key)
{
    int i;
    struct binding **pp;    /* pointer-to-pointer idiom used */

    assert(table);
    assert(key);

    table->timestamp++;    /* table modified */

    i = table->hash(key) % table->size;

    for (pp = &table->bucket[i]; *pp; pp = &(*pp)->link)
        if (table->cmp(key, (*pp)->key) == 0) {    /* key found */
            struct binding *p = *pp;
            void *value = p->value;
            *pp = p->link;
            MEM_FREE(p);
            table->length--;
            return value;
        }

    /* key not found */
    return NULL;
}


/*! @brief    converts a table to an array.
 *
 *  table_toarray() converts key-value pairs stored in a table to an array. The last element of the
 *  constructed array is assigned by @p end, which is a null pointer in most cases. Do not forget
 *  deallocate the array when it is unnecessary.
 *
 *  The resulting array is consisted of key-value pairs: its elements with even indices have keys
 *  and those with odd indices have corresponding values. For example, the second element of a
 *  resulting array has a value corresponding to a key stored in the first element. Note that the
 *  end-marker given as @p end has no corresponding value element.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p table
 *
 *  @warning    The size of an array generated from an empty table is not zero, since there is
 *              always an end-mark.
 *
 *  @warning    As in table_map(), the order in which an array is created for each key-value pair is
 *              unspecified.
 *
 *  @param[in]    table    table for which array generated
 *  @param[in]    end      end-mark to save in last element of array
 *
 *  @return    array generated from table
 *
 *  @todo    Improvements are possible and planned:
 *           - it sometimes serves better to call a user callback for key-value pairs in order they
 *             are stored.
 */
void **(table_toarray)(const table_t *table, void *end)
{
    int i, j;
    void **array;
    struct binding *p;

    assert(table);

    array = MEM_ALLOC((2*table->length + 1) * sizeof(*array));

    j = 0;
    for (i = 0; i < table->size; i++)
        for (p = table->bucket[i]; p; p = p->link) {
            array[j++] = (void *)p->key;    /* cast removes constness */
            array[j++] = p->value;
        }
    array[j] = end;

    return array;
}


/*! @brief    destroys a table.
 *
 *  table_free() destroys a table by deallocating the stroage for it and set a given pointer to the
 *  null pointer. As always, table_free() does not deallocate storages for values in the table,
 *  which must be done by a user.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: pointer to foreign data structure given for @p ptable
 *
 *  @param[in,out]    ptable    pointer to table to destroy
 *
 *  @return    nothing
 */
void (table_free)(table_t **ptable)
{
    assert(ptable);
    assert(*ptable);

    if ((*ptable)->length > 0) {    /* at least one pair */
        int i;
        struct binding *p, *q;
        for (i = 0; i < (*ptable)->size; i++)
            for (p = (*ptable)->bucket[i]; p; p = q) {
                q = p->link;
                MEM_FREE(p);
            }
    }

    MEM_FREE(*ptable);
}

/* end of table.c */
