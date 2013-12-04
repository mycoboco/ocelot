/*!
 *  @file    opt.c
 *  @brief    Source for Option Parsing Library (CEL)
 */

#include <assert.h>    /* assert */
#include <ctype.h>     /* isprint, isspace */
#include <errno.h>     /* errno */
#include <limits.h>    /* CHAR_BIT */
#include <stddef.h>    /* NULL */
#include <stdio.h>     /* sprintf */
#include <string.h>    /* memcpy, strrchr, strlen, strchr, strncmp */
#include <stdlib.h>    /* strtol, strtoul, strtod, malloc, free, getenv */

#include "opt.h"


/* @brief    provides a short-hand notation for conversion to unsigned char pointer.
 *
 * To allow for characters whose value cannot be represented by "plain" char (when the char type is
 * signed) an access to objects containing characters should be done carefully; for example,
 * locating the terminating null character in a string. UC() is used for such an access. The
 * problems for which UC() matters are too complicated to explain precisely here.
 */
#define UC(x) ((unsigned char *)(x))

/* @brief    normalizes characters for space.
 *
 * normal() allows considering non-space charaters to be spaces by normalizing them. See opt_val()
 * for why this is necessary.
 */
#define normal(c) (((c) == ' ' || (c) == '_')? '-': (c))


/* @brief    defines enum constants for the kind of options.
 */
enum {
    INVALID,     /* invalid */
    DMINUS,      /* -- */
    SHORTOPT,    /* -f... */
    LONGOPT,     /* --f... */
    OPERAND      /* operand */
};

/* @brief    specifies the ordering mode in which opt_parse() works.
 *
 * The enum constants have the same names as those used in an implementation of getopt().
 */
enum {
    PERMUTE,           /* argument permutation performed, default mode */
    REQUIRE_ORDER,     /* POSIX-compliant mode */
    RETURN_IN_ORDER    /* operands act as if be option-arguments for '\001' option */
} order;


/* @brief    provides unique storage for @c OPT_ARG_ macros.
 *
 * To override the meaning of the @c flag member of struct @c opt_t, @c OPT_ARG_ macros have to be
 * of the pointer type, thus it is necessary to provide unique objects to which those pointers
 * point.
 */
int opt_arg_req, opt_arg_no, opt_arg_opt;


/* @brief    points to the option description table provided by a user.
 */
static const opt_t *opt;

/* @brief    points to @c argc provided by a user.
 */
static int *pargc;

/* @brief    points to a copy of @c argv.
 *
 * In order to keep the contents of the original @c argv array, opt_init() copies @c argv and
 * @c pargv points to that copy.
 */
static char ***pargv;

/* @brief    points to the pointer to an object through which an option-argument passed.
 */
static const void **parg;

/* @brief    points to a short-named option to see next in grouped options.
 *
 * Short-named options may be given grouped as in "-abc" that is equivalent to "-a -b -c". After
 * opt_parse() has reported to its caller that '-a' encountered, opt_parse() called again has to
 * resume at the option 'b' not at the next program argument if any. @c nopt points to the next
 * short-named option where opt_parse() resumes. @c nopt being a null pointer means opt_parse()
 * sees the next program argument if any.
 */
static const char *nopt;

/* @brief    indicates that all remaining arguments are recognized as operands.
 *
 * Encountering an operand in the POSIX-compliant mode or "--" (when @c oprdflag is to be set)
 * makes all remaining arguments recognized as operands.
 */
static int oprdflag;

/* @brief    locates the place into which a pointer to the next operand copied.
 */
static int oargc;


/* @brief    determines the kind of a program argument.
 *
 * argcheck() determines the kind of an argument given in @p arg. An argument is categorized as a
 * short-named option if it starts with a single hyphen and a non-hyphen character follows (when
 * @c SHORTOPT returned), as a long-named option if it starts with double hyphens and a character
 * follows (when @c LONGOPT returned), as a special mark to indicate all following arguments should
 * be recognized as operands (when @c DMINUS returned), or as an operand (when @c OPERAND
 * returned).
 *
 * An option of the form "--" where no character immediately follows makes argcheck() always return
 * @c OPERAND for any remaining arguments; this enables a user to access a file with a name
 * starting with a hyphen character especially when argument permutation performed; see @c opt_t
 * for argument permutation.
 *
 * Besides, if the POSIX-compliant behavior is requested (when @c order is @c REQUIRE_ORDER),
 * encountering the first operand also makes argcheck() return @c OPERAND for all remaining
 * arguments.
 *
 * @param[in]    arg    program argument to categorize
 *
 * @return    type of program argument
 * @retval    INVALID     invalid option (null or an empty string given)
 * @retval    DMINUS      -- encountered; @c OPERAND returned for remaining arguments
 * @retval    LONGOPT     long-named option starting with --
 * @retval    SHORTOPT    short-named option starting with -
 * @retval    OPERAND     operand
 */
