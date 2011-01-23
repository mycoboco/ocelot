/*!
 *  @file    assert.h
 *  @brief    Documentation for Assertion Library (CBL)
 */

/*!
 *  @mainpage    C Basic Library: Assertion Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2011-01-24
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Assertion Library which belings to the C Basic Library. The basic
 *  structure is from David Hanson's book, "C Interfaces and Implementations." I modified the
 *  original implementation to make it more appropriate for my other projects as well as to increase
 *  its readability as done for the Exception Handling Library; conformance is not an issue here
 *  because trying to replace an exsisting standard library by "knocking it out" is already
 *  cosidered non-conforming.
 *
 *  Replacing <assert.h> with a version to support exceptions is useful based on some assumptions:
 *  - Deactiaving assertions in product code does not bring much benefit to the program's
 *    performance (only profiling would be able to say how much assertions degrades its
 *    performance);
 *  - Avoiding assertions showing cryptic dianostic messages leads users into a more catastrophic
 *    situation like dereferencing a null pointer;
 *  - It is not that a diagnostic message like "Uncaught exception Assertion failed raised at
 *    file.c:12" without an expression that caused the assertion to fail conveys less information
 *    than the message from the standard version, if the programmer can access to the problematic
 *    file; and
 *  - Dealing with assertion failures by an exception has an advantage that every assertion can be
 *    handled in a uniform way (see an example below).
 *
 *  More detailed explanation on the motivation is given in Hanson's book, so I should stop here
 *  but I provides introduction to the library; how to use the facilities is deeply explained in
 *  files that define them.
 *
 *  The Assertion Library reserves identifiers @c assert and @c ASSERT, and imports the Exception
 *  Handling Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  Using the Assertion Library is not quite different from using the standard version. Differntly
 *  from Hanson's original implementation, this implementation includes the standard version
 *  <assert.h> if either @c NDEBUG or @c ASSERT_STDC_VER is defined.
 *
 *  The following example shows how to handle assertion failures in one place:
 *
 *  @code
 *      #include "assert.h"
 *      #include "except.h"
 *
 *      int main(void)
 *      {
 *          EXCEPT_TRY
 *              real_main();
 *          EXCEPT_EXCEPT(assert_exceptfail)
 *              fprintf(stderr,
 *                      "An internal error occurred with no way to recover.\n"
 *                      "Please report this error to somebody@somewhere.\n\n");
 *              RERAISE;
 *          EXCEPT_END;
 *
 *          return 0;
 *      }
 *  @endcode
 *
 *  If you use an ELSE clause instead of the EXCEPT clause given above, any uncaught exception lets
 *  the program prints the message before aborting. (See the documentation in the Exception Handling
 *  Library.)
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  No boilerplate code is provided for this library.
 *
 *
 *  @section sec_future Future Directions
 *
 *  No future changes on this library planned yet.
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
 *  This package is an assertion facility implementation by Jun Woong. The implementation was
 *  written so as to conform with the Standard C published by ISO 9899:1990 and ISO 9899:1999.
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
