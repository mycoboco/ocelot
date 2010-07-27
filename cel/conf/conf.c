/*!
 *  @file    conf.c
 *  @brief    Source for Configuration File Library (CEL)
 */

#include <stddef.h>    /* NULL, size_t */
#include <stdio.h>     /* FILE, EOF, fgets, getc, ungetc */
#include <ctype.h>     /* isalpha, isdigit, isspace, tolower */
#include <string.h>    /* strchr, strlen, strcpy, strcspn */
#include <stdlib.h>    /* strtod, strtol, strtoul */
#include <errno.h>     /* errno */

#include "cbl/assert.h"    /* assert with exception support */
#include "cbl/memory.h"    /* MEM_ALLOC, MEM_NEW, MEM_FREE, MEM_RESIZE */
#include "cdsl/hash.h"     /* hash_string */
#include "cdsl/table.h"    /* table_t, table_new, table_get, table_put, table_free, table_map */
#include "conf.h"


#define BUFLEN 80    /* size of buffer to read lines */

/* @brief    checks @c c is a valid character for a section or variable name.
 *
 * Whenever the validity of a name given for a section or variable checked, VALID_CHR() is used. If
 * any change on the acceptable form of a variable and section name is desired, change this.
 */
#define VALID_CHR(c) (isalpha(c) || isdigit(c) || (c) == '_')


/* @struct    valnode_t    conf.c
 * @brief    represents a node in a table for variable-value pairs.
 *
 * A table for containing variables and their values actually has a pointer to struct @c valnode_t
 * as the value. To aid memory management in the library implementation, the @c freep member points
 * to any storage that has to be freed when clean-up requested. The @c type member indicates the
 * type of a value (string) that the @c val member points to.
 */
struct valnode_t {
    void *freep;    /* pointer used for clean-up */
    int type;       /* type of value */
    char *val;      /* pointer to value */
};


/* @brief    defines the table for contatining sections.
 *
 * @c section has section names as keys and other tables for containing variable-value pairs.
 */
static table_t *section;

/* @brief    provides a pointer to the current table; see conf_section().
 */
static table_t *current;

/* @brief    indicates that a set of supported sections and varialbes are prescribed.
 *
 * If @c preset is set, conf_init() accepts only what is prescribed via conf_preset() when reading
 * configuration data from a file; unknown sections and variables result in setting an error code
 * and aborting further processing of the configuration file. Otherwise, any sections and variable
 * described given in a configuration file are accepted and can be claimed later by conf_get() or
 * other similar functions. For more details, see conf_preset().
 */
static int preset;

/* @brief    indicates that an error has been occurred.
 *
 * All functions in this library clear @c errcode before starting its job and set it properly if
 * something wrong happens. A user program can inspects if an error occurred and what the error is
 * with conf_errcode() that simply returns the value of @c errcode. Error codes are defined to have
 * the names starting with @c CONF_ERR_. See conf_errcode() or conf_errstr() for details.
 */
static int errcode;

/* @brief    controls some aspects of the library.
 *
 * Through @c control that is set when conf_preset() or conf_init() invoked, a user can control how
 * the library recognizes section or variable names and values. See conf_preset() or conf_init() for
 * details.
 */
static int control;


/* @brief    trims leading whitespaces.
 *
 * triml() trims leading whitespaces of a string. The whitespaces are recognized by isspace()
 * declared by <ctype.h>. triml() returns a new starting address of the string.
 *
 * @param[in]    s    string from which leading whitespaces trimmed
 *
 * @return    pointer to trimmed string
 */
static char *triml(char *s)
{
    unsigned char *p;

    assert(s);

    for (p = (unsigned char *)s; *p != '\0' && isspace(*p); p++)
        continue;

    return (char *)p;
}


/* @brief    trims trailing whitespaces.
 *
 * trimt() trims trailing whitespaces of a string. The whitespaces are recognized by isspace()
 * declared by <ctype.h>. trimt() returns a pointer to the null character terminating the string.
 *
 * @warning    trimt() trims trailing spaces by replacing the first of them with a null character,
 *             which means it changes what @p s points to. Thus, @p s has to point to a modifiable
 *             string.
 *
 * @param[in]    s    string from which trailing whitespaces trimmed
 *
 * @return    pointer to end of trimmed string
 */
static char *trimt(char *s)
{
    int lastc;
    unsigned char *p, *q;

    assert(s);

    q = NULL;
    lastc = EOF;    /* isspace(EOF) == 0 */
    for (p = (unsigned char *)s; *p != '\0'; p++) {
        if (!isspace(lastc) ^ !isspace(*p))    /* boundary found */
            q = (isspace(lastc))? NULL: p;
        lastc = *p;
    }

    if (q)
        *q = '\0';
    else
        q = p;

    return (char *)q;
}


/* @brief    finds a section or variable name from a string.
 *
 * A string given, sepunit() trims it properly and finds a sub-string that can be a valid section or
 * variable name. If any interleaving space occurs or any invalid character follows, sepunit() sets
 * @c errcode properly and returns its value. If nothing wrong, @p pp is set to points to the
 * recognized section or variable name. sepunit() clears and sets @c errcode.
 *
 * @warning    sepunit() may change what @p points to, thus the string has to be modifiable.
 *
 * @param[in]     p     string from which section or variable name found
 * @param[out]    pp    pointer to be set to point to recognized name
 *
 * @return    success/failure indicator
 * @retval    CONF_ERR_OK    success
 * @retval    others         failure
 */
static int sepunit(char *p, char **pp)
{
    unsigned char *q;

    /* assert(p) done by triml() */
    assert(pp);

    errcode = CONF_ERR_OK;

    q = (unsigned char *)(p = triml(p));
    trimt(p);

    while (VALID_CHR(*q))
            q++;

    if (*q != '\0') {
        if (isspace(*q))
            errcode = CONF_ERR_SPACE;
        else
            errcode = CONF_ERR_CHAR;
    }
    *q = '\0';

    *pp = p;

    return errcode;
}