static int argcheck(const char *arg)
{
    if (!arg || UC(arg)[0] == '\0')
        return INVALID;

    if (!oprdflag && arg[0] == '-') {    /* -... */
        if (arg[1] == '-') {    /* --... */
            if (UC(arg)[2] == '\0') {    /* -- */
                oprdflag = 1;
                return DMINUS;
            } else    /* --f... */
                return LONGOPT;
        } else if (UC(arg)[1] == '\0')    /* - */
            return OPERAND;
        else    /* -f... */
            return SHORTOPT;
    } else
        return OPERAND;
}


/* @brief    converts an option-argument based on a type.
 *
 * argconv() converts an option-argument to an integer or floating-point number as requested.
 * @p type should be @c OPT_TYPE_BOOL (which recognizes some forms of boolean values),
 * @c OPT_TYPE_INT (which indicates conversion to signed long int), @c OPT_TYPE_UINT (conversion to
 * unsigned long int), @c OPT_TYPE_REAL (conversion to double) or @c OPT_TYPE_STR (no conversion
 * necessary). The expected forms for @c OPT_TYPE_INT, @c OPT_TYPE_UINT and @c OPT_TYPE_REAL are
 * respectively those for strtol(), strtoul() and strtod(). @c OPT_TYPE_BOOL gives 1 for a string
 * starting with 't', 'T', 'y', 'Y', '1' and 0 for others. argconv() returns a pointer to the
 * storage that contains the converted value (an integer, floating-point number or string) and its
 * caller (user code) has to convert the pointer properly (to const int *, const long *, const
 * unsigned long *, const double * and const char *) before use. If the conversion fails, argconv()
 * returns a null pointer.
 *
 * @warning    A subsequent call to argconv() may overwrite the contents of the buffer pointed to
 *             by the resulting pointer unless type is @c OPT_TYPE_STR.
 *
 * @param[in]    arg     option-argument to convert
 * @param[in]    type    type based on which conversion performed
 *
 * @return    pointer to storage that contains result or null pointer
 * @retval    non-null    pointer to conversion result
 * @retval    null        conversion failure
 */
static const void *argconv(const char *arg, int type)
{
    /* provides storage for conversion result */
    static int boolean;              /* for OPT_TYPE_BOOL */
    static long inumber;             /* for OPT_TYPE_INT */
    static unsigned long unumber;    /* for OPT_TYPE_UINT */
    static double fnumber;           /* for OPT_TYPE_REAL */

    char *endptr;

    assert(arg);

    errno = 0;
    switch(type) {
        case OPT_TYPE_BOOL:
            while (*UC(arg) != '\0' && isspace(*UC(arg)))    /* ignores leading spaces */
                arg++;
            if (*arg == 't' || *arg == 'T' || *arg == 'y' || *arg == 'Y' || *arg == '1')
                boolean = 1;
            else
                boolean = 0;
            return &boolean;
        case OPT_TYPE_INT:
            inumber = strtol(arg, &endptr, 0);
            return (*UC(endptr) != '\0' || errno)? NULL: &inumber;
        case OPT_TYPE_UINT:
            unumber = strtoul(arg, &endptr, 0);
            return (*UC(endptr) != '\0' || errno)? NULL: &unumber;
        case OPT_TYPE_REAL:
            fnumber = strtod(arg, &endptr);
            return (*UC(endptr) != '\0' || errno)? NULL: &fnumber;
        case OPT_TYPE_STR:
            return arg;
        default:
            assert(!"unknown conversion requested -- should never reach here");
            break;
    }

    return NULL;
}


/* @brief    checks consistency of an option description table.
 *
 * chckvalid() inspects validity of an option description table. The requirements checked are:
 * - if the first character of the first @c lopt member is '+', the member has to be "+" and its
 *   @c sopt member be 0,
 * - if the first character of the first @c lopt member is '-', the member has to be "-" and its
 *   @c sopt member be 0,
 * - the @c sopt member has a non-negative value,
 * - the @c sopt member is not allowed to have '?', '-', '+', '*', '=', and -1,
 * - the @c sopt member is allowed to have 0 only when the @c lopt member is not empty and a flag
 *   variable provided,
 * - the @c lopt member is not allowed to contain '=', and
 * - if an option takes an option-argument, its type has to be specified.
 *
 * chckvalid() is called by opt_init() if @c NDEBUG is not defined.
 *
 * @param[in]    o    option description table to inspect
 *
 * @return    nothing
 */
static void chckvalid(const opt_t *o)
{
    unsigned char *p;

    assert(o);

    assert(!o->lopt || (o->lopt[0] != '+' && o->lopt[0] != '-') ||
           (UC(o->lopt)[1] == '\0' && (o++)->sopt == 0));
    for ( ; o->lopt; o++) {
        assert(o->sopt >= 0);
        assert(o->sopt != '?' && o->sopt != '-' && o->sopt != '+' && o->sopt != '*' &&
               o->sopt != '=' && o->sopt != -1);
        assert(o->sopt != 0 || (UC(o->lopt)[0] != '\0' && o->flag && o->flag != OPT_ARG_NO &&
                                o->flag != OPT_ARG_REQ && o->flag != OPT_ARG_OPT));
        for (p = (unsigned char *)o->lopt; *p; p++)
            assert(*p != '=');
        assert(!(o->flag == OPT_ARG_REQ || o->flag == OPT_ARG_OPT) ||
               (o->arg == OPT_TYPE_BOOL || o->arg == OPT_TYPE_INT || o->arg == OPT_TYPE_UINT ||
                o->arg == OPT_TYPE_REAL || o->arg == OPT_TYPE_STR));
    }
}


