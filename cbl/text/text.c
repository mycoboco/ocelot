/*!
 *  @file    text.c
 *  @brief    Source for Text Library (CBL)
 */

#include <stddef.h>    /* NULL */
#include <string.h>    /* memcmp, memcpy, memchr */
#include <limits.h>    /* UCHAR_MAX */

#include "cbl/assert.h"    /* assert with exception support */
#include "cbl/memory.h"    /* MEM_NEW, MEM_ALLOC, MEM_FREE */

#include "text.h"


/* array index converted from text position */
#define IDX(i, len) (((i) <= 0)? (i) + (len): (i) - 1)

/* @brief    checks if a text can be attached to another text in the text space.
 *
 * The Text Library hires a separate allocator to maintain its text space where storages for texts
 * resides; see alloc(). If a text is generated in the text space recently and thus it occupies the
 * last byte in the storage, an operation like concatenating another text to it gets easier by
 * simply putting that text immediately after it in the space. ISATEND() called "is at end" checks
 * if such a simple operation is possible for text_dup() and text_cat(). It sees both if the next
 * byte of that ending a given text starts the available area in the text space and if there is
 * enough space to contain a text to be attached.
 *
 * Not all texts reside in the text space. For example, text_box() provides a way to "box" a string
 * literal to construct its text representation. Even in that case, ISATEND() does not invoke
 * undefined behavior thanks to the short-circuit logical operator.
 *
 * The code given here is revised from the original one to avoid generating an invalid pointer
 * value; the original code adds @p n to the @c avail member of @c current before comparing the
 * result to the @c limit member, whichs renders the code invalid when @p n is bigger than the
 * remained free space. The same modification applies to alloc().
 */
#define ISATEND(s, n) ((s).str+(s).len == current->avail && (n) <= current->limit-current->avail)

/* compares a sub-text to another text up to the length of the latter */
#define EQUAL(s, i, t) (memcmp(&(s).str[i], (t).str, (t).len) == 0)

/* swaps two integers */
#define SWAP(i, j) do {              \
                       int t = i;    \
                       i = j;        \
                       j = t;        \
                   } while(0)


/* @struct    text_save_t    text.c
 * @brief    implements a save point for the text space.
 *
 * The Text Library provides a limited control over the text space that is storage allocated by the
 * library. The text space can be seen as a stack and struct @c text_save_t provides a way to
 * remember the top of the stack and to restore it later releasing every storage allocated after
 * the top has been remembered. Comparing struct @c text_save_t with struct @c chunk, note that the
 * member @c limit is not necessary to remember since it is automatically recovered with
 * @c current, but @c avail should be remembered since it is mutable.
 */
struct text_save_t {
    struct chunk *current;    /*!< points to chunk to restore later */
    char *avail;              /*!< points to start of free area of saved chunk */
};

/* @struct    chunk    text.c
 * @brief    implements the string space.
 *
 * The Text Library uses its own storage allocator for:
 * - it has to support text_save() and text_restore(), and
 * - it enables some optimization as seen in text_dup() and text_cat().
 *
 * The text space looks like a single list of chunks in the Arena Library. It can be even simpler,
 * however, since there is no need to keep a header information for the space and no need to worry
 * about the alignment problem; the size of the allocated space is maintained by @c text_t and the
 * only type involved with the text space is a character type which has the least alignement
 * restriction.
 *
 * The text space starts from the head node whose sole purpose is to let its @c link member point
 * to the first chunk allocated if any. Within a chunk, @c avail points to the beginning of the
 * free space and @c limit points to past the end of that chunk. With these two pointers, one can
 * compute the size of the free space in a chunk.
 */
struct chunk {
    struct chunk *link;    /*!< points to next chunk */
    char *avail;           /*!< points to start of free area in chunk */
    char *limit;           /*!< points to past end of chunk */
};


/* @brief    heads the list of memory chunks for the text space; see alloc().
 */
static struct chunk head = { NULL, NULL, NULL };

/* @brief    points to the last chunk in the list for the text space; see alloc().
 */
static struct chunk *current = &head;

/* @brief    represents a text for uppercase alphabetical characters; see text_map().
 */
const text_t text_ucase = { 26, "ABCDEFGHIJKLMNOPQRSTUVWXYZ" };

/* @brief    represents a text for lowercase alphabetical characters; see text_map().
 */
const text_t text_lcase = { 26, "abcdefghijklmnopqrstuvwxyz" };

/* @brief    represents a text for decimal digits.
 */
const text_t text_digits = { 10, "0123456789" };

/* @brief    represents an empty text.
 */
const text_t text_null = { 0, "" };