/* @brief    finds section and variable names from a string.
 *
 * sep analyzes a string and finds section and variable names from it. As explained in conf_init(),
 * a section and/or variable name has the form as follows:
 *
 * @code
 *     [section] [.] variable
 * @endcode
 *
 * where the brackets means optional elements. Spaces are allowed before and after a section name
 * and a variable name. If an empty section name given as in ".variable", the global section is
 * selected (when @p psec set to point to an empty string). If no section name given as in
 * "variable", the current section is selected (when @p psec set to have a null pointer). Even
 * if an empty section name is allowed for referring to the global section, an empty variable name
 * is not permitted. sep() clears and sets @c errcode.
 *
 * @warning    sep() assumes that the given string is modifiable.
 *
 * @param[in]     var     string from which a section or variable name found
 * @param[out]    psec    set to point to section name
 * @param[out]    pvar    set to point to variable name
 *
 * @return    success/failure indicator
 * @retval    CONF_ERR_OK    success
 * @retval    others         failure
 */
static int sep(char *var, char **psec, char **pvar)
{
    char *p;

    assert(var);
    assert(psec);
    /* assert(pvar) done by sepunit() */

    errcode = CONF_ERR_OK;

    p = strchr(var, '.');
    if (p) {    /* section given */
        *p = '\0';
        errcode = sepunit(var, psec);
        p++;
    } else {    /* current section */
        *psec = NULL;
        p = var;
    }

    if (errcode == CONF_ERR_OK) {
        errcode = sepunit(p, pvar);
        if (errcode == CONF_ERR_OK && *(unsigned char *)*pvar == '\0')    /* empty variable */
            errcode = CONF_ERR_VAR;
    }

    return errcode;
}


/* @brief    removes a comment.
 *
 * cmtrem() removes a comment from a string. It locates the first occurrance of a comment character
 * that is either '#' or ';'. It replaces it with a space and removes any trailing characters until
 * a newline.
 *
 * @warning    cmtrem() may change what @p s points to, thus the string has to be modifiable.
 *
 * @param[in]    s    string from which comment stripped
 *
 * @return    nothing
 */
static void cmtrem(char *s)
{
    unsigned char *p;

    assert(s);

    for (p = (unsigned char *)s; *p != '\0'; p++)
        if (*p == ';' || *p == '#') {
            *p++ = ' ';
            *p = '\0';
        }
}


/* @brief    converts any uppercase letters in a string to corresponding lowercase ones.
 *
 * lower() converts uppercase letters in a string to lowercase ones; other characters are left
 * untouched.
 *
 * @warning    lower() converts characters in-place, thus the string has to be modifiable.
 *
 * @param[in]    s    string to convert
 *
 * @return    converted string
 */
static const char *lower(char *s)
{
    unsigned char *p;

    assert(s);

    for (p = (unsigned char *)s; *p != '\0'; p++)
        *p = tolower(*p);

    return s;
}


/*! @brief    constructs a default set for configuration variables.
 *
 *  A user program can specify the default set of configuration variables (including sections to
 *  which they belong and their types) with conf_preset(). The table (an array, in fact) containing
 *  necessary information have the @c conf_t type and called a "configuration description table."
 *  For a detailed explanation and examples, see @c conf_t. conf_preset(), if invoked, has to be
 *  called before conf_init(). conf_init() is not necessarily invoked if conf_preset() is used.
 *
 *  If invoked, conf_preset() remembers names that need to be recognized as sections and variables,
 *  types of variables, and their default values. When conf_init() processes a configuration file,
 *  a sections or variable that is not given via conf_preset() is considered an error. Using
 *  conf_preset() and a configuration description table is the only way to let variables have other
 *  types than @c CONF_TYPE_STR (string type).
 *
 *  If not invoked, conf_init() accepts any section and variable name (if they have a valid form)
 *  and all of variables are assumed to be of @c CONF_TYPE_STR type.
 *
 *  conf_preset() also takes @p ctrl for controling some behaviors of the library, especially
 *  handling section/variable names and values. If the @c CONF_OPT_CASE bit is set in @p ctrl (that
 *  is, @c CONF_OPT_CASE & @p ctrl is not 0), section and variable names are case-sensitive. If the
 *  @c CONF_OPT_ESC bit is set in @p ctrl, some forms of escape sequences are supported in a quoted
 *  value. The default behavior is that section and variable names are case-insensitive and no
 *  escape sequences are supported.
 *
 *  @warning    conf_preset() does not warn that a default value for a variable does not have an
 *              expected form for the variable's type. It is to be treated as an error when
 *              retrieving the value by conf_get() or similar functions.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    tab     configuration description table
 *  @param[in]    ctrl    control code
 *
 *  @return    success/failure indicator
 *  @retval    CONF_ERR_OK    success
 *  @retval    others         failure
 *
 *  @todo    Improvements are possible and planned:
 *           - some adjustment on arguments to table_new() is necessary;
 *           - considering changes to the format of a configuration file as a program to accept it
 *             is upgraded, making it a recoverable error to encounter a non-preset section or
 *             variable name would be useful; this enables an old version of the program to accept
 *             a new configuration file with diagnostics.
 *
 *  @internal
 *
 *  conf_preset() cannot be implemented with conf_set() since they differ in handling an omitted
 *  section name, that is a variable name with no preceeding a period. conf_section() cannot affect
 *  conf_preset() because the former is not callable before the latter. Thus, conf_preset() can
 *  safely assume that a section name is given properly whenever necessary. On the other hand,
 *  because conf_set() is affected by conf_section(), a variable name without a section name and a
 *  period should be recognized as referring to the current section, not to the global section.
 */
int (conf_preset)(const conf_t *tab, int ctrl)
{
    size_t len;
    char *pbuf, *sec, *var;
    const char *hkey;
    table_t *t;
    struct valnode_t *pnode;

    assert(!section);
    assert(tab);

    control = ctrl;
    section = table_new(0, NULL, NULL);
    errcode = CONF_ERR_OK;

    for ( ; tab->var; tab++) {
        assert(tab->defval);
        assert(tab->type == CONF_TYPE_BOOL || tab->type == CONF_TYPE_INT ||
               tab->type == CONF_TYPE_UINT || tab->type == CONF_TYPE_REAL ||
               tab->type == CONF_TYPE_STR);

        len = strlen(tab->var);
        pbuf = strcpy(MEM_ALLOC(len+1 + strlen(tab->defval)+1), tab->var);
        if (sep(pbuf, &sec, &var) != CONF_ERR_OK) {
            MEM_FREE(pbuf);
            break;    /* sep sets errcode */
        }
        if (!sec)    /* means global section in this case */
            sec = "";    /* lower modifies sec only when necessary */
        hkey = hash_string((control & CONF_OPT_CASE)? sec: lower(sec));
        t = table_get(section, hkey);
        if (!t) {    /* new section */
            t = table_new(0, NULL, NULL);
            table_put(section, hkey, t);
        }
        MEM_NEW(pnode);
        pnode->type = tab->type;
        pnode->freep = pbuf;
        pnode->val = strcpy(pbuf + len+1, tab->defval);
        table_put(t, hash_string((control & CONF_OPT_CASE)? var: lower(var)), pnode);
    }

    preset = 1;

    return errcode;
}


