/*!
 *  @file    arena.h
 *  @brief    Documentation for Arena Library (CBL)
 */

/*!
 *  @mainpage    C Basic Library: Arena Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2011-01-24
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Arena Library which belings to the C Basic Library. The basic
 *  structure is from David Hanson's book, "C Interfaces and Implementations." I modified the
 *  original implementation to make it conform to the C standard and to enhance its readibility; for
 *  example a prefix is used more strictly in order to avoid the user namespace pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving a brief motivation and introduction to the library; how to use the
 *  facilities is deeply explained in files that define them.
 *
 *  malloc() and other related functions from <stdlib.h> provide facilities for the size-based
 *  memory allocation strategy. Each invocation to allocation functions requires a corresponding
 *  invocation to deallocation functions in order to avoid the memory leakage problem. Under certain
 *  circumstances the size-based memory allocator is not the best way to manage storage. For
 *  example, consider a script interpreter that accepts multiple scripts and parses them for
 *  execution in sequence. During running a script, many data structures including trees from a
 *  parser and tables for symbols should be maintained and those structures often require
 *  complicated patterns of memory allocation/deallocation. Besides, they should be correctly freed
 *  after the sciprt has been processed and before processing of the next sciprt starts off. In such
 *  a situation, a lifetime-based memory allocator can work better and simplify the patterns of
 *  (de)allocation. With a lifetime-based memory allocator, all storages allocated for processing a
 *  script is remembered somehow, and all the instances can be freed at a time when processing the
 *  script has finished. The Arena Library provides a lifetime-based memory allocator. Hanson's book
 *  also mentions constructing a graphic user interface (having various features like input forms,
 *  scroll bars and buttons) as an example where a lifetime-based allocator does better than a
 *  size-based one.
 *
 *  The Arena Library reserves identifiers starting with @c arena_ and @c ARENA_, and imports the
 *  Assertion Library and the Exception Handling Library. Even if it does not depend on the Memory
 *  Management Library, it also uses the identifier @c MEM_MAXALIGN.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  Basically, using the Arena Library to allocate storages does not quite differ from using
 *  malloc() or similars from <stdlib.h>. The differences are that every allocation function takes
 *  an additional argument to specify an arena, and that there is no need to invoke a deallocation
 *  function for each of allocated storage blocks; a function to free all storages associated with
 *  an arena is provided.
 *
 *  @code
 *      typeA *p = malloc(sizeof(*p));
 *      ...
 *      typeB *q = malloc(sizeof(*p));
 *      ...
 *      free(p);
 *      free(q);
 *  @endcode
 *
 *  For example, suppose that @c p and @c q point to two areas that have been allocated at different
 *  places but can be freed at the same time. As the number of such instances increases,
 *  deallocating them gets more complicated and thus necessarily more error-prone.
 *
 *  @code
 *      typeA *p = ARENA_ALLOC(myarena, sizeof(*p));
 *      ...
 *      typeB *q = ARENA_ALLOC(myarena, sizeof(*q));
 *      ...
 *      ARENA_FREE(myarena);
 *  @endcode
 *
 *  On the other hand, if the allocator the Arena Library offers is used, only one call to
 *  ARENA_FREE() frees all storages associated with an arena, @c myarena.
 *
 *  Applying to the problem mentioned to introduce a lifetime-based allocator, all storages for data
 *  structures used while a script is processed can be associated with an arena and get freed
 *  readily by ARENA_FREE() before moving to the next script.
 *
 *  As <stdlib.h> provides malloc() and calloc(), this library does ARENA_ALLOC() and ARENA_CALLOC()
 *  taking similar arguments. ARENA_FREE() does what free() does (actually, it does more as
 *  explained above). ARENA_NEW() creates an arena with which allocated storages are associated, and
 *  ARENA_DISPOSE() destroys an arena, which means that an arena may be reused repeatedly even after
 *  all storages with it have been freed by ARENA_FREE().
 *
 *  @subsection subsec_caveats Some Caveats
 *
 *  As in the debugging version of the Memory Management Library, @c MEM_MAXALIGN indicates the
 *  common alignment factor; in other words, the alignment factor of pointers malloc() returns. If
 *  it is not given, the library tries to guess a proper value, but no guarantee that the guess is
 *  correct. Therefore, it is recommended to give a proper definition for @c MEM_MAXALIGN (via a
 *  compiler option like -D, if available).
 *
 *  Note that the Arena Library does not rely on the Memory Management Library. This means it
 *  constitutes a completely different allocator from the latter. Thus, the debugging version of the
 *  Memory Management Library cannot detect problems occurred in the storages maintained by the
 *  Arena Library.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  Using an arena starts with creating it:
 *
 *  @code
 *      arena_t myarena = ARENA_NEW();
 *  @endcode
 *
 *  As in the Memory Management Library, you don't need to check the return value of ARENA_NEW();
 *  an exception named @c arena_exceptfailNew is raised if the creation fails (see the Exception
 *  Handling Library for how to handle exceptions). Creating an arena is different from allocating
 *  necessary storages. With an arena, you can freely allocate storages that belong to it with
 *  ARENA_ALLOC() or ARENA_CALLOC() as in:
 *
 *  @code
 *      sometype_t *p = ARENA_ALLOC(myarena, sizeof(*p));
 *      othertype_t *q = ARENA_CALLOC(myarena, 10, sizeof(*q));
 *  @endcode
 *
 *  Again, you don't have to check the return value of these invocations. If no storage is able to
 *  be allocated, an exception, @c arena_exceptfailAlloc is raised. Due to problems in
 *  implementation, adjusting the size of the allocated storage is not supported; there is no
 *  ARENA_REALLOC() or ARENA_RESIZE() that corresponds to realloc().
 *
 *  There are two ways to release storages with an arena: ARENA_FREE() and ARENA_DISPOSE().
 *
 *  @code
 *      ARENA_FREE(myarena);
 *      ... myarena can be reused ...
 *      ARENA_DISPOSE(myarena);
 *  @endcode
 *
 *  ARENA_FREE() deallocates any storage belonging to an arena, while ARENA_DISPOSE() does the same
 *  job and also destroy the arena to make it no more usable.
 *
 *  If you will use a tool like Valgrind to detect memory problems, see arena_dispose();
 *  ARENA_DISPOSE() is a simple macro wrapper for arena_dispose() to keep the interface to the
 *  library consistent.
 *
 *
 *  @section sec_future Future Directions
 *
 *  @subsection subsec_minor Minor Changes
 *
 *  To mimic the behavior of calloc() specified by the standard, it is required for ARENA_CALLOC()
 *  to successfully return a null pointer when it cannot allocate the storage of the requested size.
 *  Since this does not allow overflow, it has to carefully check overflow when calculating the
 *  total size.
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
 *  This package is a lifetime-based memory allocator implementation by Jun Woong. The
 *  implementation was written so as to conform with the Standard C published by ISO 9899:1990 and
 *  ISO 9899:1999.
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
