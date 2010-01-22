/*!
 *  @file    list.h
 *  @brief    Documentation for List Library (CDSL)
 */

/*!
 *  @mainpage    C Data Structure Library: List Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2010-01-22
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the List Library which belongs to the C Data Structure Library. The
 *  basic structure is from David Hanson's book, "C Interfaces and Implementations." I modified the
 *  original implementation to make it more appropriate for my other projects and to enhance its
 *  readibility; for example a prefix is used more strictly in order to avoid the user namespace
 *  pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving introduction to the library; how to use the facilities is deeply explained
 *  in files that define them.
 *
 *  The List Library reserves identifiers starting with @c list_ and @c LIST_, and imports the
 *  Assertion Library (which requires the Exception Handling Library) and the Memory Management
 *  Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The List Library is a typical implementation of a list in which nodes have one pointer to their
 *  next nodes; a list with two pointers to its next and previous nodes is implemented in the
 *  Doubly-Linked List Library. The storage used to maintain a list itself is managed by the
 *  library, but any storage allocated for data stored in nodes should be managed by a user program;
 *  the library provides functions to help it.
 *
 *  Similarly for other data structure libraries, use of the List Library follows this sequence:
 *  create, use and destroy. Except for functions to inspect lists, all other functions do one of
 *  them in various ways.
 *
 *  As oppsed to a doubly-linked list, a singly-linked list does not support random access, thus
 *  there are facilities to aid sequential access to a list: list_toarray(), list_map() and
 *  LIST_FOREACH(). These facilities help a user to convert a list to an array, call a user-defined
 *  function for each node in a list and traverse a list.
 *
 *  As always, if functions that should allocate storage to finish their job fail memory allocation,
 *  an exception @c mem_exceptfail is raised rather than returning an error indicator like a null
 *  pointer.
 *
 *  The following paragraphs describe differences this library has when compared to other data
 *  structure libraries.
 *
 *  In general, pointers that library functions take and return point to descriptors for the data
 *  structure the library implements. Once an instance of the structure is created, the location of
 *  a desciptor for it remains unchanged until destroyed. This property does not hold for pointers
 *  that this library takes and returns. Those pointers in this library point to the head node of a
 *  list rather than a descriptor for it. Because it can be replaced as a result of operations like
 *  adding or removing a node, a user program is obliged to update the pointer variable it passed
 *  with a returned one. Functions that accept a list and return a modified list are list_push(),
 *  list_pop() and list_reverse().
 *
 *  A null pointer, which is considered invalid in other libraries, is a valid and only
 *  representation for an empty list. This means creating a null pointer of the list_t * type in
 *  effect creats an empty list. You can freely pass it to any functions in this library and they
 *  are guaranteed to work well with it. Because of this, functions to add data to a list can be
 *  considered to also create lists; invoking them with a null pointer gives you a list containing
 *  the given data. This includes list_list(), list_append(), list_push() and list_copy().
 *
 *  It is considered good to hide implementation details behind an abstract type with only
 *  interfaces exposed when designing and implementing a data structure. Exposing its implementation
 *  to users often brings nothing beneficial but unnecessary dependency on it. In this
 *  implementation, however, the author decided to expose its implementation since its merits trumph
 *  demerits; see the book for more discussion on this issue.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  Using a list starts with creating it. If you need just an empty list, declaring a variable of
 *  the list_t * type and then making it a null pointer is enough to make one. list_list(),
 *  list_append(), list_push and list_copy() also create a list by providing a null-terminated
 *  sequence of data for each node, combining two lists, pushing a node with a given data to a list
 *  and duplicating a list. As noted, you can use a null pointer as arguments for those functions.
 *
 *  Once a list has been created, a new node can be pushed (list_push()) and inspected
 *  (list_pop()). list_pop() pops a node (that is, gets rid of a node with returning the data in
 *  it). If you need to handle a list as if it were an array, list_toarray() converts a list to a
 *  dynamically-allocated array. You can find the length of the resulting array by calling
 *  list_length() or specifying a value used as a terminator (a null pointer in most cases). A
 *  function, list_map() and a macro, LIST_FOREACH() also provide a way to access nodes in sequence.
 *  list_reverse() reverses a list, which is useful when it is necessary to repeatedly access a
 *  list in the reverse order.
 *
 *  list_free() destroys a list that is no longer necessary, but note that any storage that is
 *  allocated by a user program does not get freed with it; list_free() only returns back the
 *  storage allocated by the library.
 *
 *  As an example, the following code creates a list and stores input characters into each node
 *  until @c EOF encountered. After read, it prints the characters twice by traversing the list and
 *  converting it to an array. Since the last input character resides in the head node, the list
 *  behaves like a stack, which is the reason list_push() and list_pop() are named so. The list is
 *  then reversed and again prints the stored characters by popping nodes; since it is reversed, the
 *  order in which character are printed out differs from the former two cases.
 *
 *  @code
 *      int c;
 *      int i;
 *      char *p;
 *      void *pv, **a;
 *      list_t *mylist, *iter;
 *
 *      mylist = NULL;
 *      while ((c = getc(stdin)) != EOF) {
 *          MEM_NEW(p);
 *          *p = c;
 *          mylist = list_push(mylist, p);
 *      }
 *
 *      LIST_FOREACH(iter, mylist) {
 *          putchar(*(char *)iter->data);
 *      }
 *      putchar('\n');
 *
 *      a = list_toarray(mylist, NULL);
 *      for (i = 0; a[i] != NULL; i++)
 *          putchar(*(char *)a[i]);
 *      putchar('\n');
 *      MEM_FREE(a);
 *
 *      mylist = list_reverse(mylist);
 *
 *      while (list_length(mylist) > 0) {
 *          mylist = list_pop(mylist, &pv);
 *          putchar(*(char *)pv);
 *          MEM_FREE(pv);
 *      }
 *      putchar('\n');
 *
 *      list_free(&mylist);
 *  @endcode
 *
 *  where MEM_NEW() and MEM_FREE() come from the Memory Management Library.
 *
 *  In this example, the storage for each node is returned back when popping nodes from the list.
 *  If list_map() were used instead to free storage, a call like this:
 *
 *  @code
 *      list_map(mylist, mylistfree, NULL);
 *  @endcode
 *
 *  would be used, where a call-back function, mylistfree() is defined as follows:
 *
 *  @code
 *      void mylistfree(void **pdata, void *ignored)
 *      {
 *          MEM_FREE(*pdata);
 *      }
 *  @endcode
 *
 *
 *  @section sec_future Future Directions
 *
 *  @subsection subsec_append Circular Lists
 *
 *  Making lists circular enables appending a new node to them to be done in a constant time. The
 *  current implementation where the last nodes point to nothing makes list_append() take time
 *  proportional to the number of nodes in a list, which is, in other words, the time complexity of
 *  list_append() is O(N).
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
 *  This package is a singly-linked list implementation by Jun Woong. The implementation was written
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