/*! @brief    prepares to start parsing program arguments.
 *
 *  opt_init() prepares to start parsing program arguments. It takes everything necessary to parse
 *  arguments and sets the internal state properly that is referred to by opt_parse() later. It
 *  also constructs a more readable program name by omitting any path preceeding the pure name. To
 *  do this job, it takes a directory separator character through @p sep and a default program name
 *  through @p name that is used when no name is available through @c argv. A typical use of
 *  opt_init() is given at the commented-out example code in the source file.
 *
 *  On success, opt_init() returns a program name (non-null pointer). On failure, it returns the
 *  null pointer; opt_init() may fail only when allocating small-sized storage fails, in which case
 *  further execution of the program is very likely to fail due to the same problem.
 *
 *  opt_init() can be called again for multiple scans of options, but only after opt_free() has
 *  been invoked. Note that, in such a case, only the internal state and flag variables given with
 *  an option description table are initialized. Other objects probably used for processing options
 *  in a user code retain their values, thus should be initialized explicitly by a user code. A
 *  convenient way to handle that initialization is to introduce a structure grouping all such
 *  objects. For example:
 *
 *  @code
 *      struct option {
 *          int html;
 *          const char *input;
 *          double val;
 *          ...
 *      } option;
 *  @endcode
 *
 *  where, say, @c html is a flag variable for --html, @c input is an argument for -i or --input,
 *  @c val is an argument for -n or --number, and so on. By assigning a properly initialized value
 *  to the structure, the initialization can be readily done:
 *
 *  @code
 *      For C90:
 *          struct option empty = { 0, };
 *          option = empty;
 *
 *      For C99:
 *          option = (struct option){ 0, };
 *  @endcode
 *
 *  Note that, in this example, the object @c option should have the static storage duration in
 *  order for the @c html member to be given as an initalizer for an option description table; C99
 *  has no such restriction.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: non-null but invalid arguments given
 *
 *  @param[in]    o       option description table
 *  @param[in]    pc      pointer to @c argc
 *  @param[in]    pv      pointer to @c argv
 *  @param[in]    pa      pointer to object to contain argument or erroneous option
 *  @param[in]    name    default program name
 *  @param[in]    sep     directory separator (e.g., '/' on UNIX-like systems)
 *
 *  @return    program name or null pointer
 *  @retval    non-null    program name
 *  @retval    null        failure
 */
const char *(opt_init)(const opt_t *o, int *pc, char **pv[], const void **pa, const char *name,
                       int sep)
{
    char **argv;
    const char *p = NULL;

    assert(o);
    assert(pc);
    assert(pv);
    assert(pa);
    assert(name);
    assert(sep != '\0');

    assert(!opt);
    assert(!pargc);
    assert(!pargv);
    assert(!parg);

#if !defined(NDEBUG)
    chckvalid(o);
#endif    /* !NDEBUG */

    argv = malloc((*pc+1) * sizeof(*argv));
    if (!argv)
        return NULL;
    memcpy(argv, *pv, (*pc+1) * sizeof(*argv));
    *pv = argv;

    order = PERMUTE;
    opt = o;
    pargc = pc;
    pargv = pv;    /* pargv has copy of argv */
    parg = pa;
    nopt = NULL;
    oprdflag = 0;
    oargc = 1;

    for (; o->lopt; o++)    /* initializes flag variables */
        if (o->flag && o->flag != OPT_ARG_REQ && o->flag != OPT_ARG_NO && o->flag != OPT_ARG_OPT)
            *o->flag = 0;

    if (*pargc > 0) {
        if (UC((*pargv)[0])[0] != '\0') {    /* program name exists */
            p = strrchr((*pargv)[0], (unsigned char)sep);
            if (p)
                p++;
            else
                p = (*pargv)[0];
        } else
            p = name;    /* defaults to user-given name */
        *pargc = 0;    /* increases as opt_parse() parses options */
    } else
        *pargc = -1;    /* no program name and options available */

    if (getenv("POSIXLY_CORRECT") || opt->lopt[0] == '+') {
        order = REQUIRE_ORDER;
        opt++;
    } else if (opt->lopt[0] == '-') {
        order = RETURN_IN_ORDER;
        opt++;
    }    /* else order = PERMUTE; */

    return p;
}