/* @brief    returns a character for a escape sequence character.
 *
 * escseq() returns a character for an escape sequence. Giving the character following the backslash
 * character in an escape sequence, a caller of escseq() can gain the character that the escape
 * sequence refers to. If @c c has a character that is invalid for an escape sequence, escseq() just
 * returns the character unchanged.
 *
 * escseq() is called only by conf_init() and constitutes a separate function just to make the
 * length of conf_init() shorter for code readability.
 *
 * @param[in]    c    second character of escape sequence
 *
 * @return    character that escape sequence represents
 */
static int escseq(int c)
{
    switch(c) {
        case '\'':    /* \' */
            return '\'';
            break;
        case '\"':    /* \" */
            return '\"';
            break;
        case '\\':    /* \\ */
            return '\\';
            break;
        case '0':    /* null char */
            return '\0';
            break;
        case 'a':    /* \a */
            return '\a';
            break;
        case 'b':    /* \b */
            return '\b';
            break;
        case 'f':    /* \f */
            return '\f';
            break;
        case 'n':    /* \n */
            return '\n';
            break;
        case 'r':    /* \r */
            return '\r';
            break;
        case 't':    /* \t */
            return '\t';
            break;
        case 'v':    /* \v */
            return '\v';
            break;
        case ';':    /* \; */
            return ';';
            break;
        case '#':    /* \# */
            return '#';
            break;
        case '=':    /* \= */
            return '=';
            break;
        default:    /* unrecognized */
            break;
    }

    return c;
}


/*! @brief    reads a configuration file and constructs the configuration data.
 *
 *  conf_init() reads a configuration file and constructs the configuration data by analyzing the
 *  file. For how conf_init() interacts with conf_preset(), see conf_preset().
 *
 *  The default behavior of the library is that names are not case-insensitive and that escape
 *  sequences are not recognized. This behavior can be changed by setting the @c CONF_OPT_CASE and
 *  @c CONF_OPT_ESC bits in @p ctrl, respectively; see also conf_preset().
 *
 *  If the control mode that can be set through @p ctrl has been already set by conf_preset(),
 *  conf_init() ignores @p ctrl.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: invalid file pointer given for @p fp
 *
 *  @param[in]    fp      file pointer from which configuration data read
 *  @param[in]    ctrl    control code
 *
 *  @return    success/failure indicator
 *  @retval    0           success
 *  @retval    positive    line number on which error occurred
 *
 *  @todo    Improvements are possible and planned:
 *           - some adjustment on arguments to table_new() is necessary.
 *
 *  @internal
 *
 *  A tricky construct using do-while for reading lines from a file is to allow for a configuration
 *  file that does not end with a newline. Asserting that @c BUFLEN is greater than 1 is necessary
 *  to prevent fgets() from returning an empty string even if there are still characters unread.
 */
