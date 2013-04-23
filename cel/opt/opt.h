/*!
 *  @file    opt.h
 *  @brief    Header for Option Parsing Library (CEL)
 */

#ifndef OPT_H
#define OPT_H


/*! @struct    opt_t    opt.h
 *  @brief    represents an element of an option description table.
 *
 *  @c opt_t represents an element of an option description table that is used for a user program
 *  to specify options and their properties. A option description table is an array of @c opt_t,
 *  each element of which is consisted of four members, two of which have overloaded meanings:
 *  - @c lopt: long-named option that can be invoked by two precedeeing hypens; optional if a
 *             short-named option given; however, encouraged to always provide a long-named option
 *  - @c sopt: short-named option that can be invoked by a precedeeing hypens; optional if both a
 *             long-named option and a flag variable provided
 *  - @c flag: if an option does not take an additional argument, @c flag can point to an object
 *             (called "flag variable") that is set to the value of @c arg when @c lopt or @c sopt
 *             option encountered; if an option can take an additional argument, @c flag specifies
 *             whether the option-argument is mandatory (with @c OPT_ARG_REQ) or optional (with
 *             @c OPT_ARG_OPT)
 *  - @c arg: if an option does not take an additional argument, @c arg has the value to be stored
 *            into a flag variable when @c lopt or @c sopt option encountered; if an option can
 *            take an additional argument, @c arg specifies the type of the option-argument using
 *            @c OPT_TYPE_BOOL (option-arguments starting with 't', 'T', 'y', 'Y' and '1' means
 *            true and others false, int), @c OPT_TYPE_INT (signed integer, long), @c OPT_TYPE_UINT
 *            (unsigned integer, unsigned long), @c OPT_TYPE_REAL (floating-point number, double)
 *            and @c OPT_TYPE_STR (string, char *)
 *
 *  To mark an end of the table, the @c lopt member of the last element has to be set to a null
 *  pointer. If the @c flag member points to a flag variable, the pointed integer object is
 *  initalized to be 0 by opt_init().
 *
 *  For @c OPT_TYPE_INT and @c OPT_TYPE_UINT, the conversion of a given option-argument recognizes
 *  the C-style prefixes; numbers starting with 0 are treated as octal, and those with 0x or 0X are
 *  treated as hexadecimal.
 *
 *  Some examples follow:
 *
 *  @code
 *      opt_t options[] = {
 *          { "verbose", 'v', &option_verbose, 1 },
 *          { "brief",   'b', &option_verbose, 0 },
 *          { NULL, }
 *      };
 *  @endcode
 *
 *  This example says that two options ("--verbose" or "-v" and "--brief" or "-b") are recognized
 *  and @c option_verbose is set to 1 when "--verbose" or "-v" given, and set to 0 when "--brief"
 *  or "-b" given.
 *
 *  @code
 *      opt_t options[] = {
 *          "version",  'v',         OPT_ARG_NO, OPT_TYPE_NO,
 *          "help",     UCHAR_MAX+1, OPT_ARG_NO, OPT_TYPE_NO,
 *          "morehelp", UCHAR_MAX+2, OPT_ARG_NO, OPT_TYPE_NO,
 *          NULL,
 *      };
 *  @endcode
 *
 *  This example shows options that do not take any additional arguments. Setting the @c flag
 *  member to a null pointer also says the option takes no argument in which case the value of the
 *  @c arg member ignored. Thus, the above example can be written as follows without any change on
 *  the behavior:
 *
 *  @code
 *      opt_t options[] = {
 *          "version",  'v',         NULL, 0,
 *          "help",     UCHAR_MAX+1, NULL, 0,
 *          "morehelp", UCHAR_MAX+2, NULL, 0,
 *          NULL,
 *      };
 *  @endcode
 *
 *  where you can put any integer in the place of 0. The former is preferred, however, since it
 *  shows more explicitly the fact that no additional arguments consumed after each of the options.
 *
 *  When only long-named options need to be provided without introducing flag variables, values
 *  from @c UCHAR_MAX+1 to @c INT_MAX (inclusive) can be used for the @c sopt member; both are
 *  defined in <limits.h>. (Even if the C standard does not require @c UCHAR_MAX less than
 *  @c INT_MAX, many parts of C, especially, of the standard library cannot work correctly without
 *  such a relationship on a hosted implementation.)
 *
 *  @code
 *      opt_t options[] = {
 *          "",   'x', OPT_ARG_NO, OPT_TYPE_NO,
 *          NULL,
 *      };
 *  @endcode
 *
 *  On the other hand, providing an empty string for the @c lopt member as in this example can
 *  specify that an option is only short-named. Note that, however, this is discouraged; long-named
 *  options are much more user-friendly especially for novices.
 *
 *  @code
 *      opt_t options[] = {
 *          "input", 'i', OPT_ARG_REQ, OPT_TYPE_STR,
 *          "port",  'p', OPT_ARG_REQ, OPT_TYPE_UINT,
 *          "start", 's', OPT_ARG_REQ, OPT_TYPE_REAL,
 *          "end",   'e', OPT_ARG_REQ, OPT_TYPE_REAL,
 *          NULL,
 *      };
 *  @endcode
 *
 *  This example shows options that take additional arguments. @c OPT_ARG_REQ for the @c flag
 *  member specifies that the option requires an option-argument and that the type of the argument
 *  is given in the @c arg member. For @c OPT_TYPE_INT, @c OPT_TYPE_UINT and @c OPT_TYPE_REAL,
 *  strtol(), strtoul() and strtod() are respectively used to convert option-arguments.
 *
 *  @code
 *      opt_t options[] = {
 *          "negative", 'n', OPT_ARG_OPT, OPT_TYPE_REAL,
 *          NULL,
 *      };
 *  @endcode
 *
 *  This table specifies the option "--negative" or "-n" takes an optionally given argument. If an
 *  option-argument with the expected form (which is determined by strtod() in this case) follows
 *  the option, it is taken. If there is no argument, or is an argument but has no expected form,
 *  the option works as if @c OPT_ARG_OPT and @c OPT_TYPE_REAL are replaced by @c OPT_ARG_NO and
 *  @c OPT_TYPE_NO.
 *
 *  The following examples show how to control the ordering mode.
 *
 *  @code
 *      opt_t options[] = {
 *          "+", 0, OPT_ARG_NO, OPT_TYPE_NO,
 *          ...
 *          NULL,
 *      };
 *  @endcode
 *
 *  Setting the first long-named option to "+" or setting the environment variable named
 *  @c POSIXLY_CORRECT says option processing performed by opt_parse() immediately stops whenever
 *  an operand encountered (which POSIX requires).
 *
 *  @code
 *      opt_t options[] = {
 *          "-", 0, OPT_ARG_NO, OPT_TYPE_NO,
 *          ...
 *          NULL,
 *      };
 *  @endcode
 *
 *  In addition, setting the first long-named option to "-" makes opt_parse() returns the character
 *  valued 1 when encounters an operand as if the operand is an option-argument for the option
 *  whose short name has the value 1.
 */
