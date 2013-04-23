/*!
 *  @file    except.h
 *  @brief    Header for Exception Handling Library (CBL)
 */

#ifndef EXCEPT_H
#define EXCEPT_H

#include <setjmp.h>    /* setjmp, jmp_buf */


/* @struct    except_t    except.h
 * @brief    represents an exception.
 *
 * @warning    The uniqueness of an exception is determined by the address of @c exception member
 *             in @c except_t; so make an @c except_t object have static storage duration when
 *             declaring it. It is undefined what happens when using an exception object with
 *             automatic storage duration.
 */
typedef struct except_t {
    const char *exception;    /* name of exception */
} except_t;

/* @struct    except_frame_t    except.h
 * @brief    provides the exception frame to handle nested exceptions.
 */
typedef struct except_frame_t {
    struct except_frame_t *prev;    /* previous exception frame */
    jmp_buf env;                    /* @c jmp_buf for current exception */
    const char *file;               /* file name in which exception raised */
#if __STDC_VERSION__ >= 199901L    /* C99 supported */
    const char *func;               /* function name in which exception raised */
#endif    /* __STDC_VERSION__ */
    int line;                       /* line number on which exception raised */
    const except_t *exception;      /* name of exception */
} except_frame_t;

/* @brief    indicates the state of exception handling.
 *
 * Note that @c EXCEPT_ENTERED is set to zero, which is the value returned from the initial call
 * to setjmp(). See the explanation for @c EXCEPT_TRY for more details.
 */
enum {
    EXCEPT_ENTERED = 0,    /*!< exception handling started and no exception raised yet */
    EXCEPT_RAISED,         /*!< exception raised and not handled yet */
    EXCEPT_HANDLED,        /*!< exception handled */
    EXCEPT_FINALIZED       /*!< exception finalized */
};


/* stack for handling nested exceptions */
extern except_frame_t *except_stack;


/*! @name    exception raising functions:
 */
/*@{*/
#if __STDC_VERSION__ >= 199901L    /* C99 version */
void except_raise(const except_t *, const char *, const char *, int);
#else    /* C90 version */
void except_raise(const except_t *, const char *, int);
#endif    /* __STDC_VERSION__ */
/*@}*/


/*! @brief    raises exception @c e.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define EXCEPT_RAISE(e) except_raise(&(e), __FILE__, __func__, __LINE__)
#else    /* C90 version */
#define EXCEPT_RAISE(e) except_raise(&(e), __FILE__, __LINE__)
#endif    /* __STDC_VERSION__ */

/*! @brief    raises the exception again that has been raised most recently.
 */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define EXCEPT_RERAISE except_raise(except_frame.exception, except_frame.file, except_frame.func, \
                                    except_frame.line)
#else    /* C90 version */
#define EXCEPT_RERAISE except_raise(except_frame.exception, except_frame.file, except_frame.line)
#endif    /* __STDC_VERSION__ */

/*! @brief    returns to the caller function within a TRY-EXCEPT statement.
 *
 *  In order to maintain the stack handling nested exceptions, the ordinary return statement should
 *  be avoided in statements following @c EXCEPT_TRY (referred to as @c S below; see the
 *  explanation for @c EXCEPT_TRY). Because @c return has no idea about the exception frame,
 *  retruning without using @c EXCEPT_RETURN from @c S spoils the exception stack. @c EXCEPT_RETURN
 *  adjusts the stack properly by popping the current exception frame before returning to the
 *  caller.
 *
 *  @warning    Note that the current exception frame is popped when an exception occurs during
 *              execution of @c S and before the control moves to one of EXCEPT, ELSE and FINALLY
 *              clauses, which means using @c EXCEPT_RETURN there is not allowed since it affects
 *              the previous, not the current, exception frame.
 */
#define EXCEPT_RETURN switch(except_stack=except_stack->prev, 0) default: return