/* @brief    allocates storage in the text space.
 *
 * alloc() allocates storage in the text space. The structure of alloc() is quite simple since, as
 * explained in struct @c chunk, it has no header information to maintain and no alignment problem
 * to handle. Note that the zero size for storage is permitted in order to allow for an empty text.
 *
 * The code given here differs from the original one in three places:
 * - it checks if the @c limit or @c avail member of @c current is a null pointer; the original
 *   code in the book did not, which results in undefined behavior, and
 * - an operation adding an integer to a pointer for comparison was revised to that subtracting a
 *   pointer from another pointer (see ISATEND()), and
 * - a composite assignment expression was decomposed to two separate assignment expressions to
 *   avoid undefined behavior.
 *
 * The last item deserves more explanation. The original expression:
 *
 * @code
 *     current = current->link = MEM_ALLOC(sizeof(*current) + 10*1024 + len);
 * @endcode
 *
 * modifies an object @c current which is also referenced between two sequence points. Even if it
 * is not crystal-clear whether or not the reference to @c current is necessary to determine a new
 * value to store into @c current, the literal reading of the standard says undefined behavior
 * about it. Thus, it seems clever to avoid it.
 *
 * @param[in]    len    size of storage to allocate
 *
 * @return    pointer to storage allocated
 */
static char *alloc(int len)
{
    assert(len >= 0);

    if (!current->avail || len > current->limit - current->avail) {
        /* new chunk added to list */
        current->link = MEM_ALLOC(sizeof(*current) + 10*1024 + len);    /* extra 10Kb */
        current = current->link;
        current->avail = (char *)(current + 1);
        current->limit = current->avail + 10*1024 + len;
        current->link = NULL;
    }
    current->avail += len;

    return current->avail - len;
}


/*! @brief    normalizes a text position.
 *
 *  A text position may be negative and it is often necessary to normalize it into the positive
 *  range. text_pos() takes a text position and adjusts it to the positive range. For example, given
 *  a text:
 *
 *  @code
 *      1  2  3  4  5  (positive positions)
 *        t  e  s  t
 *      -4 -3 -2 -1 0  (non-positive positions)
 *        0  1  2  3   (array indices)
 *  @endcode
 *
 *  both text_pos(t, 2) and text_pos(t, -3) give 2.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s
 *
 *  @param[in]    s    string for which position is to be normalized
 *  @param[in]    i    position to normalize
 *
 *  @return    nomalized positive position
 *
 *  @internal
 *
 *  Note that the relational operator given in the last call to assert() is @c <=, not @c < even if
 *  the element referred to by s.str[s.len] does not exist. Because that index is converted from a
 *  valid position for a text, it should not be precluded. For example, the valid position 5 in the
 *  above example is converted to 4 by IDX(). The validity check itself causes no problem because
 *  the non-existing element is never accessed, and a further restrictive check is performed when
 *  there is a chance to access it.
 */
int (text_pos)(text_t s, int i)
{
    assert(s.len >= 0 && s.str);    /* validity check for text */

    i = IDX(i, s.len);    /* converts to index for array */

    assert(i >= 0 && i <= s.len);    /* validity check for position */

    return i + 1;    /* positive position */
}


/*! @brief    boxes a null-terminated string to construct a text.
 *
 *  text_box() "boxes" a constant string or a string whose storage is already allocated properly by
 *  a user. Unlike text_put(), text_box() does not copy a given string and the length of a text is
 *  granted by a user. text_box() is useful especially when constructing a text representation for
 *  a string literal:
 *
 *  @code
 *      text_t t = text_box("sample", 6);
 *  @endcode
 *
 *  Note, in the above example, that the terminating null character is excluded by the length given
 *  to text_box(). If a user gives 7 for the length, the resulting text includes a null character,
 *  which constructs a different text from what the above call makes.
 *
 *  An empty text whose length is 0 is allowed. It can be constructed simply as in the following
 *  example:
 *
 *  @code
 *      text_t empty = text_box("", 0);
 *  @endcode
 *
 *  and a predefined empty text, @c text_null is also provided for convenience.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid string or length given for @p str or @p len
 *
 *  @param[in]    str    string to box for text representation
 *  @param[in]    len    length of string to box
 *
 *  @return    text containing given string
 */
text_t (text_box)(const char *str, int len)
{
    text_t text;

    assert(str);
    assert(len >= 0);

    text.str = str;
    text.len = len;

    return text;
}


/*! @brief    constructs a sub-text of a text.
 *
 *  text_sub() constructs a sub-text from characters between two specified positions in a text.
 *  Positions in a text are specified as in the Doubly-Linked List Library:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        s  a  m  p  l  e
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  Given the above text, a sub-string @c amp can be specified as [2:5], [2:-2], [-5:5] or [-5:-2].
 *  Furthermore, the order in which the positions are given does not matter, which means [5:2]
 *  indicates the same sequence of characters as [2:5]. In conclusion, the following calls to
 *  text_sub() gives the same sub-text.
 *
 *  @code
 *      text_sub(t, 2, 5);
 *      text_sub(t, -5: 5);
 *      text_sub(t, -2: -5);
 *      text_sub(t, 2: -2);
 *  @endcode
 *
 *  Since a user is not allowed to modify the resulting text and it need not end with a null
 *  character, text_sub() does not have to allocate storage for the result.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s
 *
 *  @warning    Do not assume that the resulting text always share the same storage as the original
 *              text. An implementation might change not to guarantee it, and there is already an
 *              exception to that assumption - when text_sub() returns an empty text.
 *
 *  @param[in]    s    text from which sub-text to be constructed
 *  @param[in]    i    position for sub-text
 *  @param[in]    j    position for sub-text
 *
 *  @return    sub-text constructed
 */