/* @brief    constructs a string to contain an erroneous short-named option.
 *
 * Since it is possible to give a short-named option whose code value does not correspond to a
 * visible character (for which isprint() returns non-zero), errsopt() constructs a string to
 * represent the option in question properly. The returned string can be used in user code to make
 * a diagnostic message. A subsequent call to errsopt() may overwrite the contents of the buffer
 * for the string.
 *
 * @param[in]    sopt    short-named option with which a string constructed
 *
 * @return    string containing the option in question
 */
static const char *errsopt(int sopt)
{
    static char msg[(CHAR_BIT+3)/4+4];    /* -<??> */

    sprintf(msg, (isprint((unsigned char)sopt))? "-%c": "-<%02X>", (unsigned char)sopt);

    return msg;
}


/* @brief    constructs a string to contain an erroneous long-named option.
 *
 * errlopt() constrcuts a string to represent a long-named option. errlopt() simply adds the "--"
 * prefix when an option in @p lopt is from the option description table. On the other hand, for
 * those from the program arguments errlopt() also drops characters beyond an equal sign if any.
 * If the resulting string is too long to contain in an internal buffer, its part is omitted with
 * an ellipsis.
 *
 * @param[in]    lopt    long-named option with which a string constructed
 *
 * @return    string containing the option in question
 */
static const char *errlopt(const char *lopt)
{
    static char msg[42+1];

    const char *p;

    assert(lopt);

    for (p = lopt; *UC(p) != '\0' && *p != '=' && p-lopt < sizeof(msg)-3; p++)
        continue;

    sprintf(msg, "--%.*s%s", (int)(p-lopt), lopt, (*UC(p) != '\0' && *p != '=')? "...": "");

    return msg;
}


/*! @brief    parses program options.
 *
 *  opt_parse() parses program options. In typical cases, the caller of opt_parse() has to behave
 *  based on the result of opt_parse() that is one of:
 *  - '?': unrecognized option; the pointer given to opt_init() through @c pa points to a string
 *         that represents the option
 *  - '-': valid option, but no option-argument given even if the option requires it, or invalid
 *         option-argument given; the pointer given to opt_init() through @c pa points to a string
 *         that represents the option
 *  - '+': valid option, but an option-argument given even if the option takes none; the pointer
 *         given to opt_init() through @c pa points to a string that represents the option
 *  - '*': ambiguious option; it is impossible to identify a unique option with the given prefix of
 *         a long-named option
 *  - 0: valid option; a given flag variable is set properly, thus nothing for a user code to do
 *  - -1: all options have been processed
 *  - 1: (only when the first long-named option is "-") an operand is given; the pointer given to
 *       opt_init() through @c pa points to the operand
 *
 *  This means that a valid short-named option cannot have the value of '?', '-', '+', '*', -1 or
 *  1; 0 is allowed to say no short-named option given when a flag variable is provided; see
 *  @c opt_t for details. In addition, '=' cannot also be used.
 *
 *  If an option takes an option-argument, the pointer whose address passed to opt_init() through
 *  @c pa is set to point to the argument. A subsequent call to opt_parse() may overwrite it unless
 *  the type is @c OPT_TYPE_STR.
 *
 *  After opt_parse() returns -1, @c argc and @c argv (precisely, objects whose addresses are
 *  passed to opt_init() through @c pc and @c pv) are adjusted for a user code to process remaining
 *  operands as if there were no options or option-arguments in program arguments; see the
 *  commented-out example code given in the source file. Once opt_parse() starts the parsing,
 *  @c argc and the elements of @c argv are indeterminate, thus an access to them is not allowed.
 *
 *  opt_parse() changes neither the original contents of @c argv nor strings pointed to by the
 *  elements of @c argv, thus by granting copies of @c argc and @c argv to opt_init() as in the
 *  following example, a user code can access to program arguments unchanged if necessary even
 *  after options have been parsed by opt_parse().
 *
 *  @code
 *      int main(int argc, char *argv[])
 *      {
 *          const void *arg;
 *          int argc2 = argc;
 *          char **argv2 = argv;
 *          ...
 *          pname = opt_init(options, &argc2, &argv2, &arg, "program", '/');
 *          while (opt_parse() != -1) {
 *              ...
 *          }
 *
 *          for (i = 1; i < argc2; i++)
 *              printf("operands: %s\n", argv2[i]);
 *
 *          opt_free();
 *
 *          for (i = 1; i < argc; i++)
 *              printf("untouched program arguments: %s\n", argv[i]);
 *          ...
 *      }
 *  @endcode
 *
 *  @warning    opt_init() has to be invoked successfully before calling opt_parse().
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: @c argc or @c argv modified by a user between calls to opt_parse(), modifying
 *                    an object containing an option-argument or a problematic option
 *
 *  @internal
 *
 *  At its early development stage, opt_parse() has separate case labels for @c SHORTOPT and
 *  @c LONGOPT without "fall-through." Found that two fragments of code belonging to those separate
 *  labels resemble each other very much, those two branches are merged, which is what it is now.
 *  This is a little tricky (e.g., when @c kind is @c LONGOPT, jumping into the middle of a loop is
 *  made) and brings some otherwise unnecessary checks for @c kind.
 */