typedef struct opt_t {
    char *lopt;    /*!< long-named option (optional for some cases) */
    int sopt;      /*!< short-named option (optional for some cases) */
    int *flag;     /*!< pointer to flag varible or information about additional argument */
    int arg;       /*!< value for flag variable or type of additional argument */
} opt_t;

/*! @brief    defines enum contants for types of argument conversion.
 */
enum {
    OPT_TYPE_NO,      /*!< cannot have type */
    OPT_TYPE_BOOL,    /*!< has boolean (int) type */
    OPT_TYPE_INT,     /*!< has integer (long) type */
    OPT_TYPE_UINT,    /*!< has unsigned integer (unsigned long) type */
    OPT_TYPE_REAL,    /*!< has floating-point (double) type */
    OPT_TYPE_STR      /*!< has string (char *) type */
};


/* provides unique address for OPT_ARG_ macros */
extern int opt_arg_req, opt_arg_no, opt_arg_opt;


/*! @name    option processing functions:
 */
/*@{*/
const char *opt_init(const opt_t *, int *, char **[], const void **, const char *, int);
int opt_parse(void);
void opt_abort(void);
const char *opt_errmsg(int);
void opt_free(void);
/*@}*/


/*! @name    macros for describing option-arguments; see @c opt_t
 */
/*@{*/
#define OPT_ARG_REQ (&opt_arg_req)    /*!< mandatory argument */
#define OPT_ARG_NO  (&opt_arg_no)     /*!< no argument taken */
#define OPT_ARG_OPT (&opt_arg_opt)    /*!< optional argument */
/*@}*/


#endif    /* OPT_H */

/* end of opt.h */
