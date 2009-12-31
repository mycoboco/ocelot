/*!
 *  @file    text.h
 *  @brief    Header for Text Library (CBL)
 */

#ifndef TEXT_H
#define TEXT_H


/*! @struct    text_t    text.h
 *  @brief    implements a text.
 *
 *  struct @c text_t implements a text that is alternative representation of character strings. The
 *  C representation of strings using the terminating null character has some drawbacks:
 *  - it has to scan all characters in a string to determine its length, and
 *  - it too often has to copy a string even when not necessary.
 *  For example, suppose concatenating a string to another string and those strings are not
 *  immutable. To determine the byte into which the former string is to be put, one has to compute
 *  the length of the later string, which requires to scan all characters in it. Besides, one has to
 *  somehow allocate storage to contain the result because there is no way for two different strings
 *  to share the same sequence of characters in the storage. Jobs like scanning a string and
 *  allocation of new storage can be avoided by getting rid of the terminating null character and
 *  keeping the length of a string in a separate place.
 *
 *  Because the null character does not play a role of terminating a string anymore, there is no
 *  need to treat it specially. Therefore, a text represented by @c text_t is allowed to contain the
 *  null character anywhere in it, and other @c text_t functions never treat it in a special way.
 *  Nevertheless, be warned that, if a text which has the null character embedded in it is converted
 *  to a C string, the resulting string might not work as expected because of the embedded null
 *  character.
 *
 *  The Text Library is not intended to completely replace the C representation of strings. To
 *  perform string operations other than those provided by the Text Library, a user has to convert
 *  a text back to a C string and then apply ordinary string functions to the result. Such a
 *  conversion between those two representations is the cost for the benefit the Text Library
 *  confers. To minimize the cost, some basic text operations like comparison and mapping are also
 *  supported.
 *
 *  @c text_t intentionally reveals its interals, so that a user can readily get the length of a
 *  text and can access to the text as necessary. Modifying a text, however, makes a program behave
 *  in an unpredictable way, which is the reason the @c str member is const-qualified.
 *
 *  Most functions in this library take and return a @c text_t value, not a pointer to it. This
 *  design approach simplifies an implementation since they never need to allocate a descriptor for
 *  a text. The size of @c text_t being not so big, passing its value would cause no big penalty on
 *  performance.
 */
typedef struct text_t {
    int len;            /*!< length of string */
    const char *str;    /*!< string (possibly not having null character */
} text_t;

/*! @brief    represents information on the top of the stack-like text space.
 *
 *  An object of the type @c text_save_t is used when remembering the current top of the stack-like
 *  text space and restoring the space to make it have as the current the top remembered in the
 *  @c text_save_t object. For more details, see struct @c text_save_t, struct @c chunk, text_save()
 *  and text_restore().
 */
typedef struct text_save_t text_save_t;


/* predefined texts */
extern const text_t text_ucase;
extern const text_t text_lcase;
extern const text_t text_digits;
extern const text_t text_null;


/*! @name    text creating functions:
 */
/*@{*/
text_t text_put(const char *);
text_t text_gen(const char *, int);
text_t text_box(const char *, int);
char *text_get(char *, int, text_t);
/*@}*/

/*! @name    text positioning functions:
 */
/*@{*/
int text_pos(text_t, int);
/*@}*/

/*! @name    text handling functions:
 */
/*@{*/
text_t text_sub(text_t, int, int);
text_t text_cat(text_t, text_t);
text_t text_dup(text_t, int);
text_t text_reverse(text_t);
text_t text_map(text_t, const text_t *, const text_t *);
/*@}*/

/*! @name    text comparing functions:
 */
/*@{*/
int text_cmp(text_t, text_t);
/*@}*/

/*! @name    text analyzing functions (character):
 */
/*@{*/
int text_chr(text_t, int, int, int);
int text_rchr(text_t, int, int, int);
int text_upto(text_t, int, int, text_t);
int text_rupto(text_t, int, int, text_t);
int text_any(text_t, int, text_t);
int text_many(text_t, int, int, text_t);
int text_rmany(text_t, int, int, text_t);
/*@}*/

/*! @name    text analyzing functions (string):
 */
/*@{*/
int text_find(text_t, int, int, text_t);
int text_rfind(text_t, int, int, text_t);
int text_match(text_t, int, int, text_t);
int text_rmatch(text_t, int, int, text_t);
/*@}*/

/*! @name    text space managing functions:
 */
/*@{*/
text_save_t *text_save(void);
void text_restore(text_save_t **save);
/*@}*/


/*! @brief    accesses with a position a character in a text.
 *
 *  TEXT_ACCESS() is useful when accessing a character in a text when a position number is known.
 *  The position can be negative, but has to be within a valid range. For example, given a text of
 *  4 characters, a valid positive range for the position is from 1 to 4 and a valid negative range
 *  from -4 to -1; 0 is never allowed. No validity check for the range is performed.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: invalid position given for @p i
 */
#define TEXT_ACCESS(t, i) ((t).str[((i) <= 0)? (i)+(t).len: (i)-1])


#endif    /* TEXT_H */

/* end of text.h */
