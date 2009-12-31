/*!
 *  @file    text.h
 *  @brief    Documentation for Text Library (CBL)
 */

/*!
 *  @mainpage    C Basic Library: Text Library
 *  @version     0.2.1
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2009-12-29
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Text Library which belongs to the C Basic Library. The basic
 *  structure is from David Hanson's book, "C Interfaces and Implementations." I modified the
 *  original implementation to add missing but useful functions, to make it conform to the C
 *  standard and to enhance its readibility; for example a prefix is used more strictly in order to
 *  avoid the user namespace pollution.
 *
 *  Since the book explains its design and implementation in a very comprehensive way, not to
 *  mention the copyright issues, it is nothing but waste to repeat it here, so I finish this
 *  document by giving introduction to the library; how to use the facilities is deeply explained
 *  in files that define them.
 *
 *  The Text Library reserves identifiers starting with @c text_ and @c TEXT_, and imports the
 *  Assertion Library (which requires the Exception Handling Library) and the Memory Management
 *  Library.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The Text Library is intended to aid string manipulation in C. In C, even a simple form of string
 *  handling like obtaining a sub-string requires lengthy code performing memory allocation and
 *  deallocation. This is mainly because strings in C end with a null character, which interferes
 *  the storage for a single string from being shared for representing its sub-strings. The Text
 *  Library provides an alternative representation for strings, that is composed of a sequence of
 *  characters (not necessarily terminated by a null) and its length in byte. This representation
 *  helps many string operations to be efficient. In addition to it, the storage necessary for the
 *  strings is almost completely controled by the library; every allocation done by the library is
 *  remembered internally, and a user has, even if not complete, control over it.
 *
 *  For example, consider two typical cases to handle strings: obtaining a sub-string and appending
 *  a string to another string.
 *
 *  @code
 *      char *t;
 *      t = malloc(strlen(s+n) + 1);
 *      if (!t)
 *          ...
 *      strcpy(t, s+n);
 *      ...
 *      free(t);
 *  @endcode
 *
 *  This code shows a typical way in C to get a sub-string from a string @c s and saves it to @c t.
 *  Since the string length is often not predictable, it is essential for a product-level program to
 *  dynamically allocate storage for the sub-string, and it is obliged not to forget to release it.
 *  Using the Text Library, this construct changes to:
 *
 *  @code
 *      text_t ts, tt;
 *      ts = text_put(s);
 *      tt = text_sub(ts, m, 0);
 *  @endcode
 *
 *  where text_put() converts a C string @c s to a @c text_t string @c ts, and text_sub() gets a
 *  sub-string from it. text_put() and text_sub() allocate any necessary storage and a user does not
 *  have to take care of it.
 *
 *  @code
 *      char *s1, *s2, *t;
 *      t = malloc(strlen(s1)+strlen(s2) + 1);
 *      if (!t)
 *          ...
 *      strcpy(t, s1);
 *      strcat(t, s2);
 *      ...
 *      free(t);
 *  @endcode
 *
 *  Similarly, this code appends a string @c s2 to another string @c s1, and saves the result to
 *  @c t. Not to mention that the code repeats unnecessary scanning of strings, managing storages
 *  allocated for strings is quite burdensome. Compare this code to a version using the Text
 *  Library:
 *
 *  @code
 *      text_t ts1, ts2, tt;
 *      ts1 = text_put(s1);
 *      ts2 = text_put(s2);
 *      tt = text_cat(ts1, ts2);
 *  @endcode
 *
 *  All things it has to do is to convert the C strings to their @c text_t string and to apply the
 *  string concatenating operation to them.
 *
 *  As you can see in the examples above, there is an extra expense to convert between C strings and
 *  @c text_t strings, but the merit that @c text_t strings bring is quite significant and that
 *  expense should not be that big if unnecessary conversions are eliminated by a good program
 *  design.
 *
 *  In general, referring to a character in a string is achieved by calculating an index of the
 *  character in the array for the string. In this library, a new and more convenient scheme to
 *  refer to certain positions in a string is introduced; see text_pos() for details. Just to
 *  mention one advantage the new scheme has, in order to refer to the end of a string, there is no
 *  need to call a string-length function or to inspect the @c len member of a @c text_t object;
 *  passing 0 to a position parameter of a library function is enough.
 *
 *  Managing the storage for @c text_t strings (called "the text space") is similar to record the
 *  state of the text space and to restore it to one of its previous states. Whenever the library
 *  allocates storage for a string, it acts as if it changes the state of the text space. A user
 *  code records the state when it wants and can deallocate any storage allocated after that record
 *  by restoring the text space to the remembered state; you might notice that the text space
 *  behaves like a stack containing the allocated chunks. text_save() and text_restore() explain
 *  more details.
 *
 *  @subsection subsec_caveats Some Caveats
 *
 *  A null character that terminates a C string is not special in handling a @c text_t string. This
 *  means that a @c text_t string can have embedded null characters in it and all functions except
 *  for one converting to a C string treat a null indifferently from a normal character. Note that a
 *  @c text_t string does not need to end with a null character.
 *
 *  On the contrary, nothing prevents a @c text_t string from ending with a null character; to be
 *  precise, the string contains (rather than ends with) the null character as its part. It is
 *  sometimes useful to have a @c text_t string contain a null character, especially when converting
 *  it to a C string occurs very frequently; note that, however, placing a null character in a
 *  @c text_t string prohibits other strings from sharing the storage with it, which is to give up
 *  the major advantage the Text Library offers.
 *
 *  Functions in this library always generate a new string for the result. Comparing strcat() to
 *  text_cat() shows what this means:
 *
 *  @code
 *      strcat(s1, s2);
 *  @endcode
 *
 *  Assuming that the area @c s1 points to is big enough to contain the result, strcat() modifies
 *  the string @c s1 by appending @c s2 to it. An equivalent @c text_t version is as follows:
 *
 *  @code
 *      t = text_cat(s1, s2);
 *  @endcode
 *
 *  where @c t is the resulting string and, @c s1 and @c s2 are unchanged. This difference, even if
 *  looks very small, often leads an unintended bug like writing this kind of code:
 *
 *  @code
 *      text_cat(s1, s2);
 *  @endcode
 *
 *  and expecting @c s1 to point to the resulting string. The same caution goes for text_dup() and
 *  text_reverse(), too.
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  A typical use of the Text Library starts with recording the state of the text space for managing
 *  storage:
 *
 *  @code
 *      text_save_t *chckpt;
 *
 *      chckpt = text_save();
 *  @endcode
 *
 *  Since the state of the text space is kept before any other Text Library functions are invoked,
 *  restoring the state to what is kept in @c chckpt effectively releases all storages the Text
 *  Library allocates. If you don't mind the memory leakage problem, you may ignore about saving and
 *  restoring the text space state.
 *
 *  Then, the program can generate a @c text_t string from a C string (text_box(), text_put() and
 *  text_gen()), convert a @c text_string back to a C string (text_get()), apply various string
 *  operations (text_sub(), text_cat(), text_dup() and text_reverse()) including mapping a string to
 *  another string (text_map()), compare two strings (text_cmp()), locate a character in a string
 *  (text_chr(), text_rchr(), text_upto(), text_rupto(), text_any(), text_many() and text_rmany()),
 *  and locate a string in another string (text_find(), text_rfind(), text_match() and
 *  text_rmatch()). To aid an access to the internal of strings, text_pos() and TEXT_ACCESS() are
 *  provided.
 *
 *  Finishing jobs using @c text_t strings, the following code that corresponds to the above call to
 *  text_save() restores the state of the text space:
 *
 *  @code
 *      text_restore(&chckpt);
 *  @endcode
 *
 *  As explained in text_save(), there is no requirement that text_save() and its corresponding
 *  text_restore() be called only once; see text_save() and text_restore() for details.
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
 *  Copyright (C) 2009 by Jun Woong.
 *
 *  This package is a string manipulation implementation by Jun Woong. The implementation was
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