int (opt_parse)(void)
{
    int i;
    int kind;
    int retval;
    const opt_t *p;
    int argc;
    char **argv;
    const void *arg;

    assert(opt);    /* opt_init() must be called before opt_parse() */
    assert(pargc);
    assert(pargv);
    assert(parg);

    argc = *pargc;
    argv = *pargv;
    retval = 0;
    arg = NULL;

    if (argc < 0 || (nopt == (void *)&oargc) || argv[argc+1] == NULL) {    /* done */
        argv[oargc] = NULL;
        retval = -1;
        argc = oargc;
        nopt = (void *)&oargc;    /* prevents opt_parse() from running further */
        goto retcode;
    }

    do {
        switch(kind = argcheck(argv[++argc])) {
            case SHORTOPT:    /* -f... */
                if (nopt) {    /* starts at next in grouped options */
                    assert(*UC(nopt) != '\0');
                    i = nopt - argv[argc];
                    nopt = NULL;
                } else
                    i = 1;
                for ( ; kind != LONGOPT && UC(argv[argc])[i] != '\0'; i++) {
                    int match;
            case LONGOPT:     /* --f... */
                    match = 0;    /* cannot be merged into initialization above */
                    for (p = opt; p->lopt; p++) {
                        if (kind == SHORTOPT && UC(argv[argc])[i] == (unsigned char)p->sopt)
                            match = 1;
                        else if (kind == LONGOPT && UC(p->lopt)[0] == UC(argv[argc])[2]) {
                            char *s;
                            const opt_t *q;
                            if ((s = strchr(argv[argc]+3, '=')) != NULL)    /* --f...=... */
                                 i = s - (argv[argc]+3);
                            else
                                 i = strlen(argv[argc]+3);
                            q = p;
                            do {    /* checks ambiguity */
                                if (UC(q->lopt)[0] == UC(argv[argc])[2] &&
                                    strncmp(q->lopt+1, argv[argc]+3, i) == 0) {
                                    /* prefix matched */
                                    if (UC(q->lopt)[1+i] == '\0') {    /* exact match */
                                        p = q;
                                        match = 1;
                                        break;
                                    }
                                    p = q;
                                    match++;
                                }
                            } while((++q)->lopt);
                            if (!match)
                                break;    /* assigning q to p is equivalent */
                            else if (match > 1) {    /* ambiguous prefix */
                                retval = '*';
                                arg = errlopt(argv[argc]+2);
                                goto retcode;
                            }
                            i += 2;    /* adjusts i for use below */
                            assert(UC(argv[argc])[i+1] == '\0' || argv[argc][i+1] == '=');
                        }
                        if (match) {
                            retval = p->sopt;
                            if (p->flag == OPT_ARG_REQ || p->flag == OPT_ARG_OPT) {
                                if (UC(argv[argc])[i+1] != '\0')    /* -f..., -f=..., --f...=... */
                                    arg = argconv(&argv[argc][i+1+(argv[argc][i+1]=='=')], p->arg);
                                else if (argcheck(argv[argc+1]) == OPERAND) {
                                    arg = argconv(argv[argc+1], p->arg);
                                    if (arg || p->flag == OPT_ARG_REQ)    /* extra arg consumed */
                                        argc++;
                                } else if (p->flag == OPT_ARG_OPT)
                                    arg = p->lopt;    /* arg should not be null in this case */
                                if (!arg) {
                                    if (p->flag == OPT_ARG_OPT && argv[argc][i+1] != '=') {
                                        /* argv[argc][i+1] != '\0' implies kind == SHORTOPT */
                                        if (UC(argv[argc])[i+1] != '\0') {
                                            nopt = &argv[argc][i+1];
                                            argc--;
                                        }
                                    } else {    /* no or invalid arg for OPT_ARG_REQ */
                                        retval = '-';
                                        arg = (kind == LONGOPT)? errlopt(p->lopt): errsopt(p->sopt);
                                    }
                                } else if (arg == p->lopt)
                                    arg = NULL;    /* set back it to null */
                            } else if (argv[argc][i+1] == '=') {
                                retval = '+';
                                arg = (kind == LONGOPT)? errlopt(p->lopt): errsopt(p->sopt);
                            } else if (p->flag != OPT_ARG_NO && p->flag) {    /* flag variable */
                                *(p->flag) = p->arg;
                                retval = 0;
                                break;
                            } else {    /* no flag variable, so returns option */
                                assert(kind == SHORTOPT || retval != 0);
                                if (kind == SHORTOPT && UC(argv[argc])[i+1] != '\0') {
                                    nopt = &argv[argc][i+1];
                                    argc--;
                                }
                            }
                            goto retcode;
                        }
                    }
                    if (!match) {
                        retval = '?';
                        arg = (kind == LONGOPT)? errlopt(argv[argc]+2): errsopt(argv[argc][i]);
                        goto retcode;
                    }
                }
                break;
            case OPERAND:    /* arranges ordinary arguments for later use */
                if (order == RETURN_IN_ORDER) {
                    retval = 1;
                    arg = argv[argc];
                    goto retcode;
                } else {    /* order == PERMUTE || order == REQUIRE_ORDER */
                    if (!oprdflag && order == REQUIRE_ORDER)
                        oprdflag = 1;
                    argv[oargc++] = argv[argc];
                }
                break;
            case DMINUS:    /* -- */
                /* nothing to do */
                break;
            default:
                assert(!"invalid type of arguments -- should never reach here");
                break;
        }
    } while(argv[argc+1]);

    retcode:
        *parg = arg;
        *pargc = argc;
        return retval;
}