size_t (conf_init)(FILE *fp, int ctrl)
{
    char *p;
    size_t buflen, len;
    size_t lineno, nlineno;
    const char *hkey;
    table_t *tab;
    struct valnode_t *pnode;

    assert(fp);

    assert(preset || !section);
    assert(!current);
    assert(BUFLEN > 1);

    if (!preset) {
        control = ctrl;    /* if preset, ctrl ignored */
        section = table_new(0, NULL, NULL);
    }

    tab = NULL;
    errcode = CONF_ERR_OK;
    lineno = nlineno = 0;
    len = 0;
    p = MEM_ALLOC(buflen=BUFLEN);
    *p = '\0';

    do {
        assert(buflen - len > 1);
        fgets(p+len, buflen-len, fp);
        if (ferror(fp)) {
            errcode = CONF_ERR_IO;
            break;
        }
        len += strlen(p+len);
        if (len == 0)    /* EOF and no remaining data */
            break;
        if (len > 2 && p[len-2] == '\\' && p[len-1] == '\n') {    /* line splicing */
            int c;
            nlineno++;
            len -= 2;
            if (len > 0 && isspace(p[len-1])) {
                while (--len > 0 && isspace(p[len-1]))
                    continue;
                p[len++] = ' ';
            }
            if ((c = getc(fp)) != EOF && c != '\n' && isspace(c)) {
                if (p[len-1] != ' ')
                    p[len++] = ' ';
                while ((c = getc(fp)) != EOF && c != '\n' && isspace(c))
                        continue;
            } else if (c == EOF) {    /* no following line for slicing */
                lineno += nlineno;
                errcode = CONF_ERR_BSLASH;
                goto retcode;
            }
            ungetc(c, fp);
            p[len] = '\0';
            /* at worst, backslash is replaced with space and newline with null, thus room for two
               characters, which guarantees buflen-len greater than 1 */
        } else if (p[len-1] == '\n' || feof(fp)) {    /* line completed */
            int type;
            char *s, *t, *var, *val;

            lineno++;
            if (p[len-1] == '\n')
                p[--len] = '\0';    /* in fact, no need to adjust len */

            var = triml(p);
            switch(*var) {
                case '[':    /* section specification */
                    cmtrem(++var);
                    s = strchr(var, ']');
                    if (!s) {
                        errcode = CONF_ERR_LINE;
                        goto retcode;
                    }
                    *s = '\0';
                    if (sepunit(var, &var) != CONF_ERR_OK)
                        goto retcode;    /* sepunit sets errcode */
                    hkey = hash_string((control & CONF_OPT_CASE)? var: lower(var));
                    tab = table_get(section, hkey);
                    if (!tab) {
                        if (preset) {
                            errcode = CONF_ERR_SEC;
                            goto retcode;
                        }
                        tab = table_new(0, NULL, NULL);
                    }
                    table_put(section, hkey, tab);
                    /* no break */
                case '\0':    /* empty line */
                case '#':     /* comment-only line */
                case ';':
                    break;    /* reuses existing buffer */
                default:    /* variable = value */
                    val = p + strcspn(p, "#;");
                    s = strchr(var, '=');
                    if (!s || val < s) {
                        errcode = CONF_ERR_LINE;
                        goto retcode;
                    }
                    *s = '\0';
                    if (sepunit(var, &var) != CONF_ERR_OK)
                        goto retcode;    /* sepunit() sets errcode */
                    if (*(unsigned char *)var == '\0') {    /* empty variable name */
                        errcode = CONF_ERR_VAR;
                        goto retcode;
                    }
                    val = triml(++s);
                    if (*val == '"' || *val == '\'') {    /* quoted */
                        char end = *val;    /* remembers for match */

                        t = s = ++val;    /* starts copying from s to t */
                        do {
                            switch(*s) {
                                case '\\':    /* escape sequence */
                                    if (control & CONF_OPT_ESC)
                                        *(unsigned char *)t++ = escseq(*(unsigned char *)++s);
                                    else {
                                        *t++ = '\\';
                                        *(unsigned char *)t++ = *(unsigned char *)++s;
                                    }
                                    break;
                                case '\0':    /* unclosed ' or " */
                                    errcode = CONF_ERR_LINE;
                                    goto retcode;
                                case '\'':
                                case '\"':
                                    if (*s == end) {
                                        *t = '\0';
                                        break;
                                    }
                                    /* no break */
                                default:    /* literal copy */
                                    *(unsigned char *)t++ = *(unsigned char *)s;
                                    break;
                            }
                            s++;
                        } while(*(unsigned char *)t != '\0');
                        /* checks if any trailing invalid char */
                        cmtrem(s);
                        trimt(s);
                        if (*(unsigned char *)s != '\0') {
                            errcode = CONF_ERR_LINE;
                            goto retcode;
                        }
                    } else {    /* unquoted */
                        cmtrem(val);
                        trimt(val);
                    }

                    if (!tab) {    /* global section */
                        hkey = hash_string("");
                        tab = table_get(section, hkey);
                        if (!tab) {
                            if (preset) {
                                errcode = CONF_ERR_SEC;
                                goto retcode;
                            }
                            tab = table_new(0, NULL, NULL);
                        }
                        table_put(section, hkey, tab);
                    }
                    hkey = hash_string((control & CONF_OPT_CASE)? var: lower(var));
                    if (preset) {
                        pnode = table_get(tab, hkey);
                        if (!pnode) {
                            errcode = CONF_ERR_VAR;
                            goto retcode;
                        }
                        type = pnode->type;
                    } else
                        type = CONF_TYPE_STR;
                    MEM_NEW(pnode);
                    pnode->freep = p;
                    pnode->type = type;
                    pnode->val = val;
                    pnode = table_put(tab, hkey, pnode);
                    if (pnode) {    /* value overwritten, thus free */
                        MEM_FREE(pnode->freep);
                        MEM_FREE(pnode);
                    }

                    p = MEM_ALLOC(buflen=BUFLEN);    /* uses new buffer */
                    break;
            }
            len = 0;
            *p = '\0';
            lineno += nlineno;    /* adjusts line number */
            nlineno = 0;
        } else    /* expands buffer */
            MEM_RESIZE(p, buflen+=BUFLEN);
    } while(!feof(fp));

    retcode:
        MEM_FREE(p);
        return (errcode != CONF_ERR_OK)? lineno: 0;
}


/* @brief    retrieves a pointer to a value with a section/variable name.
 *
 * valget() retrieves a pointer to a value with a section/variable name. valget() clears and sets
 * @c errcode.
 *
 * @param[in]    secvar    section/variable name
 *
 * @return    pointer to value or null pointer
 * @retval    non-null    pointer to value
 * @retval    NULL        section/variable not found
 */
static void *valget(const char *secvar)
{
    size_t len;
    char buf[30], *pbuf;
    char *sec, *var;
    table_t *tab;
    const char *hkey;
    void *pnode;

    assert(secvar);

    /* sep() always clears errcode */

    len = strlen(secvar);
    pbuf = strcpy((len > sizeof(buf)-1)? MEM_ALLOC(len+1): buf, secvar);

    if (sep(pbuf, &sec, &var) != CONF_ERR_OK) {
        if (pbuf != buf)
            MEM_FREE(pbuf);
        return NULL;    /* sep sets errcode */
    }

    if (!sec) {    /* current selected */
        if (!current)    /* no current section, thus global */
            tab = table_get(section, hash_string(""));
        else
            tab = current;
    } else    /* section name given */
        tab = table_get(section, hash_string((control & CONF_OPT_CASE)? sec: lower(sec)));

    if (!tab) {
        if (pbuf != buf)
            MEM_FREE(pbuf);
        errcode = CONF_ERR_SEC;
        return NULL;
    }

    hkey = hash_string((control & CONF_OPT_CASE)? var: lower(var));
    if (pbuf != buf)
        MEM_FREE(pbuf);

    pnode = table_get(tab, hkey);
    if (!pnode)
        errcode = CONF_ERR_VAR;

    return pnode;
}


/*! @brief    converts a string based on a type.
 *
 *  conf_conv() converts a string to an integer or floating-point number as requested. @p type
 *  should be @c CONF_TYPE_BOOL (which recognizes some forms of boolean values), @c CONF_TYPE_INT
 *  (which indicates conversion to signed long int), @c CONT_TYPE_UINT (conversion to unsigned long
 *  int), @c CONF_TYPE_REAL (conversion to double) or @c CONF_TYPE_STR (no conversion necessary).
 *  The expected forms for @c CONF_TYPE_INT, @c CONF_TYPE_UINT and @c CONF_TYPE_REAL are
 *  respectively those for strtol(), strtoul() and strtod(). @c CONF_TYPE_BOOL gives 1 for a string
 *  starting with 't', 'T', 'y', 'Y', '1' and 0 for others. conf_conv() returns a pointer to the
 *  storage that contains the converted value (an integer, floating-point number or string) and its
 *  caller (user code) has to convert the pointer properly (to const int *, const long *, const
 *  unsigned long *, const double * and const char *) before use. If the conversion fails,
 *  conf_conv() returns a null pointer and sets @c CONF_ERR_TYPE as an error code.
 *
 *  @warning    A subsequent call to conf_getbool(), conf_getint(), conf_getuint() and
 *              conf_getreal() may overwrite the contents of the buffer pointed by the resulting
 *              pointer. Similarly, a subsequent call to conf_conv() and conf_get() may overwrite
 *              the contents of the buffer pointed by the resulting pointer unless the type is
 *              @c CONF_TYPE_STR.
 *
 *  @param[in]    val     string to convert
 *  @param[in]    type    type based on which conversion performed
 *
 *  @return    pointer to storage that contains result or null pointer
 *  @retval    non-null    pointer to conversion result
 *  @retval    NULL        conversion failure
 */
