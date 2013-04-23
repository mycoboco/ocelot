/*!
 *  @file    hash.h
 *  @brief    Documentation for Hash Library (CDSL)
 */

/*!
 *  @mainpage    C Data Structure Library: Hash Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2013-04-23
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Hash Library which belongs to the C Data Structure Library. The
 *  basic structure is from David Hanson's book, "C Interfaces and Implementations." I modified the
 *  original implementation to make it more appropriate for my other projects, to speed up
 *  operations, to add missing but useulf facilities and to enhance its readibility; for example
 *  a prefix is used more strictly in order to avoid the user namespace pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving introduction to the library; how to use the facilities is deeply explained
 *  in files that define them.
 *
 *  The Hash Library reserves identifiers starting with @c hash_ and @c HASH_, and imports the
 *  Assertion Library (which requires the Exception Handling Library) and the Memory Management
 *  Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The Hash Library implements a hash table and is one of the most frequently used libraries; it
 *  is essential to get a hash key for datum before putting it into tables by the Table Library or
 *  sets by the Set Library. The storage used to maintain the hash table is managed by the library
 *  and no function in the library demands memory allocation done by user code.
 *
 *  The Hash Library provides one global hash table, so that there is no function that creates a
 *  table or destroy it. A user can start to use the hash table without its creation just by
 *  putting data to it using an appropriate function: hash_string() for C strings, hash_int() for
 *  signed integers and hash_new() for other arbitrary forms of data. Of course, since the library
 *  internally allocates storage to manage hash keys and values, functions to remove a certain key
 *  from the table and to completely clean up the table are offered: hash_free() and hash_reset().
 *  In addition, since strings are very often used to generate hash keys for them, hash_vload() and
 *  hash_aload() are provided and useful especially when preloading several strings onto the table.
 *
 *  @subsection sec_caveats Some Caveats
 *
 *  A common mistake made when using the Hash Library is to pass data to functions that expect a
 *  hash key without making one. For example, table_put() in the Table Library requires its second
 *  argument be a hash key but it is likely to careless write this code to put to a table referred
 *  to as @c mytable a string key and its relevant value:
 *
 *  @code
 *      char *key, *val;
 *      ...
 *      table_put(mytable, key, val);
 *  @endcode
 *
 *  This code, however, does not work because the second argument to table_put() should be a hash
 *  key not a raw string. Thus, the correct one should be:
 *
 *  @code
 *      table_put(mytable, hash_string(key), val);
 *  @endcode
 *
 *  One more thing to note is that hash_string() and similar functions to generate a hash key is an
 *  actual function. If a hash key obtained from your data is frequently used in code, it is better
 *  for efficiency to have it in a variable rather than to call hash_string() several times.
 *
 *  If your compiler rejects to compile the library with a diagnostic including "scatter[] assumes
 *  UCHAR_MAX < 256!" which says @c CHAR_BIT (the number of bits in a byte) in your environment is
 *  larger than 8, you have to add elements to the array @c scatter to make its total number match
 *  the number of characters in your implementation. @c scatter is used to map a character to a
 *  random number. For how to generate the array, see the explanation given for the array in code.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  No boilerplate code is provided for this library.
 *
 *
 *  @section sec_future Future Directions
 *
 *  No future change on this library planned yet.
 *
 *
 *  @section sec_contact Contact Me
 *
 *  Visit http://code.woong.org to get the lastest version of this library. Only a small portion of
 *  my homepage (http://www.woong.org) is maintained in English, thus one who is not good at Korean
 *  would have difficulty when navigating most of other pages served in Korean. If you think the
 *  information you are looking for is on pages written in Korean, do not hesitate to send me an
 *  email to ask for help.
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
 *  This package is a hash table implementation by Jun Woong. The implementation was written so as
 *  to conform with the Standard C published by ISO 9899:1990 and ISO 9899:1999.
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