/* @brief    compares strings for argument comparison in opt_val().
 *
 * striscmp() compares two strings as strcmp() does except that:
 * - it returns a non-zero if two strigns compare unequal (that is, its sign is meaningless);
 * - the case will be ignored if @c OPT_CMP_CASEIN is set in @p flag; and
 * - hyphens(-) and underscores(_) will be considered equivalent to spaces if @c OPT_CMP_NORMSPC is
 *   set in @p flag.
 *
 * @param[in]    s       string to compare
 * @param[in]    t       string to compare
 * @param[in]    flag    flag to control behavior
 *
 * @return    integer to indicate whether or not two strings compare equal
 * @retval    0           two strings compare equal
 * @retval    non-zero    two strings compare unequal
 */
static int striscmp(const char *s, const char *t, int flag)
{
    unsigned char c, d;

    assert(s);
    assert(t);

    do {
        c = *(unsigned char *)s++;
        d = *(unsigned char *)t++;
        if (flag & OPT_CMP_NORMSPC)
            c = normal(c), d = normal(d);
        if (flag & OPT_CMP_CASEIN)
            c = tolower(c), d = tolower(d);
    } while(c == d && c != '\0');

    return (c != '\0' || d != '\0');
}


/*! @brief    compares a string argument to a set of strings.
 *
 *  opt_val() helps comparison of a string argument (referred to as @c argptr of the
 *  @c OPT_TYPE_STR type in examples here) with a set of strings. opt_val() compares a string @p s
 *  to strings from an @c opt_val_t array as if done with strcmp() except that the case will be
 *  ignored if @c OPT_CMP_CASEIN is set in @p flag and that hyphens(-) and underscores(_) will be
 *  regarded as equivalent to spaces if @c OPT_CMP_NORMSPC set. The later flag is useful when
 *  letting users readily pass to a program an argument with spaces. For example, when a program
 *  accepts @c "unsigned int" as a string option-argument, specifying @c unsigned-int or
 *  @c unsigned_int should be easier than quoting @c "unsigned int" to preserve the intervening
 *  space. That means setting @c OPT_CMP_NORMSPC effectively changes
 *
 *  @code
 *      opt_val_t t[] = {
 *          "unsigned int", 0,
 *          NULL, -1
 *      };
 *  @endcode
 *
 *  to
 *
 *  @code
 *      opt_val_t t[] = {
 *          "unsigned int", 0, "unsigned-int", 0, "unsigned_int", 0,
 *          NULL, -1
 *      };
 *  @endcode
 *
 *  If there is a matched string in @c tab, opt_val() returns an integer that is paired with the
 *  string, or an integer that is paired with a null pointer otherwise.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: invalid @p tab given, an option-argument that is not of the @c OPT_TYPE_STR
 *                    type given
 *
 *  @param[in]    tab     array containing string-integer pairs
 *  @param[in]    s       string argument to compare with @p tab
 *  @param[in]    flag    flag to control behavior
 *
 *  @return    integer corresponding to a matched string
 */
int (opt_val)(opt_val_t *tab, const char *s, int flag)
{
    assert(tab);
    assert(s);

    for (; tab->str; tab++)
        if (striscmp(tab->str, s, flag) == 0)
            break;

    return tab->val;
}


/*! @brief    aborts parsing options.
 *
 *  opt_abort() aborts parsing options immediately handling the remaining arguments as operands.
 *  Having invoked opt_abort(), opt_parse() need not be called to access to operands; @c argc and
 *  @ argv are properly adjusted as if opt_parse() has returned -1 except that the remaining
 *  options (if any) are treated as operands. If opt_parse() invoked after aborting the parsing,
 *  opt_parse() does nothing and returns -1.
 *
 *  @return    nothing
 */
void (opt_abort)(void)
{
    int argc;
    char **argv;

    assert(opt);    /* opt_init() must be called before opt_abort() */
    assert(pargc);
    assert(pargv);
    assert(parg);

    argc = *pargc;
    argv = *pargv;

    while(argv[argc+1]) {
        argv[oargc++] = argv[++argc];
    }
    argv[oargc] = NULL;

    *pargc = oargc;
    nopt = (void *)&oargc;    /* prevents opt_parse() from running further */
}