text_t (text_sub)(text_t s, int i, int j)
{
    text_t text;

    assert(s.len >= 0 && s.str);    /* validity check for text */

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    text.len = j - i;
    text.str = s.str + i;

    return text;
}


/*! @brief    constructs a text from a null-terminated string.
 *
 *  text_put() copies a null-terminated string to the text space and returns a text representing the
 *  copied string. The resulting text does not contain the terminating null character. Because it
 *  always copies a given string, the storage for the original string can be safely released after a
 *  text for it has been generated.
 *
 *  Possible exceptions: assert_exceptfail, memory_exceptfail
 *
 *  Unchecked errors: invalid string given for @p str
 *
 *  @param[in]    str    null terminated string to copy for text representation
 *
 *  @return    text containing given string
 */
text_t (text_put)(const char *str)
{
    text_t text;

    assert(str);

    text.len = strlen(str);    /* note that null character not counted */
    text.str = memcpy(alloc(text.len), str, text.len);

    return text;
}


/*! @brief    constructs a text from an array of characters.
 *
 *  text_gen() copies @c size characters from @c str to the text space and returns a text
 *  representing the copied characters. The terminating null character is considered an ordinary
 *  character if any. Because it always copies given characters, the storage for the original array
 *  can be safely released after a text for it has been generated.
 *
 *  text_gen() is useful when a caller wants to construct a text that embodies the terminating null
 *  character with allocating storage for it. text_put() allocates storage but always precludes the
 *  null character, and text_box() can make the resulting text embody the null character but
 *  allocates no storage. text_gen() is added to fill the gap.
 *
 *  Possible exceptions: assert_exceptfail, memory_exceptfail
 *
 *  Unchecked errors: invalid string given for @p str, invalid size given for @p size
 *
 *  @param[in]    str     null terminated string to copy for text representation
 *  @param[in]    size    length of string
 *
 *  @return    text containing given string
 */
text_t (text_gen)(const char str[], int size)
{
    text_t text;

    assert(str);

    text.len = size;
    text.str = memcpy(alloc(text.len), str, text.len);

    return text;
}


/*! @brief    converts a text to a C string.
 *
 *  text_get() is used when converting a text to a C string that is null-terminated. There are two
 *  ways to provide a buffer into which the resulting C string is to be written. If @p str is not
 *  a null pointer, text_get() assumes that a user provides the buffer whose size is @p size, and
 *  tries to write the conversion result to it. If its specified size is not enough to contain the
 *  result, it raises an exception due to assertion failure. If @p str is a null pointer, @c size
 *  is ignored and text_get() allocates a proper buffer to contain the reulsting string. The Text
 *  Library never deallocates the buffer allocated by text_ger(), thus a user has to set it free
 *  when it is no longer necessary.
 *
 *  Possible exceptions: assert_exceptfail, memory_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s, invalid buffer or size given for @p str or
 *                    @p size
 *
 *  @param[out]    str     buffer into which converted string to be written
 *  @param[in]     size    size of given buffer
 *  @param[in]     s       text to convert to C string
 *
 *  @return    pointer to buffer containing C string
 */
char *(text_get)(char *str, int size, text_t s)
{
    assert(s.len >= 0 && s.str);

    if (str == NULL)
        str = MEM_ALLOC(s.len + 1);    /* +1 for null character */
    else
        assert(size >= s.len + 1);

    memcpy(str, s.str, s.len);
    str[s.len] = '\0';

    return str;
}


/*! @brief    constructs a text by duplicating another text.
 *
 *  text_dup() takes a text and constructs a text that duplicates the original text @p n times. For
 *  example, the following call
 *
 *  @code
 *      text_dup(text_box("sample", 6), 3);
 *  @endcode
 *
 *  constructs as the result a text: @c samplesamplesample
 *
 *  Possible exceptions: assert_exceptfail, memory_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s
 *
 *  @warning    Note that text_dup() does not change a given text, but creates a new text that
 *              duplicates a given text. Do not forget, in this library, a text is immutable.
 *
 *  @param[in]    s    text to duplicate
 *  @param[in]    n    number of duplication
 *
 *  @return    text duplicated
 *
 *  @internal
 *
 *  In this implementation, there are two special cases where no real duplication need not occur:
 *  @p n or the length of a text is 0, and @p n is 1. Even when duplication is necessary, there is a
 *  case where the original and the resulting texts share the storage. ISATEND() detects such a
 *  case.
 */
text_t (text_dup)(text_t s, int n)
{
    assert(s.len >= 0 && s.str);    /* validity check for text */
    assert(n >= 0);

    if (n == 0 || s.len == 0)
        return text_null;

    if (n == 1)    /* no need to duplicate */
        return s;

    {    /* s.len > 0 && n > 2 */
        text_t text;
        char *p;

        text.len = n*s.len;
        if (ISATEND(s, text.len - s.len)) {    /* possible to duplicate n-1 times and attach */
            text.str = s.str;    /* share storage */
            p = alloc(text.len - s.len);    /* allocates (n-1)*s.len bytes */
            n--;
        } else
            text.str = p = alloc(text.len);
        for (; n-- > 0; p += s.len)
            memcpy(p, s.str, s.len);

        return text;
    }
}