/*! @brief    starts a TRY statement.
 *
 *  Statements (referred to as @c S hereafter) whose exception is to be handled in EXCEPT, ELSE or
 *  FINALLY clause follow.
 *
 *  @warning    Do not forget using @c EXCEPT_RETURN when returning from @c S. See
 *              @c EXCEPT_RETURN for more details. Besides, the TRY-EXCEPT/FINALLY statement uses
 *              the non-local jump mechanism provided by <setjmp.h>, which means any restriction
 *              applied to <setjmp.h> also applies to the TRY-EXCEPT/FINALLY statement. For
 *              example, the standard does not guarantee that an automatic non-volatile variable
 *              belonging to the function which contains setjmp() preserves its last stored value
 *              when it is updated between a call to setjmp() and a call to longjmp() with the same
 *              @c jmp_buf.
 *
 *  @internal
 *
 *  @c EXCEPT_TRY pushes the current exception frame into the exception stack and calls setjmp().
 *  Note that @c EXCEPT_ENTERED is 0, which is defined as the return value of the initial call to
 *  setjmp().
 *
 *  Differntly from the original implementation, the do-while(0) trick to make a macro invocation
 *  look like a real statement or function call has been eliminated. This is to allow @c continue
 *  or @c break to appear in code within a TRY-EXCEPT statement when the TRY-EXCEPT statement is
 *  in a loop. This change has a side effect to make a TRY-EXCEPT statement without a trailing
 *  semicolon valid or, in order words, to make a trailing semicolon of a TRY-EXCEPT statement
 *  redundant, but considering normal use of the statement it should not be a big deal.
 *
 *  Due to the above restrictions imposed on setjmp() and longjmp(), @c except_flag is declared as
 *  volatile. In theory, @c except_frame also should be declared as volatile, but not because it is
 *  a structure containing various members and modified through pointers; it will rarely cause a
 *  problem in practice.
 */
#define EXCEPT_TRY                                             \
            {                                                  \
                volatile int except_flag;                      \
                /* volatile */ except_frame_t except_frame;    \
                except_frame.prev = except_stack;              \
                except_stack = &except_frame;                  \
                except_flag = setjmp(except_frame.env);        \
                if (except_flag == EXCEPT_ENTERED) {

/*! @brief    starts an EXCEPT(e) clause.
 *
 *  When an exception @c e is raised, its following statements are executed. Finishing them moves
 *  the control to the end of the TRY statement.
 *
 *  @internal
 *
 *  The indented @c if plays its role only when an EXCEPT clause follows statements @c S; it
 *  handles the case where no exception raised during execution of @c S.
 */
#define EXCEPT_EXCEPT(e)                                        \
                    if (except_flag == EXCEPT_ENTERED)          \
                        except_stack = except_stack->prev;      \
                } else if (except_frame.exception == &(e)) {    \
                    except_flag = EXCEPT_HANDLED;

/*! @brief    starts an ELSE clause.
 *
 *  If there is no matched EXCEPT clause for a raised exception the control moves to the statements
 *  following the ELSE clause.
 *
 *  @internal
 *
 *  The same explanation given above goes for the indented @c if below.
 */
#define EXCEPT_ELSE                                           \
                    if (except_flag == EXCEPT_ENTERED)        \
                        except_stack = except_stack->prev;    \
                } else {                                      \
                    except_flag = EXCEPT_HANDLED;

/*! @brief    starts a FINALLY clause.
 *
 *  It is used to construct a TRY-FINALLY statement, which is useful when some clean-up is necessary
 *  before exiting the TRY-FINALLY statement; the statements under the FINALLY clause are executed
 *  whether or not an exception occurs. @c EXCEPT_RERAISE macro can be used to hand over the
 *  not-yet-handled exception to the previous handler.
 *
 *  @warning    Remember that, since raising an exception pops up the execption stack, re-raising
 *              an exception in a FINALLY clause has the effect to move the control to the outer
 *              (previous) handler. Also note that, even if not explicitly specified, a
 *              TRY-EXCEPT-FINALLY statement (there are both EXCEPT and FINALLY clauses) is
 *              possible and works as expected.
 *
 *  @internal
 *
 *  The same explanation given above goes for the indented @c if with @c except_flag @c ==
 *  @c EXCEPT_ENTERED below.
 */
#define EXCEPT_FINALLY                                        \
                    if (except_flag == EXCEPT_ENTERED)        \
                        except_stack = except_stack->prev;    \
                }                                             \
                {                                             \
                    if (except_flag == EXCEPT_ENTERED)        \
                        except_flag = EXCEPT_FINALIZED;

/*! @brief    ends a TRY-EXCEPT or TRY-FINALLY statement.
 *
 *  If a raised exception is not handled by the current handler, it will be handled by the previous
 *  handler if any.
 */
#define EXCEPT_END                                            \
                    if (except_flag == EXCEPT_ENTERED)        \
                        except_stack = except_stack->prev;    \
                }                                             \
                if (except_flag == EXCEPT_RAISED)             \
                    EXCEPT_RERAISE;                           \
            }


#endif    /* EXCEPT_H */

/* end of except.h */
