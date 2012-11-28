/*!
 *  @file    dlist.h
 *  @brief    Documentation for Doubly-Linked List Library (CDSL)
 */

/*!
 *  @mainpage    C Data Structure Library: Doubly-Linked List Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2011-01-24
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Double-Linked List Library which belongs to the C Data Structure
 *  Library. The basic structure is from David Hanson's book, "C Interfaces and Implementations." I
 *  modified the original implementation to make it more appropriate for my other projects, to speed
 *  up operations and to enhance its readibility; for example a prefix is used more strictly in
 *  order to avoid the user namespace pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving introduction to the library; how to use the facilities is deeply explained in
 *  files that define them.
 *
 *  The Doubly-Linked List Library reserves identifiers starting with @c dlist_ and @c DLIST_, and
 *  imports the Assertion Library (which requires the Exception Handling Library) and the Memory
 *  Management Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The Doubly-Linked List Library is a typical implementation of a list in which nodes have two
 *  pointers to their next and previous nodes; a list with a unidirectional pointer is implemented
 *  in the List Library. The storage used to maintain a list itself is managed by the library, but
 *  any storage allocated for data stored in nodes should be managed by a user program.
 *
 *  Similarly for other data structure libraries, use of the Doubly-Linked List Library follows this
 *  sequence: create, use and destroy. Except for functions to inspect lists, all other functions do
 *  one of them in various ways.
 *
 *  As oppsed to a singly-linked list, a doubly-linked list enables its nodes to be accessed
 *  randomly. To speed up such accesses, the library is revised from the original version so that a
 *  list remembers which node was last accessed. If a request is made to access a node that is next
 *  or previous to the remembered node, the library locates it starting from the remembered node.
 *  This is from observation that traversing a list from the head or the tail in sequence occurs
 *  frequently in many programs and can make a program making heavy use of lists run almost 3 times
 *  faster. Therefore, for good performance of your program, it is highly recommended that lists are
 *  traversed sequentially whenever possible. Do not forget that the current implementation requires
 *  for other types of accesses (that is, any access to a node that is not immediately next or
 *  previous to a remembered node) the library to locate the desired node from the head or the tail.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  Using a list starts with creating one. The simplest way to do it is to call dlist_new().
 *  dlist_new() returns an empty list, and if it fails to allocate storage for the list, an
 *  exception @c mem_exceptfail is raised rather than returning a null pointer. All functions that
 *  allocate storage signals a shortage of memory via the exception; no null pointer returned.
 *  There is another function to create a list: dlist_list() that accepts a sequence of data and
 *  creates a list containing them in each node.
 *
 *  Once a list has been created, a new node can be inserted in various ways (dlist_add(),
 *  dlist_addhead() and dlist_addtail()) and an existing node can be removed from a list also in
 *  various ways (dlist_remove(), dlist_remhead() and dlist_remtail()). You can inspect the data of
 *  a node (dlist_get()) or replace it with new one (dlist_put()). In addition, you can find the
 *  number of nodes in a list (dlist_length()) or can rotate (or shift) a list (dlist_shift()). For
 *  an indexing sheme used when referring to an existing node, see dlist_get(). For that used when
 *  referring to a position into which a new node inserted, see dlist_add().
 *
 *  dlist_free() destroys a list that is no longer necessary, but note that any storage that is
 *  allocated by a user program does not get freed with it; dlist_free() only returns back the
 *  storage allocated by the library.
 *
 *  As an example, the following code creates a list and stores input characters into each node
 *  until @c EOF encountered. After read, it copies characters in nodes to continuous storage area
 *  to construct a string and prints the string.
 *
 *  @code
 *      int c;
 *      char *p, *q;
 *      dlist_t *mylist;
 *
 *      mylist = dlist_new();
 *
 *      while ((c = getc(stdin)) != EOF) {
 *          MEM_NEW(p);
 *          *p = c;
 *          dlist_addtail(mylist, p);
 *      }
 *
 *      n = dlist_length(mylist);
 *
 *      p = MEM_ALLOC(n+1);
 *      for (i = 0; i < n; i++) {
 *          p = dlist_get(mylist, i);
 *          q[i] = *p;
 *          MEM_FREE(p);
 *      }
 *      q[i] = '\0';
 *
 *      dlist_free(&mylist);
 *
 *      puts(q);
 *  @endcode
 *
 *  where MEM_NEW(), MEM_ALLOC() and MEM_FREE() come from the Memory Management Library.
 *
 *  Note that, before adding a node to a list, unique storage to contain a character is allocated
 *  with MEM_NEW() and this storage is returned back by MEM_FREE() while copying characters into an
 *  allocated array.
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
 *  This package is a doubly-linked list implementation by Jun Woong. The implementation was written
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