/*! @brief    constructs a text by concatenating two texts.
 *
 *  text_cat() constructs a new text by concatenating @p s2 to @p s1.
 *
 *  Possible exceptions: assert_exceptfail, memory_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s1 or @p s2
 *
 *  @warning    Unlike strcat() in the standard library, text_cat() does not change a given text by
 *              concatenation, but creates a new text by concatenating @p s2 to @p s1, which means
 *              only the returned text has the concatenated result.
 *
 *  @param[in]    s1    text to which another text is to be concatenated
 *  @param[in]    s2    text to concatenate
 *
 *  @return    concatenated text
 *
 *  @internal
 *
 *  As in text_dup(), there are several cases where some optimization is possible:
 *  - either of two texts is empty, text_cat() can simply return the other,
 *  - if two texts already exist adjacently in the text space, no need to concatenate them, and
 *  - if a text resides at the end of the text space, copying the other there can construct the
 *    result by sharing the storage; ISATEND() detects such a case.
 *
 *  According to the strict interpretation of the C standard, this code might invoke undefined
 *  behavior. If two texts that do not reside in the text space are incidentally adjacent,
 *  text_cat() returns a text representation containing the address of the preceding text, but the
 *  authoritative interpretation of the standard does not guarantee walking through beyond the end
 *  of the preceding text.
 */
text_t (text_cat)(text_t s1, text_t s2)
{
    assert(s1.len >= 0 && s1.str);    /* validity checks for text */
    assert(s2.len >= 0 && s2.str);

    if (s1.len == 0)
        return s2;

    if (s2.len == 0)
        return s1;

    if (s1.str + s1.len == s2.str) {    /* already adjacent texts */
        s1.len += s2.len;
        return s1;    /* possible undefined behavior */
    }

    {    /* s1.len > 0 && s2.len > 0 */
        text_t text;

        text.len = s1.len + s2.len;

        if (ISATEND(s1, s2.len)) {
            text.str = s1.str;
            memcpy(alloc(s2.len), s2.str, s2.len);
        } else {
            char *p;
            text.str = p = alloc(s1.len + s2.len);
            memcpy(p, s1.str, s1.len);
            memcpy(p + s1.len, s2.str, s2.len);
        }

        return text;
    }
}


/*! @brief    constructs a text by reversing a text.
 *
 *  text_reverse() constructs a text by reversing a given text.
 *
 *  Possible exceptions: assert_exceptfail, memory_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s
 *
 *  @warning    text_reverse() does not change a given text, but creates a new text by reversing a
 *              given text.
 *
 *  @param[in]    s    text to reverse
 *
 *  @return    reversed text
 *
 *  @internal
 *
 *  Two obvious cases where some optimization is possible are that a given text has no or only one
 *  character. Considering characters that signed char cannot represent on an implementation where
 *  "plain" char is signed, casts to unsigned char * are added.
 */
text_t (text_reverse)(text_t s)
{
    assert(s.len >= 0 && s.str);    /* validity check for text */

    if (s.len == 0)
        return text_null;
    else if (s.len == 1)
        return s;
    else {    /* s.len > 1 */
        text_t text;
        unsigned char *p;
        int i;

        i = text.len = s.len;
        p = (unsigned char *)(text.str = alloc(s.len));
        while (--i >= 0)    /* avoids i-- > 0 to use s.str[i] */
            *p++ = ((unsigned char *)s.str)[i];

        return text;
    }
}


/*! @brief    constructs a text by converting a text based on a specified mapping.
 *
 *  text_map() converts a text based on a mapping that is described by two pointers to texts. Both
 *  pointers to describe a mapping should be a null pointers or non-null pointers; it is not
 *  allowed for only one of them to be a null pointer.
 *
 *  When they are non-null, they should point to texts whose lengths equal. text_map() takes a text
 *  and copies it converting any occurrence of characters in a text pointed by @p from to
 *  corresponding characters in a text pointed by @p to, where the corresponding characters are
 *  determined by their positions in a text. Ohter characters are copied unchagned.
 *
 *  Once a mapping is set by calling text_map() with non-null text pointers, text_map() can be
 *  called with a null pointers for @p from and @p to, in which case the latest mapping is used
 *  for conversion. Calling with a null pointers is highly recommended whenever possible, since
 *  constructing a mapping table from two texts costs time.
 *
 *  For example, after the following call:
 *
 *  @code
 *      result = text_map(t, &text_upper, &text_lower);
 *  @endcode
 *
 *  @c result is a text copied from @c t converting any uppercase letters in it to corresponding
 *  lowercase letters.
 *
 *  Possible exceptions: assert_exceptfail, memory_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s, @p from or @p to
 *
 *  @param[in]    s       text to convert
 *  @param[in]    from    pointer to text describing mapping
 *  @param[in]    to      pointer to text describing mapping
 *
 *  @return    converted text
 *
 *  @internal
 *
 *  Considering characters that signed char cannot represent and an implementation where "plain"
 *  char is signed, casts to unsigned char * are added.
 */
