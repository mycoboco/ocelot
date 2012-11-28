/*!
 *  @file    stack.h
 *  @brief    Documentation for Stack Library (CDSL)
 */

/*!
 *  @mainpage    C Data Structure Library: Stack Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2011-01-24
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Stack Library which belongs to the C Data Structure Library. The
 *  basic structure is from David Hanson's book, "C Interfaces and Implementations." I modified the
 *  original implementation to enhance its readibility, for example a prefix is used more strictly
 *  in order to avoid the user namespace pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving introduction to the library; how to use the facilities is deeply explained in
 *  files that define them.
 *
 *  The Stack Library reserves identifiers starting with @c stack_ and @c STACK_, and imports the
 *  Assertion Library (which requires the Exception Handling Library) and the Memory Management
 *  Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The Stack Library is a typical implementation of a stack based on a liked list. Even if its
 *  implementation is very similar to the List Library, the implementation details are hidden behind
 *  an abstract type called @c stack_t because, unlike lists, revealing the implementation of a
 *  stack hardly brings benefit. The storage used to maintain a stack itself is managed by the
 *  library, but any storage allocated for data stored in a stack should be managed by a user
 *  program.
 *
 *  Similarly for other data structure libraries, use of the Stack Library follows this sequence:
 *  create, use and destroy.
 *
 *  If functions that allocate storage fail memory allocation, an exception @c mem_exceptfail is
 *  raised; therefore functions never return a null pointer.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  Using a list starts with creating it. There is only one function provided to create a new stack,
 *  stack_new(). Calling it returns a new and empty stack.
 *
 *  Once a stack has benn created, you can push data into or pop it from a stack using stack_push()
 *  and stack_pop(), respectively. stack_peek() also can be used to see what is stored at the top of
 *  a stack without popping it out. Because popping an empty stack triggers an exception
 *  @c assert_exceptfail, calling stack_empty() is recommended to inspect if a stack is empty before
 *  applying stack_pop() to it.
 *
 *  stack_free() destroys a stack that is no longer necessary, but note that any storage that is
 *  allocated by a user program does not get freed with it; stack_free() only returns back the
 *  storage allocated by the library.
 *
 *  As an example, the following code creates a stack and pushes input characters into it until
 *  @c EOF encountered. After that, it prints the characters by popping the characters and destroy
 *  the stack.
 *
 *  @code
 *      int c;
 *      char *p, *q;
 *      stack_t *mystack;
 *
 *      mystack = stack_new();
 *      while ((c = getc(stdin)) != EOF) {
 *          MEM_NEW(p);
 *          *p = c;
 *          stack_push(mystack, p);
 *      }
 *
 *      while (!stack_empty(mystack)) {
 *          p = stack_peek(mystack);
 *          q = stack_pop(mystack);
 *          assert(p == q);
 *          putchar(*p);
 *          MEM_FREE(p);
 *      }
 *      putchar('\n');
 *
 *      stack_free(&mystack);
 *  @endcode
 *
 *  where MEM_NEW() and MEM_FREE() come from the Memory Management Library.
 *
 *  Note that before invoking stack_pop(), the stack is checked whether empty or not by
 *  stack_empty() and that when popping characters, the storage allocated for them gets freed.
 *
 *
 *  @section sec_future Future Directions
 *
 *  No future change on this library planned yet.
 *
 *
 *  @section sec_contact Contact Me
 *
 *  Visit http://project.woong.org to get the lastest version of this library. Only a small portion
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
 *  Copyright (C) 2009-2012 by Jun Woong.
 *
 *  This package is a stack implementation by Jun Woong. The implementation was written so as to
 *  conform with the Standard C published by ISO 9899:1990 and ISO 9899:1999.
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
