/*!
 *  @file     except.c
 *  @brief    Source for Exception Handling Library (CBL)
 */

#include <stddef.h>    /* NULL */
#include <setjmp.h>    /* longjmp */
#include <stdio.h>     /* stderr, fprintf, fflush */
#include <stdlib.h>    /* abort */

#include "cbl/assert.h"    /* assert with exception support */
#include "except.h"


/*! @brief    stack for handling nested exceptions.
 */
except_frame_t *except_stack = NULL;


/*! @brief    raises an exception @e and set its information properly.
 *
 *  @c EXCEPT_RAISE and @c EXCEPT_RERAISE macros call except_raise() with @c __FILE__ and
 *  @c __LINE__ predefined macros (and @c __func__ if C99 supported) for the @p file and @p line
 *  parameters. In general there is little chance to call except_raise() directly from application
 *  code.
 *
 *  Possible exceptions: assert_exceptfail
 *
 *  Unchecked errors: foreign data structure given for @p e
 *
 *  @param[in]    e       exception to raise
 *  @param[in]    file    file name where exception occurred
 *  @param[in]    func    function name where exception occurred (if C99 supported)
 *  @param[in]    line    line number where exception occurred
 *
 *  @return    except_raise() cannot return anything
 *
 *  @todo    Improvements are possible and planned:
 *           - it would be useful to show stack traces when an uncaught exception leads to abortion
 *             of a program. The stack traces should include as much information as possible, for
 *             example, names of caller functions, calling sites (file name, function name and line
 *             number) and arguments.
 *
 *  @internal
 *
 *  Note that the current exception frame is popped as soon as possible, which enables another
 *  exception occurred in handling the current exception to be handled by the previous hander (if
 *  any).
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void (except_raise)(const except_t *e, const char *file, const char *func, int line)
#else    /* C90 version */
void (except_raise)(const except_t *e, const char *file, int line)
#endif    /* __STDC_VERSION__ */
{
    except_frame_t *p = except_stack;    /* current exception frame */

    assert(e);    /* e should not be null pointer */
    if (!p) {    /* no current exception frame */
        fprintf(stderr, "Uncaught exception");
        if (e->exception && e->exception[0] != '\0')
            fprintf(stderr, " %s", e->exception);
        else
            fprintf(stderr, " at 0x%p", (void *)e);
#if __STDC_VERSION__ >= 199901L    /* C99 version */
        if (file && func && line > 0)
            fprintf(stderr, " raised at %s() %s:%d\n", func, file, line);
#else    /* C90 version */
        if (file && line > 0)
            fprintf(stderr, " raised at %s:%d\n", file, line);
#endif    /* __STDC_VERSION__ */
        fprintf(stderr, "Aborting...\n");
        fflush(stderr);
        abort();
    } else {
        /* set exception frame properly */
        p->exception = e;
        p->file = file;
#if __STDC_VERSION__ >= 199901L    /* C99 supported */
        p->func = func;
#endif
        p->line = line;

        /* pop exception stack */
        except_stack = except_stack->prev;

        /* exception raised but not handled yet, so EXCEPT_RAISED */
        longjmp(p->env, EXCEPT_RAISED);
    }
}

/* end of except.c */