text_t (text_map)(text_t s, const text_t *from, const text_t *to)
{
    static int inited = 0;    /* indicates if mapping is set */
    static unsigned char map[UCHAR_MAX];

    assert(s.len >= 0 && s.str);    /* validity check for text */

    if (from && to) {    /* new mapping given */
        int k;

        for (k = 0; k < (int)sizeof(map); k++)
            map[k] = k;
        assert(from->len == to->len);    /* validity check for mapping text */
        for (k = 0; k < from->len; k++)
            map[((unsigned char *)from->str)[k]] = ((unsigned char *)to->str)[k];
        inited = 1;
    } else {    /* uses preset mapping */
        assert(from == NULL && to == NULL);
        assert(inited);
    }

    if (s.len == 0)
        return text_null;
    else {
        text_t text;
        int i;
        unsigned char *p;

        text.len = s.len;
        p = (unsigned char *)(text.str = alloc(s.len));
        for (i = 0; i < s.len; i++)
            *p++ = map[((unsigned char *)s.str)[i]];

        return text;
    }
}


/*! @brief    compares two texts.
 *
 *  text_cmp() compares two texts as strcmp() does strings except that a null character is not
 *  treated specially by the former.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s1 or @p s2
 *
 *  @param[in]    s1    text to compare
 *  @param[in]    s2    text to compare
 *
 *  @return    comparison result
 *  @retval    negative    @p s1 compares less than @p s2
 *  @retval    0           @p s1 compares equal to @p s2
 *  @retval    positive    @p s1 compares larger than @p s2
 */
int (text_cmp)(text_t s1, text_t s2)
{
    assert(s1.len >= 0 && s1.str);    /* validity checks for texts */
    assert(s2.len >= 0 && s2.str);

    /* if texts share the storage,
           if s1 is longer than s2, return positive
           if s1 and s2 are of the same length, return 0
           if s1 is shorter than s2, return negative
       if texts do not share the storage,
           if the lengths differ,
               compare texts up to the length of shorter text
               if compare equal
                   return negative or positive according to which text is longer
               if compare unequal
                   return the result of memcmp()
           if the lengths equal,
               return the result of memcmp() */

    if (s1.str == s2.str)
        return s1.len - s2.len;
    else if (s1.len < s2.len) {
        int cond = memcmp(s1.str, s2.str, s1.len);
        return (cond == 0)? -1 : cond;
    } else if (s1.len > s2.len) {
        int cond = memcmp(s1.str, s2.str, s2.len);
        return (cond == 0)? +1 : cond;
    } else    /* s1.len == s2.len */
        return memcmp(s1.str, s2.str, s1.len);
}


/*! @brief    saves the current top of the text space.
 *
 *  text_save() saves the current state of the text space and returns it. The text space to provide
 *  storages for texts can be seen as a stack and storages allocated by text_*() (except that
 *  allocated by text_get()) can be seen as piled up in the stack, thus any storage being used by
 *  the Text Library after a call to text_save() can be set free by calling text_restore() with the
 *  saved state. After text_restore(), any text constructed after the text_save() call is
 *  invalidated and should not be used. In addition, other saved states, if any, get also
 *  invalidated if the text space gets back to a previous state by a state saved before they are
 *  generated. For example, after the following code:
 *
 *  @code
 *      h = text_save();
 *      ...
 *      g = text_save();
 *      ...
 *      text_restore(h);
 *  @endcode
 *
 *  calling text_restore() with @c g makes the program behave in an unpredicatble way since the last
 *  call to text_restore() with @c h invalidates @c g.
 *
 *  Possible exceptions: memory_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @return    saved state of text space
 *
 *  @todo    Some improvements are possible and planned:
 *           - text_save() and text_restore() can be improved to detect an erroneous call shown in
 *             the above example;
 *           - the stack-like storage management by text_save() and text_restore() unnecessarily
 *             keeps the Text Library from being used in ohter libraries. For example,
 *             text_restore() invoked by a clean-up function of a library can destroy the storage
 *             for texts that are still in use by a program. The approach used by the Arena Library
 *             would be more appropriate.
 *
 *  @internal
 *
 *  The original code calls alloc() to keep a text from straddling the end of the text space that is
 *  returned by text_save(). This seems unnecessary in this implementation, so commented out.
 */
text_save_t *(text_save)(void)
{
    text_save_t *save;

    MEM_NEW(save);
    save->current = current;
    save->avail = current->avail;
    /* alloc(1); */

    return save;
}


/*! @brief    restores a saved state of the text space.
 *
 *  text_restore() gets the text space to a state returned by text_save(). As explained in
 *  text_save(), any text and state generated after saving the state to be reverted are invalidated,
 *  thus they should not be used. See text_save() for more details.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid saved state given for @p save
 *
 *  @param[in]    save    pointer to saved state of text space
 *
 *  @return    nothing
 */
void (text_restore)(text_save_t **save)
{
    struct chunk *p, *q;

    assert(save);
    assert(*save);

    current = (*save)->current;
    current->avail = (*save)->avail;
    MEM_FREE(*save);

    for (p = current->link; p; p = q) {
        q = p->link;
        MEM_FREE(p);
    }

    current->link = NULL;
}


