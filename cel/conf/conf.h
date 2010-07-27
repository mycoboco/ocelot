/*!
 *  @file    conf.h
 *  @brief    Header for Configuration File Library (CEL)
 */

#ifndef CONF_H
#define CONF_H


/*! @struct    conf_t    conf.h
 *  @brief    represents an element of a configuration description table.
 *
 *  @c conf_t represents an element of a configuration description table that is used for a user
 *  program to specify a set of sections and variable including their types and default values; a
 *  configuration table is the only way to specify types of variables as having other than
 *  @c CONF_TYPE_STR (string type).
 *
 *  The @c var member specifies a section/variable name. The string has one of the following two
 *  forms:
 *
 *  @code
 *      variable
 *      section . variable
 *  @endcode
 *
 *  where whitespaces are allowed before and/or after a section and variable name. The first form
 *  refers to a variable in the global section; there is no concept of the "current" section yet
 *  because conf_section() cannot be invoked before conf_preset() or conf_init(). To mark the end of
 *  a table, set the @c var member to a null pointer.
 *
 *  The @c type member specifies the type of a variable and should be one of @c CONF_TYPE_BOOL
 *  (boolean value, int), @c CONF_TYPE_INT (signed integer, long), @c CONF_TYPE_UINT (unsigned
 *  integer, unsigned long), @c CONF_TYPE_REAL (floating-point number, double), and
 *  @c CONF_TYPE_STR (string, char *). Once a variable is set to have a type, there is no way to
 *  change its type; thus, if a variable is supposed to have various types depending on the context,
 *  set to @c CONF_TYPE_STR and use conf_conv(). For @c OPT_TYPE_INT and @c OPT_TYPE_UINT, the
 *  conversion of a given value recognizes the C-style prefixes; numbers starting with 0 are treated
 *  as octal, and those with 0x or 0X are treated as hexadecimal.
 *
 *  The @c defval member specifies a default value for a variable that is used when a configuration
 *  file dose not set that variable to a value. It cannot be a null pointer but an empty string.
 *  Note that conf_preset() that accepts a configuration description table does not check if a
 *  default value has a proper form for the type of a variable.
 *
 *  See the commented-out example code given in the source file for more about a configuration
 *  description table.
 */
typedef struct conf_t {
    char *var;       /*!< section name and variable name */
    int type;        /*!< type of variable */
    char *defval;    /*!< default value */
} conf_t;

/*! @brief    defines enum constants for types of values.
 */
enum {
    CONF_TYPE_NO,      /* cannot have type (not used in this library) */
    CONF_TYPE_BOOL,    /*!< has boolean (int) type */
    CONF_TYPE_INT,     /*!< has integer (long) type */
    CONF_TYPE_UINT,    /*!< has unsigned integer (unsigned long) type */
    CONF_TYPE_REAL,    /*!< has floating-point (double) type */
    CONF_TYPE_STR      /*!< has string (char *) type */
};

/*! @brief    defines enum constants for error codes.
 */
enum {
    CONF_ERR_OK,        /*!< everything is okay */
    CONF_ERR_FILE,      /*!< file not found */
    CONF_ERR_IO,        /*!< I/O error occurred */
    CONF_ERR_SPACE,     /*!< space in section/variable name */
    CONF_ERR_CHAR,      /*!< invalid character encountered */
    CONF_ERR_LINE,      /*!< invalid line encountered */
    CONF_ERR_BSLASH,    /*!< no following line for slicing */
    CONF_ERR_SEC,       /*!< section not found */
    CONF_ERR_VAR,       /*!< variable not found */
    CONF_ERR_TYPE,      /*!< data type mismatch */
    CONF_ERR_MAX        /* number of error codes */
};

/*! @brief    defines masks for control options.
 */
enum {
    CONF_OPT_CASE = 0x01,                /*!< case-sensitive variable/section name */
    CONF_OPT_ESC = CONF_OPT_CASE << 1    /*!< supports escape sequence in quoted value */
};


/*! @name    configuration initializing functions:
 */
/*@{*/
int conf_preset(const conf_t *, int);
size_t conf_init(FILE *, int);
void conf_free(void);
void conf_hashreset(void);
/*@}*/

/*! @name    configuration data-handling functions:
 */
/*@{*/
const void *conf_conv(const char *, int);
const void *conf_get(const char *);
int conf_getbool(const char *, int);
long conf_getint(const char *, long);
unsigned long conf_getuint(const char *, unsigned long);
double conf_getreal(const char *, double);
const char *conf_getstr(const char *);
int conf_set(const char *, const char *);
int conf_section(const char *);
/*@}*/

/*! @name    error handling functions:
 */
/*@{*/
int conf_errcode(void);
const char *conf_errstr(int);
/*@}*/


#endif    /* CONF_H */

/* end of conf.h */