/*! @brief    returns a diagnostic format string for an error code.
 *
 *  Given an error code that is one of '?', '-', '+' and '*', opt_errmsg() returns a string that
 *  can be used as a format string for the printf() family. A typical way to handle exceptional
 *  cases opt_parse() may return is as follows:
 *
 *  @code
 *      switch(c) {
 *          ... cases for valid options ...
 *          case 0:
 *              break;
 *          case '?':
 *              fprintf(stderr, "%s: unknown option '%s'\n", option.prgname, (const char *)argptr);
 *              opt_free();
 *              return EXIT_FAILURE;
 *          case '-':
 *              fprintf(stderr, "%s: no or invalid argument given for '%s'\n", option.prgname,
 *                      (const char *)argptr);
 *              opt_free();
 *              return EXIT_FAILURE;
 *          case '+':
 *              fprintf(stderr, "%s: option '%s' takes no argument\n", option.prgname,
 *                      (const char *)argptr);
 *              opt_free();
 *              return EXIT_FAILURE;
 *          case '*':
 *              fprintf(stderr, "%s: ambiguous option '%s'\n", option.prgname,
 *                      (const char *)argptr);
 *              opt_free();
 *              return EXIT_FAILURE;
 *          default:
 *              assert(!"not all options covered -- should never reach here");
 *              break;
 *      }
 *  @endcode
 *
 *  where "case 0" is for options that sets a flag variable so in most cases leaves nothing for a
 *  user code to do. The following four case labels handle erroneous cases and the default case is
 *  there to handle what is never supposed to happen.
 *
 *  As repeating this construct for every program using this library is cumbersome, for convenience
 *  opt_errmsg() is provided to handle those four erroneous cases as follows:
 *
 *  @code
 *      switch(c) {
 *          ... cases for valid options ...
 *          case 0:
 *              break;
 *          case '?':
 *          case '-':
 *          case '+':
 *          case '*':
 *              fprintf(stderr, "%s: ", option.prgname);
 *              fprintf(stderr, opt_errmsg(c), (const char *)argptr);
 *              opt_free();
 *              return EXIT_FAILURE;
 *          default:
 *              assert(!"not all options covered -- should never reach here");
 *              break;
 *      }
 *  @endcode
 *
 *  or more compatly:
 *
 *  @code
 *      switch(c) {
 *          ... cases for valid options ...
 *          case 0:
 *              break;
 *          default:
 *              fprintf(stderr, "%s: ", option.prgname);
 *              fprintf(stderr, opt_errmsg(c), (const char *)argptr);
 *              opt_free();
 *              return EXIT_FAILURE;
 *      }
 *  @endcode
 *
 *  The difference of the last two is that the latter turns the assertion in the former (that
 *  possibly gets dropped from the delivery code) into a defensive check (that does not). Note that
 *  the returned format string contains a newline.
 *
 *  If a user needs flexibility on the format of diagnostics or actions done in those cases, resort
 *  to the cumbersome method shown first.
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: none
 *
 *  @param[in]    c    error code opt_parse() returned
 *
 *  @return    format string for diagnostic message
 */
const char *(opt_errmsg)(int c)
{
    const char *p, *tab = "?-+*";
    const char *msg[] = {
        "unknown option '%s'\n",
        "no or invalid argument given for '%s'\n",
        "option '%s' takes no argument\n",
        "ambiguous option '%s'\n",
        "not all options covered\n"
    };

    if ((p = strchr(tab, c)) == NULL) {
        assert(!"not all options covered -- should never reach here");
        p = tab + sizeof(msg)/sizeof(*msg)-1;
    }

    assert((p-tab) >= 0 && (p-tab) < sizeof(msg)/sizeof(*msg));

    return msg[p - tab];
}


/*! @brief    cleans up any storage used and disables the internal state.
 *
 *  opt_free() cleans up any storage allocated by opt_init() and used by opt_parse(). It also
 *  initializes the internal state, which allows for multiple scans; see opt_init() for some caveat
 *  when scanning options multiple times.
 *
 *  @warning    opt_free(), if invoked, should be invoked after all arguments including operands
 *              have been processed. Since opt_init() makes copies of pointers in @c argv of
 *              main(), and opt_free() releases storages for them, any access to them gets
 *              invalidated by opt_free().
 *
 *  Possible exceptions: none
 *
 *  Unchecked errors: none
 *
 *  @return    nothing
 */
void (opt_free)(void)
{
    free(*pargv);

    opt = NULL;
    pargc = NULL;
    pargv = NULL;
    parg = NULL;
}


#if 0    /* example code */
#include <assert.h>    /* assert */
#include <stddef.h>    /* NULL */
#include <stdio.h>     /* puts, putchar, printf, fprintf, stderr */
#include <stdlib.h>    /* EXIT_FAILURE */
#include <limits.h>    /* UCHAR_MAX */

#include <cel\opt.h>    /* opt_t, opt_init, opt_parse, opt_errmsg, opt_free, OPT_ARG_*,
                           OPT_TYPE_* */

#define PRGNAME "opt-test"

struct option {
    const char *prgname;    /* program name */
    int verbose;            /* set by "--verbose" and unset by "--brief" */
} option;