const void *conf_conv(const char *val, int type)
{
    /* provides storage for conversion result */
    static int boolean;              /* for CONF_TYPE_BOOL */
    static long inumber;             /* for CONF_TYPE_INT */
    static unsigned long unumber;    /* for CONF_TYPE_UINT */
    static double fnumber;           /* for CONF_TYPE_REAL */

    char *endptr;

    assert(val);

    errcode = CONF_ERR_OK;
    errno = 0;
    switch(type) {
        case CONF_TYPE_BOOL:
            while (*(unsigned char *)(val) != '\0' && isspace(*(unsigned char *)val))
                val++;    /* ignores leading spaces */
            if (*val == 't' || *val == 'T' || *val == 'y' || *val == 'Y' || *val == '1')
                boolean = 1;
            else
                boolean = 0;
            return &boolean;
        case CONF_TYPE_INT:
            inumber = strtol(val, &endptr, 0);
            if (*(unsigned char *)endptr != '\0' || errno)
                break;
            else
                return &inumber;
        case CONF_TYPE_UINT:
            unumber = strtoul(val, &endptr, 0);
            if (*(unsigned char *)endptr != '\0' || errno)
                break;
            else
                return &unumber;
        case CONF_TYPE_REAL:
            fnumber = strtod(val, &endptr);
            if (*(unsigned char *)endptr != '\0' || errno)
                break;
            else
                return &fnumber;
        case CONF_TYPE_STR:
            return val;
        default:
            assert(!"unknown conversion requested -- should never reach here");
            break;
    }

    errcode = CONF_ERR_TYPE;
    return NULL;
}


/*! @brief    retrieves a value with a section/variable name.
 *
 *  conf_get() retrieves a value with a section/variable name.
 *
 *  In a program (e.g., when using conf_get()), variables can be referred to using one of the
 *  following forms:
 *
 *  @code
 *      variable
 *      . variable
 *      section . variable
 *  @endcode
 *
 *  where whitespaces are optional before and after section and variable names. The first form
 *  refers to a variable belonging to the "current" section; the current section can be set by
 *  invoking conf_section(). The second form refers to a variable belonging to the global section.
 *  The last form refers to a variable belonging to a specific section.
 *
 *  @warning    A subsequent call to conf_conv() and conf_get() may overwrite the contents of the
 *              buffer pointed by the resulting pointer unless the type is @c CONF_TYPE_STR.
 *              Similarly, a subsequent call to conf_getbool(), conf_getint(), conf_getuint() and
 *              conf_getreal() may overwrite the contents of the buffer pointed by the resulting
 *              pointer.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    var    section/variable name
 *
 *  @return    pointer to storage that contains value or null pointer
 *  @retval    non-null    value retrieved
 *  @retval    NULL        failure
 */
const void *(conf_get)(const char *var)
{
    struct valnode_t *pnode;

    /* assert(var) done by valget() */

    if ((pnode = valget(var)) == NULL)
        return NULL;    /* valget() sets errcode */

    return conf_conv(pnode->val, pnode->type);    /* conf_conv() sets errcode */
}


/*! @brief    retrieves a boolean value with a section/variable name.
 *
 *  conf_getbool() retrieves a boolean value with a section/variable name. Every value for a
 *  variable is stored in a string form, and conf_getbool() converts it to a boolean value; the
 *  result is 1 (indicating true) if the string starts with 't', 'T', 'y', 'Y' or '1' ignoring any
 *  leading spaces and 0 (indicating false) otherwise. If there is no variable with the given name
 *  or the preset type of the variable is not @c CONF_TYPE_BOOL, the value of @p errval is returned.
 *
 *  For how to refer to variables in a program, see conf_get().
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    var       section/variable name
 *  @param[in]    errval    value returned as error
 *
 *  @return    converted result or @p errval
 */
int (conf_getbool)(const char *var, int errval)
{
    struct valnode_t *pnode;

    /* assert(var) done by valget() */

    pnode = valget(var);

    if (!pnode || pnode->type != CONF_TYPE_BOOL)
        return errval;    /* valget() sets errcode */

    return *(const int *)conf_conv(pnode->val, CONF_TYPE_BOOL);    /* conf_conv() sets errcode */
}


/*! @brief    retrieves an integral value with a section/variable name.
 *
 *  conf_getint() retrieves an integral value with a section/variable name. Every value for a
 *  variable is stored in a string form, and conf_getint() converts it to an integer using strtol()
 *  declared in <stdlib.h>. If there is no variable with the given name or the preset type of the
 *  variable is not @c CONF_TYPE_INT, the value of @p errval is returned.
 *
 *  For how to refer to variables in a program, see conf_get().
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    var       section/variable name
 *  @param[in]    errval    value returned as error
 *
 *  @return    converted result or @p errval
 */
long (conf_getint)(const char *var, long errval)
{
    const void *p;
    struct valnode_t *pnode;

    /* assert(val) done by valget() */

    pnode = valget(var);

    if (!pnode || pnode->type != CONF_TYPE_INT ||
        (p = conf_conv(pnode->val, CONF_TYPE_INT)) == NULL)
        return errval;    /* valget() and confconv() set errcode */

    return *(const long *)p;
}


/*! @brief    retrieves an unsigned integral value with a section/variable name.
 *
 *  conf_getuint() retrieves an unsigned integral value with a section/variable name. Every value
 *  for a variable is stored in a string form, and conf_getuint() converts it to an unsigned
 *  integer using strtoul() declared in <stdlib.h>. If there is no variable with the given name or
 *  the preset type of the variable is not @c CONF_TYPE_UINT, the value of @p errval is returned.
 *
 *  For how to refer to variables in a program, see conf_get().
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    var       section/variable name
 *  @param[in]    errval    value returned as error
 *
 *  @return    converted result or @p errval
 */
unsigned long (conf_getuint)(const char *var, unsigned long errval)
{
    const void *p;
    struct valnode_t *pnode;

    /* assert(var) done by valget() */

    pnode = valget(var);

    if (!pnode || pnode->type != CONF_TYPE_UINT ||
        (p = conf_conv(pnode->val, CONF_TYPE_UINT)) == NULL)
        return errval;    /* valget() and conf_conv() set errcode */

    return *(const unsigned long *)p;
}


