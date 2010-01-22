/*!
 *  @file    memory.h
 *  @brief    Documentation for Memory Management Library (CBL)
 */

/*!
 *  @mainpage    C Basic Library: Memory Management Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2010-01-21
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Memory Management Library which belongs to the C Basic Library. The
 *  basic structure is from David Hanson's book, "C Interfaces and Implementations." I modifies the
 *  original implementation to add missing but useful facilities, to make it conform to the C
 *  standard and to enhance its readibility; for example a prefix is used more strictly in order to
 *  avoid the user namespace pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving introduction to the library, including explanation on its two versions, one
 *  for production code and the other for debugging code. How to use the facilities is deeply
 *  explained in files that define them.
 *
 *  The Memory Management Library reserves identifiers starting with @c mem_ and @c MEM_, and
 *  imports the Assertion Library and the Exception Handling Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The Memory Management Library is intended to substitute calls to the memory
 *  allocation/deallcation functions like malloc() provided by <stdlib.h>. Its main purpose is to
 *  enhance their safety by making them:
 *  - never return a null pointer in any case, which eliminates handling an exceptional case after
 *    memory allocation; failing allocation results in raising an exception (that can be handled by
 *    the Exception Handling Library) rather than in returning a null pointer, and
 *  - set a freed pointer to null, which helps preventing the pointer from being used further.
 *
 *  The following example shows a typical case to allocate/deallocate the storage for a type that a
 *  pointer @c p points to:
 *
 *  @code
 *      {
 *          type_t *p;
 *          MEM_NEW(p);
 *          ...
 *          MEM_FREE(p);
 *      }
 *  @endcode
 *
 *  The user code does not need to check the returned value from MEM_NEW(), because if the
 *  allocation fails, in which case the standard's malloc() returns the null pointer, an exception
 *  named @c mem_exceptfail raised. If you want to do something when the memory allocation fails,
 *  simply establish its handler in a proper place as follows.
 *
 *  @code
 *      EXCEPT_TRY
 *          ... code containing call to allocation functions ...
 *      EXCEPT_EXCEPT(mem_exceptfail)
 *          ... code handling allocation failure ...
 *      EXCEPT_END
 *  @endcode
 *
 *  MEM_NEW0() is also provided to do the same job as MEM_NEW() except that the allocated storage is
 *  initialized to zero-valued bytes.
 *
 *  MEM_FREE() requires that a given pointer be an lvalue, and assigns a null pointer to it after
 *  deallocation. This means that a user should use a temporary object when having only a pointer
 *  value as opposed to an object containing the value, but its benefit that the freed pointer is
 *  prevented from being misused seems overwhelming the inconvenience.
 *
 *  MEM_RESIZE() that is intended to be a wrapper for realloc() differs from realloc() in that its
 *  job is limited to adjusting the size of an allocated area; realloc() allocates as malloc() when
 *  a given pointer is a null pointer, and deallocates as free() when a given size is 0. Thus, a
 *  pointer given to MEM_RESIZE() has to be non-null and a size greater than 0. The justification
 *  for the limitation is given in the book from which this library comes.
 *
 *  MEM_ALLOC() and MEM_CALLOC() are simple wrappers for malloc() and calloc(), and their major
 *  difference from the original functions is, of course, that allocation failure results in raising
 *  an exception.
 *
 *  @subsection subsec_twover Two Versions
 *
 *  This library is provided as two versions, one for production code (memory.c) and the other for
 *  debugging code (memoryd.c). Two versions offers exactly the same interfaces and only their
 *  implementations differ. During debugging code, linking the debugging version is helpful when you
 *  want to figure out if there are invalid memory usages like a free-free problem (that is, trying
 *  to release an already-deallocated area) and a memory leakage. This does not cover the whole
 *  range of such problems as valgrind does, but if there are no other tools available for catching
 *  memory problems, the debugging version of this library would be useful. Unfortunately, the
 *  debugging version is not able to keep track of memory usage unless done through this library;
 *  for example, an invalid operation applied to the storage allocated via malloc() goes undetected.
 *
 *  @subsection subsec_debug Debugging Version
 *
 *  As explained, the debugging version catches certain invalid memory usage. The full list
 *  includes:
 *  - freeing an unallocated area
 *  - resizing an unallocated area and
 *  - listing allocated areas at a given time.
 *
 *  The function implemented in the debugging version print out no diagnostics unless mem_log() is
 *  invoked properly. You can get the list of allocated areas by calling mem_leak() after properly
 *  invoking mem_log().
 *
 *  The diagnostic message printed when an assertion failed changed in C99 to include the name of
 *  the function in which it failed. This can be readily attained with a newly introduced predefined
 *  identifier @c __func__. To provide more useful information, if an implementation claims to
 *  support C99 by defining the macro @c __STDC_VERSION__ properly, the library also includes the
 *  function name when making up the message output when an uncaught exception detected.
 *
 *  @subsection subsec_product Product Version
 *
 *  Even if the product version does not track the memory problems that the debugging version does,
 *  mem_log() and mem_leak() are provided as dummy functions for convenience. See the functions for
 *  more details.
 *
 *  @subsection subsec_caveats Some Caveats
 *
 *  In the implementation of the debugging version, @c MEM_MAXALIGN plays an important role; it is
 *  intended to specify the alignment factor of pointers malloc() returns; without that, a valid
 *  memory operation might be mistaken as an invalid one and stop a running program issuing a wrong
 *  diagnostic message. If @c MEM_MAXALIGN not defined, the library tries to guess a proper value,
 *  but it is not guaranteed for the guess to be always correct. Thus, when compiling the library,
 *  giving an explicit definition of @c MEM_MAXALIGN (via a compiler option like -D, if available)
 *  is recommended.
 *
 *  MEM_ALLOC() and MEM_CALLOC() have the same interfaces as malloc() and calloc() respectively, and
 *  thus their return values should be stored. On the other hand, MEM_NEW() and MEM_RESIZE(), even
 *  if they act as if returning a pointer value, modify a given pointer as the result. This means
 *  that a user codes like:
 *
 *  @code
 *      type_t *p;
 *      p = MEM_NEW(p);
 *  @endcode
 *
 *  might unconsciously trigger undefined behavior since between two sequence points @c p is
 *  modified twice. So remember that any MEM_ functions taking a pointer (including MEM_FREE())
 *  modify the pointer and a user code need not to store explicitly the result to the pointer.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  No boilerplate code is provided for this library.
 *
 *
 *  @section sec_future Future Directions
 *
 *  @subsection subsec_minor Minor Changes
 *
 *  To mimic the behavior of calloc() specified by the standard, it is required for the debugging
 *  version of MEM_CALLOC() to successfully return a null pointer when it cannot allocate the
 *  storage of the requested size. Since this does not allow overflow, it has to carefully check
 *  overflow when calculating the total size.
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
 *  Copyright (C) 2009 by Jun Woong.
 *
 *  This package is a memory management implementation by Jun Woong. The implementation was written
 *  so as to conform with the Standard C published by ISO 9899:1990 and ISO 9899:1999.
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
