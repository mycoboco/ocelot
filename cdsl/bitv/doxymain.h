/*!
 *  @file    bitv.h
 *  @brief    Documentation for Bit-vector Library (CDSL)
 */

/*!
 *  @mainpage    C Data Structure Library: Bit-vector Library
 *  @version     0.3.0
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2013-04-19
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Bit-vector Library which belongs to the C Data Structure Library.
 *  The basic structure is from David Hanson's book, "C Interfaces and Implementations." I modifies
 *  the original implementation to make it more appropriate for my other projects and to enhance
 *  its readibility; for example a prefix is used more strictly in order to avoid the user
 *  namespace pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving introduction to the library; how to use the facilities is deeply explained in
 *  files that define them.
 *
 *  The Bit-vector Library reserves identifiers starting with @c bitv_ and @c BITV_, and imports the
 *  Assertion Library (which requires the Exception Handling Library) and the Memory Management
 *  Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The Bit-vector Library implements a bit-vector that is a set of integers. A unsigned integer
 *  type like @c unsigned @c long or a bit-field in a struct or union is often used to represent
 *  such a set, and various bitwise operators serve set operations; for example, the bitwise OR
 *  operator effectively provides a union operation. This approach, however, imposes a restriction
 *  that the size of a set should be limited by that of the primitive integer type chosen. This
 *  Bit-vector Library gets rid of such a limit and allows users to use a set of integers with an
 *  arbitrary size at the cost of dynamic memory allocation and a more complex data structure than
 *  a simple integer type.
 *
 *  Similarly for other data structure libraries, use of the Bit-vector Library follows this
 *  sequence: create, use and destroy.
 *
 *  In general, a null pointer given to an argument expecting a bit-vector is considered an error
 *  which results in an assertion failure, but the functions for set operations (bitv_union(),
 *  bitv_inter(), bitv_minus() and bitv_diff()) take a null pointer as a valid argument and treat
 *  it as representing an empty (all-bits-cleared) bit-vector. Also note that they always produce a
 *  distinct bit-vector; none of them alters the original set.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  Using a bit-vector starts with creating one using bitv_new(). There are other ways to create
 *  bit-vectors from an existing one with bitv_union(), bitv_inter(), bitv_minus() and bitv_diff()
 *  (getting a union, intersection, difference, symmetric difference of bit-vectors, respectively);
 *  all bit-vector creation functions allocate storage for a bit-vector to create and if no
 *  allocation is possible, an exception is raised instead of returning a failure indicator like a
 *  null pointer.
 *
 *  Once a bit-vector created, a bit in the vector can be set and cleared using bitv_put(). A
 *  sequence of bits also can be handled in group using bitv_set(), bitv_clear(), bitv_not() and
 *  bitv_setv(); unlike a generic set, the concept of a universe can be defined for integral sets,
 *  thus we can have a function for complement. set_get() inspects if a certain bit is set in a
 *  bit-vector, and bitv_length() gives the size (or the length) of a bit-vector while bitv_count()
 *  counts the number of bits set in a given bit-vector.
 *
 *  bitv_map() offers a way to apply some operations on every bit in a bit-vector; it takes a
 *  user-defined function and calls it for each of bits.
 *
 *  bitv_free() takes a bit-vector (to be precise, a pointer to a bit-vector) and releases the
 *  storage used to maintain it.
 *
 *  As an example, the following code creates two bit-vectors each of which has 60 bits. It then
 *  obtains two more from getting a union and intersection of them after setting bits in different
 *  ways. It ends with printing bits in each set using a user-provided function and destroying all
 *  vectors.
 *
 *  @code
 *       int len;
 *       bitv_t *s, *t, *u, *v;
 *       unsigned char a[] = { 0x42, 0x80, 0x79, 0x29, 0x54, 0x19, 0xFF };
 *
 *       s = bitv_new(60);
 *       t = bitv_new(60);
 *       len = bitv_length(s);
 *
 *       bitv_set(s, 10, 50);
 *       bitv_setv(t, a, 7);
 *
 *       u = bitv_union(s, t);
 *       v = bitv_inter(s, t);
 *
 *       bitv_map(s, print, &len);
 *       bitv_map(t, print, &len);
 *       bitv_map(u, print, &len);
 *       bitv_map(v, print, &len);
 *
 *       bitv_free(&s);
 *       bitv_free(&t);
 *       bitv_free(&u);
 *       bitv_free(&v);
 *  @endcode
 *
 *  where print() is defined as follows:
 *
 *  @code
 *      static void print(size_t n, int v, void *cl)
 *      {
 *          printf("%s%d", (n > 0 && n % 8 == 0)? " ": "", v);
 *          if (n == *(int *)cl - 1)
 *              puts("");
 *      }
 *  @endcode
 *
 *
 *  @section sec_contact Contact Me
 *
 *  Visit http://code.woong.org to get the lastest version of this library. Only a small portion
 *  of my homepage (http://www.woong.org) is maintained in English, thus one who is not good at
 *  Korean would have difficulty when navigating most of other pages served in Korean. If you think
 *  the information you are looking for is on pages written in Korean, do not hesitate to send me
 *  an email to ask for help.
 *
 *  Any comments about the library are welcomed. If you have a proposal or question on the library
 *  just email me, and I will reply as soon as possible.
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
 *  Copyright (C) 2009-2013 by Jun Woong.
 *
 *  This package is a set implementation by Jun Woong. The implementation was written so as to
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