/*! @brief    retrieves a real value with a section/variable name.
 *
 *  conf_getreal() retrieves a real value with a section/variable name. Every value for a variable
 *  is stored in a string form, and conf_getreal() converts it to a floating-point number using
 *  strtod() declared in <stdlib.h>. If there is no variable with the given name or the preset type
 *  of the variable is not @c CONF_TYPE_REAL, the value of @p errval is returned; @c HUGE_VAL
 *  defined <math.h> would be a nice choice for @p errval.
 *
 *  For how to refer to variables in a program, see conf_get().
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    var       section/variable name
 *  @param[in]    errval    value returned as error
 *
 *  @return    converted result or @p errval
 */
double (conf_getreal)(const char *var, double errval)
{
    const void *p;
    struct valnode_t *pnode;

    /* assert(val) done by valget() */

    pnode = valget(var);

    if (!pnode || pnode->type != CONF_TYPE_REAL ||
        (p = conf_conv(pnode->val, CONF_TYPE_REAL)) == NULL)
        return errval;    /* valget() and conf_conv() set errcode */

    return *(const double *)p;
}


/*! @brief    retrieves a string with a section/variable name.
 *
 *  conf_getstr() retrieves a string with a section/variable name. Every value for a variable is
 *  stored in a string form, thus conf_getstr() performs no conversion. If there is no variable with
 *  the given name or the preset type of the variable is not @c CONF_TYPE_STR, a null pointer is
 *  returned.
 *
 *  For how to refer to variables in a program, see conf_get().
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    var       section/variable name
 *
 *  @return    string or null pointer
 *  @retval    non-null    string retrieved
 *  @retval    NULL        failure
 */
const char *(conf_getstr)(const char *var)
{
    struct valnode_t *pnode;

    /* assert(var) done by valget() */

    pnode = valget(var);

    if (!pnode || pnode->type != CONF_TYPE_STR)
        return NULL;    /* valget() sets errcode */

    return conf_conv(pnode->val, CONF_TYPE_STR);    /* conf_conv() sets errcode */
}


/*! @brief    inserts or replaces a value associated with a variable.
 *
 *  conf_set() inserts or replaces a value associated with a variable. If conf_preset() has been
 *  invoked, conf_set() is able to only replace a value associated with an existing variable, which
 *  means an error code is returned when a user tries to insert a new variable and its value
 *  (possibly with a new section). conf_set() is allowed to insert a new variable-value pair
 *  otherwise.
 *
 *  For how to refer to variables in a program, see conf_get().
 *
 *  @warning    When conf_preset() invoked, conf_set() does not check if a given value is
 *              appropriate to the preset type of a variable. That mismatch is to be detected when
 *              conf_get() or similar functions called later for the variable.
 *
 *  Possible exceptions: assert_exceptfail, mem_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    secvar    section/variable name
 *  @param[in]    value     value to store
 *
 *  @return    success/failure indicator
 *  @retval    CONF_ERR_OK    success
 *  @retval    others         failure
 *
 *  @todo    Improvements are possible and planned:
 *           - some adjustment on arguments to table_new() is necessary.
 *
 *  @internal
 *
 *  When an existing value is replaced, conf_set() reuses the storage allocated for a struct
 *  @c valnode_t object.
 */
int (conf_set)(const char *secvar, const char *value)
{
    size_t len;
    char buf[30], *pbuf;
    char *sec, *var;
    table_t *tab;
    const char *hkey;
    struct valnode_t *pnode;

    assert(secvar);
    assert(value);

    /* sep() always clears errcode */

    len = strlen(secvar);
    pbuf = strcpy((len > sizeof(buf)-1)? MEM_ALLOC(len+1): buf, secvar);

    if (sep(pbuf, &sec, &var) != CONF_ERR_OK) {
        if (pbuf != buf)
            MEM_FREE(pbuf);
        return errcode;    /* sep sets errcode */
    }

    if (!sec) {    /* current selected */
        if (!current) {    /* no current section, thus global */
            hkey = hash_string("");    /* in case table for global not yet allocated */
            tab = table_get(section, hkey);
        } else
            tab = current;
    } else {    /* section name given */
        hkey = hash_string((control & CONF_OPT_CASE)? sec: lower(sec));
        tab = table_get(section, hkey);
    }

    if (!tab) {    /* new section */
        if (preset) {
            if (pbuf != buf)
                MEM_FREE(pbuf);
            return (errcode = CONF_ERR_SEC);
        } else {    /* adds it */
            tab = table_new(0, NULL, NULL);
            table_put(section, hkey, tab);
        }
    }

    assert(tab);

    hkey = hash_string((control & CONF_OPT_CASE)? var: lower(var));
    if (pbuf != buf)
        MEM_FREE(pbuf);

    pnode = table_get(tab, hkey);
    if (!pnode) {    /* new variable */
        if (preset)
            return (errcode = CONF_ERR_VAR);
        else {    /* inserts */
            MEM_NEW(pnode);
            pnode->type = CONF_TYPE_STR;
            table_put(tab, hkey, pnode);
        }
    } else    /* replaces, thus reuses pnode */
        MEM_FREE(pnode->freep);
    pnode->val = pnode->freep = strcpy(MEM_ALLOC(strlen(value)+1), value);

    return errcode;
}


/*! @brief    sets the current section.
 *
 *  conf_section() sets the current section to a given section. The global section can be set as the
 *  current section by giving an empty string "" to conf_section(). conf_section() affects how
 *  conf_get(), conf_getbool(), conf_getint(), confgetuint(), confgetreal(), confgetstr() and
 *  conf_set() work.
 *
 *  @param[in]    sec    section name to set as current section
 *
 *  @return    success/failure indicator
 *  @retval    CONF_ERR_OK    success
 *  @retval    others         failure
 */
int (conf_section)(const char *sec)
{
    size_t len;
    char buf[30], *pbuf;
    const char *hkey;
    table_t *tab;

    assert(sec);

    /* sepunit() always clears errcode */

    len = strlen(sec);
    pbuf = strcpy((len > sizeof(buf)-1)? MEM_ALLOC(len+1): buf, sec);

    if (sepunit(pbuf, &pbuf) != CONF_ERR_OK) {
        if (pbuf != buf)
            MEM_FREE(pbuf);
        return errcode;    /* sepunit sets errcode */
    }

    hkey = hash_string((control & CONF_OPT_CASE)? pbuf: lower(pbuf));
    if (pbuf != buf)
        MEM_FREE(pbuf);

    tab = table_get(section, hkey);
    if (tab)
        current = tab;
    else
        errcode = CONF_ERR_SEC;

    return errcode;
}


