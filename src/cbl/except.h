/*
 *  exception (cbl)
 */

#ifndef EXCEPT_H
#define EXCEPT_H

#include <setjmp.h>    /* setjmp, jmp_buf */


/* represents an exception */
typedef struct except_t {
    const char *exception;    /* name of exception */
} except_t;

/* exception frame to handle nested exceptions */
typedef struct except_frame_t {
    struct except_frame_t *prev;    /* previous exception frame */
    jmp_buf env;                    /* jmp_buf for current exception */
    const char *file;               /* file name in which exception raised */
#if __STDC_VERSION__ >= 199901L    /* C99 supported */
    const char *func;               /* function name in which exception raised */
#endif    /* __STDC_VERSION__ */
    int line;                       /* line number on which exception raised */
    const except_t *exception;      /* name of exception */
} except_frame_t;

/* state of exception handling;
   EXCEPT_ENTERED is set to zero, which is value returned from initial call to setjmp() */
enum {
    EXCEPT_ENTERED = 0,    /* exception handling started and no exception raised */
    EXCEPT_RAISED,         /* exception raised and not handled yet */
    EXCEPT_HANDLED,        /* exception handled */
    EXCEPT_FINALIZED       /* exception finalized */
};


/* stack for handling nested exceptions */
extern except_frame_t *except_stack;


#if __STDC_VERSION__ >= 199901L    /* C99 version */
void except_raise(const except_t *, const char *, const char *, int);
#else    /* C90 version */
void except_raise(const except_t *, const char *, int);
#endif    /* __STDC_VERSION__ */


/* raises exception */
#if __STDC_VERSION__ >= 199901L    /* C99 version */
#define EXCEPT_RAISE(e) except_raise(&(e), __FILE__, __func__, __LINE__)
#define EXCEPT_RERAISE except_raise(except_frame.exception, except_frame.file,    \
                                    except_frame.func, except_frame.line)
#else    /* C90 version */
#define EXCEPT_RAISE(e) except_raise(&(e), __FILE__, __LINE__)
#define EXCEPT_RERAISE except_raise(except_frame.exception, except_frame.file, except_frame.line)
#endif    /* __STDC_VERSION__ */

/* returns to the caller within TRY-EXCEPT statement */
#define EXCEPT_RETURN switch(except_stack=except_stack->prev, 0) default: return

/* starts a TRY statement */
#define EXCEPT_TRY                                             \
            {                                                  \
                volatile int except_flag;                      \
                /* volatile */ except_frame_t except_frame;    \
                except_frame.prev = except_stack;              \
                except_stack = &except_frame;                  \
                except_flag = setjmp(except_frame.env);        \
                if (except_flag == EXCEPT_ENTERED) {

/*
 *  starts an EXCEPT(e) clause
 *
 *  The indented if plays its role only when an EXCEPT clause follows statements S; it handles the
 *  case where no exception raised during execution of S.
 */
#define EXCEPT_EXCEPT(e)                                        \
                    if (except_flag == EXCEPT_ENTERED)          \
                        except_stack = except_stack->prev;      \
                } else if (except_frame.exception == &(e)) {    \
                    except_flag = EXCEPT_HANDLED;

/* starts an ELSE clause */
#define EXCEPT_ELSE                                           \
                    if (except_flag == EXCEPT_ENTERED)        \
                        except_stack = except_stack->prev;    \
                } else {                                      \
                    except_flag = EXCEPT_HANDLED;

/* starts a FINALLY clause */
#define EXCEPT_FINALLY                                        \
                    if (except_flag == EXCEPT_ENTERED)        \
                        except_stack = except_stack->prev;    \
                }                                             \
                {                                             \
                    if (except_flag == EXCEPT_ENTERED)        \
                        except_flag = EXCEPT_FINALIZED;

/* ends a TRY-EXCEPT or TRY-FINALLY statement */
#define EXCEPT_END                                            \
                    if (except_flag == EXCEPT_ENTERED)        \
                        except_stack = except_stack->prev;    \
                }                                             \
                if (except_flag == EXCEPT_RAISED)             \
                    EXCEPT_RERAISE;                           \
            }


#endif    /* EXCEPT_H */

/* end of except.h */