/*! @brief    finds the first occurrence of a character in a text.
 *
 *  text_chr() finds the first occurrence of a character @p c in the specified range of a text @p s.
 *  The range is specified by @p i and @p j. If found, text_chr() returns the left position of the
 *  found character. It returns 0 otherwise. For example, given the following text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        e  v  e  n  t  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_chr(t, -6, 5, 'e') gives 1 while text_chr(t, -6, 5, 's') does 0.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s
 *
 *  @param[in]    s    text in which character is to be found
 *  @param[in]    i    range specified
 *  @param[in]    j    range specified
 *  @param[in]    c    character to find
 *
 *  @return    left positive position of found character or 0
 *
 *  @internal
 *
 *  Considering characters that sigend char cannot represent and an implementation where "plain"
 *  char is signed, a cast to unsigned char * is added.
 */
int (text_chr)(text_t s, int i, int j, int c)
{
    assert(s.len >= 0 && s.str);    /* validity check for text */

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    for (; i < j; i++)    /* note that < is used */
        if (((unsigned char *)s.str)[i] == c)
            return i + 1;

    return 0;
}


/*! @brief    finds the last occurrence of a character in a text.
 *
 *  text_rchr() finds the last occurrence of a character @p c in the specified range of a text @p s.
 *  The range is specified by @p i and @p j. If found, text_rchr() returns the left position of the
 *  found character. It returns 0 otherwise. For example, given the following text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        e  v  e  n  t  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_rchr(t, -6, 5, 'e') gives 3 while text_rchr(t, -6, 5, 's') does 0. The "r" in its name
 *  stands for "right" since what it does can be seen as scanning a given text from the right end.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s
 *
 *  @param[in]    s    text in which character is to be found
 *  @param[in]    i    range specified
 *  @param[in]    j    range specified
 *  @param[in]    c    character to find
 *
 *  @return    left positive position of found character or 0
 *
 *  @internal
 *
 *  Considering characters that signed char cannot represent and an implementation where "plain"
 *  char is signed, a cast to unsigned char * is added.
 */
int (text_rchr)(text_t s, int i, int j, int c)
{
    assert(s.len >= 0 && s.str);    /* validity check for text */

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    while (j > i)    /* note that > and prefix -- are used */
        if (((unsigned char *)s.str)[--j] == c)
            return j + 1;

    return 0;
}


/*! @brief    finds the first occurrence of any character from a set in a text.
 *
 *  text_upto() finds the first occurrence of any character from a set @p set in the specified range
 *  of a text @p s. The range is specified by @p i and @p j. If found, text_upto() returns the left
 *  position of the found character. It returns 0 otherwise. For example, given the following text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        e  v  e  n  t  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_upto(t, -6, 5, text_box("vwxyz", 5)) gives 2. If the set containing characters to find is
 *  empty, text_upto() always fails and returns 0.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p set
 *
 *  @param[in]    s      text in which character is to be found
 *  @param[in]    i      range specified
 *  @param[in]    j      range specified
 *  @param[in]    set    set text containing characters to find
 *
 *  @return    left positive position of found character or 0
 */
int (text_upto)(text_t s, int i, int j, text_t set)
{
    assert(set.len >= 0 && set.str);    /* validity check for texts */
    assert(s.len >= 0 && s.str);

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    for (; i < j; i++)    /* note that < is used */
        if (memchr(set.str, s.str[i], set.len))
            return i + 1;

    return 0;
}


/*! @brief    finds the last occurrence of any character from a set in a text.
 *
 *  text_rupto() finds the last occurrence of any character from a set @p set in the specified range
 *  of a text @p s. The range is specified by @p i and @p j. If found, text_rupto() returns the left
 *  position of the found character. It returns 0 otherwise. For example, given the following text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        e  v  e  n  t  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_rupto(t, -6, 5, text_box("escape", 6)) gives 3. If the set containing characters to find is
 *  empty, text_rupto() always fails and returns 0.
 *
 *  The "r" in its name stands for "right" since what it does can be seen as scanning a given text
 *  from the right end.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p set
 *
 *  @param[in]    s      text in which character is to be found
 *  @param[in]    i      range specified
 *  @param[in]    j      range specified
 *  @param[in]    set    set text containing characters to find
 *
 *  @return    left positive position of found character or 0
 */
int (text_rupto)(text_t s, int i, int j, text_t set)
{
    assert(set.len >= 0 && set.str);    /* validity check for texts */
    assert(s.len >= 0 && s.str);

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    while (j > i)    /* note that > and prefix -- are used */
        if (memchr(set.str, s.str[--j], set.len))
            return j + 1;

    return 0;
}


/*! @brief    finds the first occurrence of a text in a text.
 *
 *  text_find() finds the first occurrence of a text @p str in the specified range of a text @p s.
 *  The range is specified by @p i and @p j. If found, text_find() returns the left position of the
 *  character starting the found text. It returns 0 otherwise. For example, given the following
 *  text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        c  a  c  a  o  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_find(t, 6, -6, text_box("ca", 2)) gives 1. If @p str is empty, text_find() always succeeds
 *  and returns the left positive position of the specified range.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p str
 *
 *  @param[in]    s      text in which another text is to be found
 *  @param[in]    i      range specified
 *  @param[in]    j      range specified
 *  @param[in]    str    text to find
 *
 *  @return    left positive position of found text or 0
 *
 *  @internal
 *
 *  Considering characters that signed char cannot represent and an implementation where "plain"
 *  char is signed, casts to unsigned char * are added.
 */