/* @brief    provides a callback for conf_free().
 *
 * conf_free() deallocates storages occupied by tables using table_map(). tabfree() provides a
 * callback for the call. To be precise, tabfree() provides two callbacks, one for deallocating
 * variable-value pair tables (the latter half of the function) and the other for deallocating
 * section tables (the former half). These two callbacks are differntiated by the value of @p cl;
 * whether @p cl is equal to @c section or not.
 *
 * @param[in]    key      not used
 * @param[in]    value    pointer to section table, or pointer to pointer to node
 * @param[in]    cl       data to distinguish which callback is invoked
 *
 * @return    nothing
 */
static void tabfree(const void *key, void **value, void *cl)
{
    if (cl == section) {    /* for section table */
        table_t *t = *value;
        table_map(t, tabfree, *value);
        table_free(&t);
    } else {    /* for var=val tables */
        struct valnode_t *pnode = *value;
        assert(pnode);
        MEM_FREE(pnode->freep);
        MEM_FREE(pnode);
    }
}


/*! @brief    deallocates the stroage for the configuration data.
 *
 *  conf_free() deallocates storages for the configuration data. After conf_free() invoked, other
 *  conf_ functions should not be called without an intervening call to conf_preset() or
 *  conf_init().
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @warning    conf_free() does not reset the hash table used internally since it may be used by
 *              other parts of the program. Invoking hash_reset() through conf_hashreset() before
 *              program termination cleans up storages occupied by the table.
 *
 *  @return    nothing
 */
void (conf_free)(void)
{
    table_map(section, tabfree, section);
    table_free(&section);
    current = NULL;
    preset = 0;
    errcode = CONF_ERR_OK;
    control = 0;
}


/*! @brief    resets the hash table using hash_reset().
 *
 *  conf_hashreset() simply calls hash_reset() to reset the hash table. As explained in conf_free(),
 *  conf_free() does not invoke hash_reset() because the single hash table may be used by other
 *  parts of a user program. Since requiring a reference to hash_reset() when using the
 *  Configuration File Library is inconsistent and inconvenient (e.g., a user code is obliged to
 *  include "hash.h"), conf_hashreset() is provided as a wrapper for hash_reset().
 *
 *  @warning    Do not forget that the effect on the hash table caused by conf_hashreset() is not
 *              limited to eliminating only what conf_ functions adds to the table; it completely
 *              cleans up the entire hash table.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: none
 *
 *  @return    nothing
 */
void (conf_hashreset)(void)
{
    hash_reset();
}


/*! @brief    returns an error code.
 *
 *  Every function in this library sets the internal error variable as it performs its operation.
 *  Unlike errno provided by <errno.h>, the error variable of this library is set to @c CONF_ERR_OK
 *  before starting an operation, thus a user code need not to clear it before calling a conf_
 *  function.
 *
 *  When using a function returning an error code (of the int type), the returned value is the same
 *  as what conf_errcode() will return if there is no intervening call to a conf_ function between
 *  them. When using a function returning a pointer, the only way to get what the error has been
 *  occurred is to use conf_errcode().
 *
 *  The following code fragment shows an example for how to use conf_errcode() and conf_errstr():
 *
 *  @code
 *      fp = fopen(conf, "r");
 *      if (!fp)
 *          fprintf(stderr, "%s:%s: %s\n", prg, conf, conf_errstr(CONF_ERR_FILE));
 *      line = conf_init(fp, CONF_OPT_CASE | CONF_OPT_ESC);
 *      if (line != 0)
 *          fprintf(stderr, "%s:%s:%lu: %s\n", prg, conf, line, conf_errstr(conf_errcode()));
 *  @endcode
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: none
 *
 *  @return    current error code
 *
 *  @internal
 *
 *  The policy on clearing and setting @c errcode is that, if a function sets @c errcode it should
 *  also clears it at its start whether it has the internal linkage or the external one.
 */
int (conf_errcode)(void)
{
    return errcode;
}


/*! @brief    returns an error message.
 *
 *  conf_errstr() returns an error message for a given error code.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: none
 *
 *  @param[in]    code    error code for which error message returned
 *
 *  @return    error message
 */
const char *(conf_errstr)(int code)
{
    static const char *s[] = {
        "everything is okay",                /* CONF_ERR_OK */
        "file not found",                    /* CONF_ERR_NOFILE */
        "I/O error occurred",                /* CONF_ERR_IO */
        "space in section/variable name",    /* CONF_ERR_SPACE */
        "invalid character encountered",     /* CONF_ERR_CHAR */
        "invalid line encountered",          /* CONF_ERR_LINE */
        "no following line for splicing",    /* CONF_ERR_BSLASH */
        "section not found",                 /* CONF_ERR_SEC */
        "variable not found",                /* CONF_ERR_VAR */
        "data type mismatch",                /* CONF_ERR_TYPE */
    };

    assert(CONF_ERR_MAX == sizeof(s)/sizeof(*s));
    assert(code >= 0 || code < sizeof(s)/sizeof(*s));

    return s[code];
}


#if 0    /* test code */
#include <assert.h>    /* assert */
#include <stddef.h>    /* size_t, NULL */
#include <stdio.h>     /* FILE, fopen, fclose, printf, fprintf, stderr, puts, rewind, EOF */
#include <stdlib.h>    /* EXIT_FAILURE */

#include "conf.h"    /* conf_t, CONF_TYPE_*, CONF_ERR_*, conf_preset, conf_init, conf_get,
                        conf_errset, conf_errcode, conf_section, conf_set, conf_free */

static const char *cfname = "test.conf";

