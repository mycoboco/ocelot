/*!
 *  @file    conf.h
 *  @brief    Documentation for Configuration File Library (CEL)
 */

/*!
 *  @mainpage    C Environment Library: Configuration File Library
 *  @version     0.2.0
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2011-01-24
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Configuration File Library which belongs to the C Environment
 *  Library. This library reads an "ini-style" configuration file and allows its user to readily
 *  access to values set by the file. There is no de jure standard for "ini" files, but this
 *  library supports most of what the Wikipedia page for the "ini" file
 *  (http://en.wikipedia.org/wiki/INI_file) describes; sections (with no support for nested ones),
 *  line concatenation by a backslash, escape sequences and so on. Differently from other
 *  implementataions, this library supports a simple type system. This aids its users to retrieve
 *  values set by a configuration file without manual conversion to a desired type.
 *
 *  The Configuration File Library reserves identifiers starting with @c conf_ and @c CONF_, and
 *  imports the Assertion Library (which requires the Exception Handling Library), the Memory
 *  Management Library, the Hash Library and the Table Library.
 *
 *  @subsection subsec_concept Concepts
 *
 *  There are several concepts used to specify the Configuration File Library.
 *
 *  "Configuration varaibles" (called simply "variables" hereafter) are variables managed by the
 *  library and set by a default setting, a configuration file or a program. Variables have names
 *  and types.
 *
 *  A set of variable can be grouped and defined to belong to a "configuration section" (called
 *  simply "section" hereafter). A section comprises a distinct namespace, thus two variables with
 *  the same name designate two different variables if they belong to different sections.
 *
 *  The "global section" is an unnamed section that always exists. How to designate the global
 *  section and when variables belong to it are described below. The "current section" is a section
 *  set by a user program so that variables with no section designation are assumed to belong to it;
 *  conf_section() is used to set a section to the current one.
 *
 *  The supported "types" are boolean, signed/unsigned integer, real and string. The string type is
 *  most general and the library provides facilities to convert that type to others.
 *
 *  A "congifuration description table" is an array that has a sequence of variable names and their
 *  properties including a default value. The table also specifies a set of supported sections and
 *  variables. If supplied, the library recognizes sections and variables only appeared in the
 *  table. Otherwise, all sections and variables mentioned in a configuration file are recognized.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  The Configuration File Library reads an "ini-style" configuration file and set variables
 *  according to its contents. The library behaves differently depending on whether a configuration
 *  description table is given by conf_preset(). The table can be composed by creating an array of
 *  @c conf_t and passed to the library through conf_preset(). If conf_preset() is not invoked,
 *  conf_init() has to be used to initiate the library and to read a specified configuration file.
 *  If conf_init() is called after conf_preset(), the contents read from a configuration file
 *  override what the table sets.
 *
 *  A configuration description table gives a list of sections and variables that the library can
 *  recognize and any other section/variable names that appear in a configuration file (but not in
 *  the table) are treated as an error.
 *
 *  If no configuration description table given, the library recognizes all possible sections and
 *  variables during conf_init() reads a configuration file and since there is no way to prescribe
 *  the type of each variable, all variables are assumed to have the string type. Note that unless a
 *  configuration file is protected from a malicious user, the user can exploit it to interfere the
 *  normal starting of a program; lots of sections and variables require the library to consume lots
 *  of memory blocks and thus other parts of a program might fail to perform its job due to lack of
 *  memory. It is essential, thus, to restrict allowable section and variable names by letting
 *  conf_preset() provide a predefined set of names if a configuration file can be exposed to such a
 *  user.
 *
 *  The storage used to maintain a program configuration itself is managed by the library.
 *
 *  @subsection subsec_cdt Configuration Description Tables
 *
 *  If ever invoked, conf_preset() has to be invoked before conf_init(). If a program need not read
 *  a configuration file and uses only predefined settings given through conf_preset(), it need not
 *  call conf_init() at all. A configuration description table is an array of @c conf_t and
 *  enumerates section/variable names, their types and default values. For more details including
 *  how to designate a section and variable in the table and what each field of the table means,
 *  see @c conf_t.
 *
 *  @subsection subsec_file Configuration Files
 *
 *  A configuration file basically has the form of a so-called "ini" file. The file is consisted of
 *  variable-value pairs belonged to a certain section as follows:
 *
 *  @code
 *      [section_1]
 *      var1 = value1
 *      var2 = value2
 *  @endcode
 *
 *  A string between the square brackets specifies a section and variable-value pairs appear below
 *  are belonged to that section. Names for sections and variables have to be consisted of only
 *  digits, alphabets and an underscore (_). By default, names are case-insensitive (setting the
 *  @c CONF_OPT_CASE bit in the second argument to conf_preset() and conf_init() changes this
 *  behavior). They cannot have an embedded space. Digits, alphabets are here dependent on the
 *  locale where the library is used. If a program using the library changes its locale to other
 *  than "C" locale, characters that are allowed for section/variable names also change. Even if
 *  multibyte characters can appear in values, section and variable names cannot have them.
 *
 *  If the pairs are given before any section has not been specified, they belong to the "global"
 *  section. The global section also can be specified by an empty section name as shown below:
 *
 *  @code
 *      []    # global section
 *      var1 = value1
 *  @endcode
 *
 *  A section does not nest and variables belonging to a section need not be gethered.
 *
 *  @code
 *      var0 = value0    # belongs to the global section
 *
 *      [section_1]
 *      var1 = value1    # belongs to section_1
 *
 *      [section_2]
 *      var2 = value2    # belongs to section_2
 *
 *      [section_1]
 *      var3 = value3    # belongs to section_1, now section_1 has two variables
 *  @endcode
 *
 *  Two different sections have the same variable and they are distinct variables.
 *
 *  @code
 *      [section_1]
 *      var1 = value1    # var1 belonging to section_1
 *
 *      [section_2]
 *      var1 = value1    # var1 belonging to section_2
 *  @endcode
 *
 *  If a variable appears with the same name as one that appeared first under the same section, the
 *  value is overwritten by the latter variable setting.
 *
 *  @code
 *      [section_1]
 *      var1 = value1    # var1 has value1
 *      var1 = value2    # var2 now has value2
 *  @endcode
 *
 *  Comments begin with a semicolon (;) or a number sign (#) and ends with a newline as you have
 *  shown in examples above.
 *
 *  If the last character of a line is a backslash (\) without any trailing spaces, its following
 *  line is spliced to the line by eliminating the backslash and following newline. Any whitespaces
 *  preceding the backslash and any leading whitespaces in the following line are replaced with a
 *  space.
 *
 *  @code
 *      [section_1]
 *      var1 = val\
 *      ue             # var1 = value
 *      var2 = value\
 *                 2      # var2 = value 2
 *      var3 = value    \
 *                 3         # var3 = value 3
 *  @endcode
 *
 *  Values following an equal sign (=) after variables can have two forms, quoted and unquoted.
 *  Quoted values have to start with either a double-quote (") or single-quote (') and end with the
 *  matching character; that is, the whole value should be quoted. A semicolon (;) or number sign
 *  (#) in a quoted value does not start a comment.
 *
 *  @code
 *      [section_1]
 *      var1 = "quoted value. ; or # starts no comment"  # now this is comment
 *      var2 = 'quoted value again'
 *      var3 = this is not a "quoted" value
 *  @endcode
 *
 *  The default behavior of the library recognizes no escape sequences, but if the @c CONF_OPT_ESC
 *  bit is set in the second argument to conf_preset() and conf_init(), they are recognized in a
 *  quoted value; an unquoted value supports no escape sequences. The supported sequences are:
 *
 *  @code
 *      \'    \"    \?    \\    \0
 *      \a    \b    \f    \n    \r    \t    \v
 *  @endcode
 *
 *  with the same meanings as defined in C, and also include:
 *
 *  @code
 *      \;    \#    \=
 *  @endcode
 *
 *  that are replaced with a semicolon, number sign and equal sign respectively.
 *
 *  Any leading and trailing whitespaces are omitted from an unquoted value; thus only way to keep
 *  those spaces is to quote the value. Other whitespaces are kept unchanged.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  As already explained, using the library starts with invoking conf_preset() or conf_init(). If
 *  you desire to provide a predefined set of sections and variables with default values,
 *  call conf_preset() before calling conf_init() that reads a configuration file. It is decided
 *  when calling conf_preset() or conf_init() (if conf_preset() has not been invoked) whether names
 *  are case-sensitive and escape sequences are recognized in quoted values.
 *
 *  After reading a configuration file using conf_init(), a user program can freely inspects
 *  variables using conf_get(), conf_getbool(), conf_getint(), conf_getuint(), conf_getreal() and
 *  conf_getstr(). conf_get() retrieves the value of a given variable and interprets it as having
 *  the declared type of the variable. Other functions are useful when a variable from which a value
 *  is to be retrieved has the string type and a user code knows how to interpret it; when a
 *  configuration description table is not used, all variables are assumed to have the string type.
 *  If variables belonging to a specific section are frequently referred to, conf_section() that
 *  changes the current section to a given section helps.
 *
 *  If a function returns an error indicator, an immediate call to conf_errcode() returns the
 *  information about the error and conf_errstr() gives a string describing a given error code that
 *  is useful when constructing error or log messages for users.
 *
 *  This library works on top of the Memory Management Library and if any function that performs
 *  memory allocation fails to get necessary memory, an exception is raised.
 *
 *  conf.c contains an example designed to use as many facilities of the library as possible in a
 *  disabled part and a bolierplate code is given here:
 *
 *  @code
 *      #define CONFFILE "test.conf"
 *
 *      ...
 *
 *      conf_t tab[] = {
 *          "VarBool",          CONF_TYPE_BOOL, "yes",
 *          "VarInt",           CONF_TYPE_INT,  "255",
 *          "VarUint",          CONF_TYPE_UINT, "0xFFFF",
 *          "VarReal",          CONF_TYPE_REAL, "3.14159",
 *          "VarStr",           CONF_TYPE_STR,  "Global VarStr Default",
 *          "Section1.VarBool", CONF_TYPE_BOOL, "FALSE",
 *          "Section1.VarStr",  CONF_TYPE_STR,  "Section1.VarStr Default",
 *          "Section2.VarBool", CONF_TYPE_BOOL, "true",
 *          "Section2.VarReal", CONF_TYPE_REAL, "314159e-5",
 *          NULL,
 *      };
 *      size_t line;
 *      FILE *fp;
 *
 *      if (conf_preset(tab, CONF_OPT_CASE | CONF_OPT_ESC) != CONF_ERR_OK) {
 *          fprintf(stderr, "test: %s\n", conf_errstr(conf_errcode()));
 *          conf_free();
 *          conf_hashreset();
 *          exit(EXIT_FAILURE);
 *      }
 *
 *      fp = fopen(CONFFILE, "r");
 *      if (!fp) {
 *          fprintf(stderr, "test: failed to open %s for reading\n", CONFFILE);
 *          conf_free();
 *          conf_hashreset();
 *          exit(EXIT_FAILURE);
 *      }
 *
 *      line = conf_init(fp, 0);
 *      fclose(fp);
 *
 *      if (line != 0) {
 *          fprintf(stderr, "test:%s:%ld: %s\n", CONFFILE, (unsigned long)line,
 *                  conf_errstr(conf_errcode()));
 *          conf_free();
 *          conf_hashreset();
 *          exit(EXIT_FAILURE);
 *      }
 *
 *      ... sets an internal data structure properly
 *          according to what are read from configuration variables ...
 *
 *      conf_free();
 *      conf_hashreset();
 *
 *      ...
 *  @endcode
 *
 *  Even if this code defines the name of a configuration file as a macro, you may hard-code it or
 *  make it determined from a program argument.
 *
 *  An array of the @c conf_t type, @c tab is a configuration description table. It defines five
 *  variables in the global scope, each of which has the boolean, integer, unsigned integer, real
 *  and string type, respectively. It defines two more sections named "Section1" and "Section2", and
 *  four variables that belong to them. The last value in each row is a default value for each
 *  variable being defined. A null pointer terminates defining the table.
 *
 *  conf_preset() delivers the table to the library. If a problem occurs, conf_preset() returns an
 *  error code (that is not @c CONF_ERR_OK), and you can inspect it further using conf_errcode() and
 *  conf_errstr(). Do not forget that this library is based on data structures using the Memory
 *  Management Library that raises an exception if memory allocation fails.
 *
 *  conf_init() takes a stream (a @c FILE pointer), not a file name. This is because taking a stream
 *  allows its user to hand to conf_init() various kinds of files or file-like objects, for example,
 *  a string connected to a stream which has no file name.
 *
 *  Once conf_init() has done its job, the stream for the configuration file is no longer necessary,
 *  so fclose() closes it.
 *
 *  conf_init() returns 0 if nothing is wrong, or the line number (that is greater than 0) on which
 *  a problem occurs otherwise. You can use the return value when issueing an error message.
 *
 *  Note that if the hash table supported by the Hash Library is used for other purposes, it may not
 *  be desired to call conf_hashreset(). See conf_free() and conf_hashreset() for more details. If
 *  you feel uncomfortable with several instances of calls to conf_free() and conf_hashreset(), you
 *  can introduce a label before clean-up code and jump to that label whenever cleaning-up required.
 *
 *
 *  @section sec_future Future Directions
 *
 *  @subsection subsec_recover Recoverable Errors
 *
 *  The current implementation does not provide a way to recover from errors like encountering
 *  unrecognized sections or variables. Recovering from them is sometimes necessary; for example, a
 *  programmer might want to issue a diagnostic message when a user uses an old version of the
 *  configuration file format, or to construct a certain part of the configuration file format
 *  dynamically depending on other parts of it.
 *
 *  @subsection subsec_minor Minor Changes
 *
 *  table_new() used by the Configuration File Library to create tables for storing configuration
 *  data takes a hint for the expected size of the table to create. Even if the performance is not a
 *  big issue in this library and granting a good hint improves the performance of operations on
 *  tables, providing a reasonable one to table_new() is necessary.
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
 *  Copyright (C) 2009-2011 by Jun Woong.
 *
 *  This package is a configuration file reader implementation by Jun Woong. The implementation was
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