int (text_find)(text_t s, int i, int j, text_t str)
{
    assert(str.len >= 0 && str.str);    /* validity check for texts */
    assert(s.len >= 0 && s.str);

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    if (str.len == 0)    /* finding empty text always succeeds */
        return i + 1;
    else if (str.len == 1) {    /* finding-character case */
        for (; i < j; i++)    /* note that < is used */
            if (((unsigned char *)s.str)[i] == *(unsigned char *)str.str)
                return i + 1;
    } else
        for (; i + str.len <= j; i++)    /* note that <= is used and str.len added */
            if (EQUAL(s, i, str))
                return i + 1;

    return 0;
}


/*! @brief    finds the last occurrence of a text in a text.
 *
 *  text_rfind() finds the last occurrence of a text @p str in the specified range of a text @p s.
 *  The range is specified by @p i and @p j. If found, text_rfind() returns the left position of the
 *  character starting the found text. It returns 0 otherwise. For example, given the following
 *  text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        c  a  c  a  o  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_rfind(t, -6, 6, text_box("ca", 2)) gives 3. If @p str is empty, text_rfind() always
 *  succeeds and returns the right positive position of the specified range.
 *
 *  The "r" in its name stands for "right" since what it does can be seen as scanning a given text
 *  from the right end.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p str
 *
 *  @param[in]    s      text in which another text is to be found
 *  @param[in]    i      range specified
 *  @param[in]    j      range specified
 *  @param[in]    str    text to find
 *
 *  @return    left positive position of found text or 0
 *
 *  @internal
 *
 *  Considering characters that signed char cannot represent and an implementation where "plain"
 *  char is signed, casts to unsigned char * are added.
 */
int (text_rfind)(text_t s, int i, int j, text_t str)
{
    assert(str.len >= 0 && str.str);    /* validity check for text */
    assert(s.len >= 0 && s.str);

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    if (str.len == 0)    /* finding empty text always succeeds */
        return j + 1;
    else if (str.len == 1) {    /* finding-character case */
        while (j > i)    /* note that > and prefix -- are used */
            if (((unsigned char *)s.str)[--j] == *(unsigned char *)str.str)
                return j + 1;
    } else
        for (; j - str.len >= i; j--)    /* note that >= is used and str.len subtracted */
            if (EQUAL(s, j - str.len, str))
                return j - str.len + 1;

    return 0;
}


/*! @brief    checks if a character of a specified position matches any character from a set.
 *
 *  text_any() checks if a character of a specified position by @p i in a text @p s matches any
 *  character from a set @p set. @p i specifies the left position of a character. If it matches,
 *  text_any() returns the right positive position of the character or 0 otherwise. For example,
 *  given the following text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        c  a  c  a  o  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_any(t, 2, text_box("ca", 2)) gives 3 because @c a matches. If the set containing characters
 *  to find is empty, text_any() always fails and returns 0.
 *
 *  Note that giving to @p i the last position (7 or 0 in the example text) makes text_any() fail
 *  and return 0; that does not cause the assertion to fail since it is a valid position.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p set
 *
 *  @param[in]    s      text in which character is to be found
 *  @param[in]    i      left position of character to match
 *  @param[in]    set    set text containing characters to find
 *
 *  @return    right positive position of matched character or 0
 */
int (text_any)(text_t s, int i, text_t set)
{
    assert(s.len >= 0 && s.str);    /* validity check for texts */
    assert(set.len >= 0 && set.str);

    i = IDX(i, s.len);

    assert(i >= 0 && i <= s.len);    /* validity check for position; see text_pos() */

    if (i < s.len && memchr(set.str, s.str[i], set.len))    /* note that i < s.len is checked */
        return i + 2;    /* right position, so +2 */

    return 0;
}


/*! @brief    finds the end of a span consisted of characters from a set.
 *
 *  If the specified range of a text @p s starts with a character from a set @p set, text_many()
 *  returns the right positive position ending a span consisted of characters from the set. The
 *  range is specified by @p i and @p j. It returns 0 otherwise. For example, given the following
 *  text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        c  a  c  a  o  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_many(t, 2, 6, text_box("ca", 2)) gives 5. If the set containing characters to find is
 *  empty, text_many() always fails and returns 0.
 *
 *  Since text_many() checks the range starts with a character from a given set, text_many() is
 *  often called after text_upto().
 *
 *  The original code in the book is modified to form a more compact form.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p set
 *
 *  @param[in]    s      text in which character to be found
 *  @param[in]    i      range specified
 *  @param[in]    j      range specified
 *  @param[in]    set    set text containing characters to find
 *
 *  @return    right positive position of span or 0
 */
int (text_many)(text_t s, int i, int j, text_t set)
{
    int t;

    assert(set.len >= 0 && set.str);    /* validity check for text */
    assert(s.len >= 0 && s.str);

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    t = i;
    while (i < j && memchr(set.str, s.str[i], set.len))    /* note that i < j is checked */
        i++;

    return (t == i)? 0: i + 1;
}