int main(int argc, char *argv[])
{
    opt_t tab[] = {
        "verbose",  0,           &(option.verbose), 1,
        "brief",    0,           &(option.verbose), 0,
        "add",      'a',         OPT_ARG_NO,        OPT_TYPE_NO,
        "append",   'b',         OPT_ARG_NO,        OPT_TYPE_NO,
        "create",   'c',         OPT_ARG_REQ,       OPT_TYPE_STR,
        "delete",   'd',         OPT_ARG_REQ,       OPT_TYPE_STR,
        "file",     'f',         OPT_ARG_REQ,       OPT_TYPE_STR,
        "",         'h',         OPT_ARG_NO,        OPT_TYPE_NO,
        "integer",  'i',         OPT_ARG_OPT,       OPT_TYPE_INT,
        "number",   'n',         OPT_ARG_OPT,       OPT_TYPE_REAL,
        "",         'o',         OPT_ARG_NO,        OPT_TYPE_NO,
        "connect",  UCHAR_MAX+1, OPT_ARG_REQ,       OPT_TYPE_STR,
        "html",     UCHAR_MAX+2, OPT_ARG_NO,        OPT_TYPE_NO,
        "help",     UCHAR_MAX+3, OPT_ARG_NO,        OPT_TYPE_NO,
        "helpmore", UCHAR_MAX+4, OPT_ARG_NO,        OPT_TYPE_NO,
        "bool",     UCHAR_MAX+5, OPT_ARG_REQ,       OPT_TYPE_BOOL,
        NULL,    /* must end with NULL */
    };
    int i;
    int c;
    int connect = 0;
    const void *argptr;

    option.prgname = opt_init(tab, &argc, &argv, &argptr, PRGNAME, '/');
    if (!option.prgname) {
        fprintf(stderr, "%s: failed to parse options\n", PRGNAME);
        return EXIT_FAILURE;
    }
    printf("Program name: %s\n", option.prgname);
    while ((c = opt_parse()) != -1) {
        switch(c) {
            case 'a':
                printf("%s: option -a given\n", option.prgname);
                break;
            case 'b':
                printf("%s: option -b given\n", option.prgname);
                break;
            case 'c':
            case 'd':
            case 'f':
                printf("%s: option -%c given with value '%s'\n", option.prgname, c,
                       (const char *)argptr);
                break;
            case 'h':
                printf("%s: option -h given\n", option.prgname);
                break;
            case 'i':
                printf("%s: option -i given", option.prgname);
                if (argptr)
                    printf(" with value '%ld'\n", *(const long *)argptr);
                else    /* optional */
                    putchar('\n');
                break;
            case 'n':
                printf("%s: option -n given", option.prgname);
                if (argptr)
                    printf(" with value '%f'\n", *(const double *)argptr);
                else    /* optional */
                    putchar('\n');
                break;
            case 'o':
                printf("%s: option -o given\n", option.prgname);
                break;
            case UCHAR_MAX+1:
                {
                    opt_val_t t[] = {
                        "stdin",   0, "standard input",  0,
                        "stdout",  1, "standard output", 1,
                        "stderr",  2, "standard error",  2,
                        NULL,     -1
                    };
                    connect = opt_val(t, argptr, OPT_CMP_CASEIN | OPT_CMP_NORMSPC);
                    if (connect == -1) {
                        printf("%s: `stdin', `stdout' or `stderr' must be given for --connect\n",
                               option.prgname);
                        opt_free();
                        return EXIT_FAILURE;
                    }
                }
                break;
            case UCHAR_MAX+2:
                printf("%s: option --html given\n", option.prgname);
                break;
            case UCHAR_MAX+3:
                printf("%s: help requested\n", option.prgname);
                opt_free();
                return 0;
            case UCHAR_MAX+4:
                printf("%s: more help requested\n", option.prgname);
                opt_free();
                return 0;
            case UCHAR_MAX+5:
                printf("%s: option --boolean given with %s\n", option.prgname,
                       (*(const int *)argptr)? "true": "false");
                break;

            /* common case labels follow */
            case 0:    /* flag variable set; do nothing else now */
                break;
            case '?':    /* unrecognized option */
            case '-':    /* no or invalid argument given for option */
            case '+':    /* argument given to option that takes none */
            case '*':    /* ambiguous option */
                fprintf(stderr, "%s: ", option.prgname);
                fprintf(stderr, opt_errmsg(c), (const char *)argptr);
                opt_free();
                return EXIT_FAILURE;
            default:
                assert(!"not all options covered -- should never reach here");
                break;
        }
    }

    if (option.verbose)
        puts("verbose flag is set");
    printf("connect option is set to %d\n", connect);

    if (argc > 1) {
        printf("non-option ARGV-arguments:");
        for (i = 1; i < argc; i++)
            printf(" %s", argv[i]);
        putchar('\n');
    }

    opt_free();

    return 0;
}
#endif    /* disabled */

/* end of opt.c */
