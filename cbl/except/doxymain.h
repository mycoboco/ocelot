/*!
 *  @file    except.h
 *  @brief    Documentation for Exception Handling Library (CBL)
 */

/*!
 *  @mainpage    C Basic Library: Exception Handling Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2011-01-24
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Exception Handling Library which belongs to the C Basic Library. The
 *  basic structure is from David Hanson's book, "C Interfaces and Implementations." I modified the
 *  original implementation to make it more appropriate for my other projects, to conform to the C
 *  standard and to enhance its readibility; for example a prefix is used more strictly in order to
 *  avoid the user namespace pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving typical exception-handing constructs with short explanation and emphasis on
 *  crucial issues. Some improvements to support C99 is also explained. How to use the facilities is
 *  deeply explained in files that define them.
 *
 *  The Exception Handling Library reserves identifiers starting with @c except_ and @c EXCEPT_, and
 *  imports the Assertion Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The followin constrcut shows how a typical TRY-EXCEPT statement looks.
 *
 *  @code
 *      EXCEPT_TRY
 *          S;
 *      EXCEPT_EXCEPT(e1)
 *          S1;
 *      EXCEPT_EXCEPT(e2)
 *          S2;
 *      EXCEPT_ELSE
 *          S3;
 *      EXCEPT_END;
 *  @endcode
 *
 *  @c EXCEPT_TRY starts a TRY-EXCEPT or TRY-FINALLY statement. The statements following
 *  @c EXCEPT_TRY (referred to as @c S in this example) are executed, and if an exception is
 *  occurred during the execution the control moves to one of EXCEPT clauses with a matching
 *  exception or the ELSE clause. The statements @c Sn under the matched EXCEPT clause or ELSE
 *  clause are executed and the control moves to the next statement (if any) to @c EXCEPT_END.
 *
 *  @code
 *      EXCEPT_TRY
 *          S
 *      EXCEPT_ELSE
 *          S1
 *      EXCEPT_END;
 *  @endcode
 *
 *  A constrcut without any EXCEPT clause is useful when catching all exceptions raised during
 *  execution of @c S in a uniform way. If other exception handers are established during execution
 *  of @c S only uncaught exceptions there move the control to the ELSE clause above. For example,
 *  any uncaught exception with no recovery (e.g., assertion failures or memory allocation failures)
 *  can be handled as follows in the @c main function.
 *
 *  @code
 *      int main(void)
 *      {
 *          EXCEPT_TRY
 *              real_main();
 *          EXCEPT_ELSE
 *              fprintf(stderr,
 *                      "An internal error occurred with no way to recover.\n"
 *                      "Please report this error to somebody@somewhere.\n\n");
 *              EXCEPT_RERAISE;
 *          EXCEPT_END;
 *
 *          return 0;
 *      }
 *  @endcode
 *
 *  A TRY-FINALLY statement looks like:
 *
 *  @code
 *      EXCEPT_TRY
 *          S;
 *      EXCEPT_FINALLY
 *          S1;
 *      EXCEPT_END;
 *  @endcode
 *
 *  The statements following @c EXCEPT_FINALLY are executed regardless of occurrence of an
 *  exception, so if a kind of clean-up like closing open files or freeing allocated storages is
 *  necessary to be performed unconditionally, @c S1 is its right place. If the exception caught by
 *  a TRY-FINALLY statement needs to be also handled by a TRY-EXCEPT statement @c EXCEPT_RERAISE
 *  raises it again to give the previous handler (if any) a chance to handle it.
 *
 *  Note that each group of the statements, say, @c S, @c S1 and so on, constitutes an independent
 *  block; opening or closing braces are hidden in @c EXCEPT_TRY, @c EXCEPT_EXCEPT,
 *  @c EXCEPT_FINALLY and @c EXCEPT_END. Therefore variables declared in a block, say, @c S is not
 *  visible to another block, say, @c S1.
 *
 *  And even if not explicitly specified in Hanson's book, it is possible to construct an exception
 *  handling statement which has both EXCEPT and FINALLY clauses, which looks like:
 *
 *  @code
 *      EXCEPT_TRY
 *          S;
 *      EXCEPT_EXCEPT(e)
 *          Se;
 *      EXCEPT_FIANLLY
 *          Sf;
 *      EXCEPT_END;
 *  @endcode
 *
 *  Looking into the implementation by combining those macros explains how it works. Finding when
 *  it is useful is up to its users.
 *
 *  @subsection subsec_caveats Some Caveats
 *
 *  Exception handling mechanism given here is implemented using a non-local jump provided by
 *  <setjmp.h>. Thus every restriction applied to <setjmp.h> also applies to this library. For
 *  example, there is no guarantee that an updated auto variable preserves its last stored value if
 *  the update done between setjmp() and longjmp(). For example,
 *
 *  @code
 *      {
 *          int i;
 *
 *          EXCEPT_TRY
 *              i++;
 *              S;
 *          EXCEPT_EXCEPT(e1)
 *              S1;
 *          EXCEPT_TRY;
 *      }
 *  @endcode
 *
 *  If an exception @c e1 occurs, which moves the control to @c S1, it is unknown what the value of
 *  i is in the EXCEPT clause above. A way to walk around this problem is to declare @c i as
 *  volatile or static. (See the manpage for setjmp() and longjmp().)
 *
 *  At the first blush, this restriction seems too restrictive, but not quite. The restriction
 *  applies only to those non-volatile variables with automatic storage duration and belonged to the
 *  function containing @c EXCEPT_TRY (which has setjmp() in it), and only when they are modified
 *  between setjmp() (@c EXCEPT_TRY) and corresponding longjmp() (except_raise()).
 *
 *  One more thing to remember is that the ordinary @c return statement does not work in the
 *  statements @c S above because it does not know anything about maintaining the exception stack.
 *  Inside @c S, the exception frame has already been pushed to the exception stack. Returning from
 *  it without adjusting the stack by popping the current frame spoils the exception handling
 *  mechanism, which results in undefined behavior. @c EXCEPT_RETURN is provided for this purpose.
 *  It does the same thing as the ordinary @c return statement except that it adjusts the exception
 *  stack properly. Also note that @c EXCEPT_RETURN is not necessary in a EXCEPT, ELSE or FINALLY
 *  clause since entering those cluases entails popping the current frame from the stack.
 *
 *  In general, it is said that recovery from an erroneous situation gets easier if you have a way
 *  to handle it with an exception and its handler. In practice, with the implementation using a
 *  non-local jump facility like this library, however, that is not always true. If a program
 *  manages resources like allocated memory and open file pointers in a complicated fashion and an
 *  exception can be raised at a deeply nested level, it is very likely for you to lose control over
 *  them. In addition to it, keeping internal data structures consistent is also a problem. If an
 *  exception can be triggered during modifying fileds of an internal data structure, it is never a
 *  trivia to guarantee consistency of that. Therefore, an exception handling facility this library
 *  provides is in fact best suited for handling in one place various problematic circumstances and
 *  then terminating the program's execution almost immediately. If you would like your code to be
 *  tolerant to an exceptional case by, for example, making it revert to an inferior but reliable
 *  approach, you have to keep these facts in your mind.
 *
 *  @subsection subsec_improves Improvements
 *
 *  The diagnostic message printed when an assertion failed changed in C99 to include the name of
 *  a function in which it failed. This can be readily attained with a newly introduced predefined
 *  identifier @c __func__. To provide more useful information, if an implementation claims to
 *  support C99 by defining the macro @c __STDC_VERSION__ properly, the library also includes the
 *  function name when making up the message output when an uncaught exception detected. For the
 *  explanation on @c __func__ and @c __STDC_VERSION__, see ISO/IEC 9899:1999.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  To show a bolierplate code, suppose that a module named "mod" defines and may raise exceptions
 *  named @c mod_exceptfail and @c mod_exceptmem, and that code invoking the module is expected to
 *  install an exception handler for that exception. Now implementing the module "mod" looks in part
 *  like:
 *
 *  @code
 *      const except_t mod_exceptfail = { "Some operation failed" };
 *      const except_t mod_exceptmem = { "Memory operation failed" };
 *
 *      ...
 *
 *      int mod_oper(int arg)
 *      {
 *          ...
 *          if (!p)
 *              EXCEPT_RAISE(mod_exceptfail);
 *          else if (p != q)
 *              EXCEPT_RAISE(mod_exceptmem);
 *          ...
 *      }
 *  @endcode
 *
 *  where the names of exceptions and the contents of strings used as initializers are up to an
 *  user. The string is printed out when the corresponding exception is raised but not caught. By
 *  installing an exception handler with a TRY-EXCEPT construct, code that invokes mod_oper() can
 *  handle exceptions mod_oper() may raise:
 *
 *  @code
 *      EXCEPT_TRY
 *          result = mod_oper(value);
 *          ...
 *      EXCEPT_EXCEPT(mod_exceptfail)
 *          fprintf(stderr, "program: some operation failed; no way to recover\n");
 *          EXCEPT_RERAISE;
 *      EXCEPT_EXCEPT(mod_exceptmem)
 *          fprintf(stderr, "program: memory allocation failed; internal buffer used\n");
 *          ... prepare internal buffer and retry ...
 *      EXCEPT_END
 *  @endcode
 *
 *  Note that exceptions other than @c mod_exceptfail and @c mod_exceptmem are uncaught by this
 *  handler and handed to an outer handler if any.
 *
 *
 *  @section sec_future Future Directions
 *
 *  @subsection subsec_stack Stack Traces
 *
 *  The current implementation provides no information about the execution stack of a program when
 *  an exception occurred leads it to abnormal termination. This imposes a burden on programmers
 *  since they have to track function calls by themselves to pinpoint the problem. Thus, showing
 *  stack traces on an uncaught exception would be useful especially when they include enough
 *  information like callers' names, calling sites and arguments.
 *
 *
 *  @section sec_contact Contact Me
 *
 *  Visit http://project.woong.org to get the latest version of this library. Only a small portion
 *  of my homepage (http://www.woong.org) is maintained in English, thus one who is not good at
 *  Korean would have difficulty when navigating most of other pages served in Korean. If you think
 *  the information you are looking for is on pages written in Korean you cannot read, do not
 *  hesitate to send me an email asking for help.
 *
 *  Any comments about the library are welcomed. If you have a proposal or question on the library
 *  just email me, and then I will reply as soon as possible.
 *
 *
 *  @section sec_license Copyright
 *
 *  I do not wholly hold the copyright of this library; it is mostly held by David Hanson as stated
 *  in his book, "C: Interfaces and Implementations:"
 *
 *  Copyright (c) 1994,1995,1996,1997 by David R. Hanson.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 *  and associated documentation files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all copies
 *  or substantial portions of the Software.
 *
 *  For the parts I added or modified, the following applies:
 *
 *  Copyright (C) 2009-2011 by Jun Woong.
 *
 *  This package is an exception handling facility implementation by Jun Woong. The implementation
 *  was written so as to conform with the Standard C published by ISO 9899:1990 and ISO 9899:1999.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 *  and associated documentation files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all copies
 *  or substantial portions of the Software.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