/*! @brief    finds the start of a span consisted of characters from a set.
 *
 *  If the specified range of a text @p s ends with a character from a set @p set, text_rmany()
 *  returns the left positive position starting a span consisted of characters from the set. The
 *  range is specified by @p i and @p j. It returns 0 otherwise. For example, given the following
 *  text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        c  a  c  a  o  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_rmany(t, 3, 7, text_box("aos", 3)) gives 4. The "r" in its name stands for "right" since
 *  what it does can be seen as scanning a given text from the right end. If the set containing
 *  characters to find is empty, text_rmany() always fails and returns 0.
 *
 *  Since text_rmany() checks the range ends with a character from a given set, text_rmany() is
 *  often called after text_rupto().
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p set
 *
 *  @param[in]    s      text in which character to be found
 *  @param[in]    i      range specified
 *  @param[in]    j      range specified
 *  @param[in]    set    set text containing characters to find
 *
 *  @return    right positive position of span or 0
 *
 *  @internal
 *
 *  The original code in the book is modified to form a more compact form.
 */
int (text_rmany)(text_t s, int i, int j, text_t set)
{
    int t;

    assert(set.len >= 0 && set.str);    /* validity check for texts */
    assert(s.len >= 0 && s.str);

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    t = j;
    while (j > i && memchr(set.str, s.str[j-1], set.len))    /* note that j > i is checked */
        j--;

    return (t == j)? 0: j + 1;
}


/*! @brief    checks if a text starts with another text.
 *
 *  If the specified range of a text @p s starts with a text @p str, text_match() returns the right
 *  positive position ending the matched text. The range is specified by @p i and @p j. It returns
 *  0 otherwise. For example, given the following text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        c  a  c  a  o  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_match(t, 3, 7, text_box("ca", 2)) gives 5. If @p str is empty, text_match() always succeeds
 *  and returns the left positive position of the specified range.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p str
 *
 *  @param[in]    s      text in which another text to be found
 *  @param[in]    i      range specified
 *  @param[in]    j      range specified
 *  @param[in]    str    text to find
 *
 *  @return    right positive position ending matched text or 0
 *
 *  @internal
 *
 *  Considering characters that signed char cannot represent and an implementation where "plain"
 *  char is signed, casts to unsigned char * are used.
 */
int (text_match)(text_t s, int i, int j, text_t str)
{
    assert(str.len >= 0 && str.str);    /* validity check for text */
    assert(s.len >= 0 && s.str);

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    if (str.len == 0)    /* finding empty text always succeeds */
        return i + 1;
    else if (str.len == 1) {    /* finding-character case */
        if (i < j &&    /* note that < is used */
            ((unsigned char *)s.str)[i] == *(unsigned char *)str.str)
            return i + 2;
    } else if (i + str.len <= j && EQUAL(s, i, str))    /* note that <= is used and str.len added */
        return i + str.len + 1;

    return 0;
}


/*! @brief    checks if a text ends with another text.
 *
 *  If the specified range of a text @p s ends with a text @p str, text_rmatch() returns the left
 *  positive position starting the matched text. The range is specified by @p i and @p j. It returns
 *  0 otherwise. For example, given the following text:
 *
 *  @code
 *      1  2  3  4  5  6  7    (positive positions)
 *        c  a  c  a  o  s
 *      -6 -5 -4 -3 -2 -1 0    (non-positive positions)
 *  @endcode
 *
 *  text_rmatch(t, 3, 7, text_box("os", 2)) gives 5. If @p str is empty, text_rmatch() always
 *  succeeds and returns the right positive position of the specified range.
 *
 *  The "r" in its name stands for "right" since what it does can be seen as scanning a given text
 *  from the right end.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: invalid text given for @p s or @p str
 *
 *  @param[in]    s      text in which another text to be found
 *  @param[in]    i      range specified
 *  @param[in]    j      range specified
 *  @param[in]    str    text to find
 *
 *  @return    left positive position starting matched text or 0
 *
 *  @internal
 *
 *  Considering characters that signed char cannot represent and an implementation where "plain"
 *  char is signed, casts to unsigned char * are added.
 */
int (text_rmatch)(text_t s, int i, int j, text_t str)
{
    assert(str.len >= 0 && str.str);    /* validity check for texts */
    assert(s.len >= 0 && s.str);

    i = IDX(i, s.len);
    j = IDX(j, s.len);

    if (i > j)
        SWAP(i, j);

    assert(i >= 0 && j <= s.len);    /* validity check for position; see text_pos() */

    if (str.len == 0)    /* finding empty text always succeeds */
        return j + 1;
    else if (str.len == 1) {    /* finding-character case */
        if (j > i &&    /* note that > is used */
            ((unsigned char *)s.str)[j-1] == *(unsigned char *)str.str)
            return j;
    } else if (j - str.len >= i && EQUAL(s, j - str.len, str))    /* note that >= is used and
                                                                     str.len subtracted */
        return j - str.len + 1;

    return 0;
}

/* end of text.c */