static void print(const char *var, int type, int preset)
{
    const void *p;
    const char *boolean[] = { "true", "false" };

    assert(var);

    p = conf_get(var);
    if (!p)
        fprintf(stderr, "test: %s - %s\n", var, conf_errstr(conf_errcode()));
    else {
        if (!preset)
            switch(type) {
                case CONF_TYPE_BOOL:
                    printf("%s = %s\n", var, boolean[*(const int *)p]);
                    break;
                case CONF_TYPE_INT:
                    printf("%s = %ld\n", var, *(const long *)p);
                    break;
                case CONF_TYPE_UINT:
                    printf("%s = %lu\n", var, *(const unsigned long *)p);
                    break;
                case CONF_TYPE_REAL:
                    printf("%s = %e\n", var, *(const double *)p);
                    break;
                case CONF_TYPE_STR:
                    printf("%s = %s\n", var, (const char *)p);
                    break;
                default:
                    assert(!"invalid type -- should never reach here");
                    break;
            }
        else    /* ignores type and always assumes CONF_TYPE_STR */
            printf("%s = %s\n", var, (const char *)p);
    }
}

int main(void)
{
    conf_t tab[] = {
        /* global section */
        "VarBool",          CONF_TYPE_BOOL, "yes",
        "VarInt",           CONF_TYPE_INT,  "255",
        "VarUint",          CONF_TYPE_UINT, "0xFFFF",
        "VarReal",          CONF_TYPE_REAL, "3.14159",
        "VarStr",           CONF_TYPE_STR,  "Global VarStr Default",
        /* Section1 */
        "Section1.VarBool", CONF_TYPE_BOOL, "FALSE",
        "Section1.VarInt",  CONF_TYPE_INT,  "254",
        "Section1.VarUint", CONF_TYPE_UINT, "0xFFFE",
        "Section1.VarReal", CONF_TYPE_REAL, "3.14160",
        "Section1.VarStr",  CONF_TYPE_STR,  "Section1.VarStr Default",
        /* Section2 */
        "Section2.VarBool", CONF_TYPE_BOOL, "TRUE",
        "Section2.VarInt",  CONF_TYPE_INT,  "253",
        "Section2.VarUint", CONF_TYPE_UINT, "0xFFFD",
        "Section2.VarReal", CONF_TYPE_REAL, "3.14161",
        "Section2.VarStr",  CONF_TYPE_STR,  "Section2.VarStr Default",
        /* Section3 */
        "Section3.VarBool", CONF_TYPE_BOOL, "1",
        "Section3.VarInt",  CONF_TYPE_INT,  "252",
        "Section3.VarUint", CONF_TYPE_UINT, "0xFFFC",
        "Section3.VarReal", CONF_TYPE_REAL, "3.14162",
        "Section3.VarStr",  CONF_TYPE_STR,  "Section3.VarStr Default",
        NULL,
    };
    int i;
    size_t line;
    FILE *fp;
    const char *s = "# configuration file example\n"
                    "\n"
                    "\t[\tSection1]    # Section 1\n"
                    "VarReal      \\\n"
                    "    = 0.314\\\n"
                    "159\n"
                    "\n"
                    "\n"
                    "[Section2\t]\t; Section 2\n"
                    "\n"
                    "        Var\\\n"
                    "Str = \"Section#2.VarStr\\tNew\\\n"
                    "\t    default\"     ; set VarStr of Section 2\n"
                    "\n"
                    "[       ]    # global section\n"
                    "VarUint     =      \"010\"\n"
                    "VarStr=\"\"\n"
                    "VarStr=This does not use \"\t# unquoted value\n";

    if (conf_preset(tab, CONF_OPT_CASE | CONF_OPT_ESC) != CONF_ERR_OK) {
        fprintf(stderr, "test: %s\n", conf_errstr(conf_errcode()));
        conf_free();
        conf_hashreset();
        return EXIT_FAILURE;
    }

    for (i = 0; i < 2; i++) {
        fp = fopen(cfname, "r");
        if (!fp) {    /* creates a configuration file */
            fp = fopen(cfname, "w+");
            assert(fp);
            assert(fputs(s, fp) != EOF);
            rewind(fp);
        }

        /* starts testing */
        line = conf_init(fp, (i == 0)? CONF_OPT_CASE | CONF_OPT_ESC: 0);
        fclose(fp);    /* fp is not necessary any more */

        if (line != 0) {
            fprintf(stderr, "test:%s:%ld: %s\n", cfname, (unsigned long)line,
                    conf_errstr(conf_errcode()));
            conf_free();
            conf_hashreset();
            return EXIT_FAILURE;
        }

        printf("%s* Testing in %spreset mode with%s CONF_OPT_CASE and CONF_OPT_ESC...\n",
               (i == 0)? "": "\n", (i == 0)? "": "non-", (i == 0)? "": "out");

        puts("\n[global section]");
        print("VarBool", CONF_TYPE_BOOL, i);
        print("VarInt", CONF_TYPE_INT, i);
        print("VarUint", CONF_TYPE_UINT, i);
        print("VarReal", CONF_TYPE_REAL, i);
        print("VarStr", CONF_TYPE_STR, i);

        puts("");
        print("Section1.VarBool", CONF_TYPE_BOOL, i);
        print("Section1.VarInt", CONF_TYPE_INT, i);
        print("Section1.VarUint", CONF_TYPE_UINT, i);
        print("Section1.VarReal", CONF_TYPE_REAL, i);
        print("Section1.VarStr", CONF_TYPE_STR, i);

        printf("\n[switching to Section2 - %s]\n",
               (conf_section("Section2") == CONF_ERR_OK)?  "done": "failed");
        print("VarBool", CONF_TYPE_BOOL, i);
        print("VarInt", CONF_TYPE_INT, i);
        print("VarUint", CONF_TYPE_UINT, i);
        print("VarReal", CONF_TYPE_REAL, i);
        print("VarStr", CONF_TYPE_STR, i);

        puts("");
        print("Section3.VarBool", CONF_TYPE_BOOL, i);
        print("Section3.VarInt", CONF_TYPE_INT, i);
        print("Section3.VarUint", CONF_TYPE_UINT, i);
        print("Section3.VarReal", CONF_TYPE_REAL, i);
        print("Section3.VarStr", CONF_TYPE_STR, i);

        printf("\n[setting VarStr - %s]\n",
               (conf_set("VarStr", "Set in run-time") == CONF_ERR_OK)? "done": "failed");
        printf("[setting Section4.VarReal - %s]\n",
               (conf_set("Section4.VarReal", "2.718281") == CONF_ERR_OK)? "done": "failed");

        print("VarStr", CONF_TYPE_STR, i);
        print("Section4.VarReal", CONF_TYPE_REAL, i);

        conf_free();
        conf_hashreset();    /* conf_free() does not reset hash table */
    }

    return 0;
}
#endif    /* disabled */

/* end of conf.c */
