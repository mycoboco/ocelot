/*!
 *  @file    opt.h
 *  @brief    Documentation for Option Parsing Library (CEL)
 */

/*!
 *  @mainpage    C Environment Library: Option Parsing Library
 *  @version     0.2.0
 *  @author      Jun Woong (woong.jun at gmail.com)
 *  @date        last modified on 2013-04-23
 *
 *
 *  @section sec_intro Introduction
 *
 *  This document specifies the Option Parsing Library which belongs to the C Environment Library.
 *  This library is intended to implement all features of Linux's getopt() and getopt_long() in an
 *  integrated and thus more consistent fashion; the funtionality of getopt() specified by POSIX is
 *  also subsumed by the library.
 *
 *  Precisely, this library:
 *  - supports three ordering modes - argument permutation mode, POSIX-compliant mode and
 *    "return-in-order" mode (see below);
 *  - allows multiple scans of possibly multiple sets of program arguments;
 *  - preserves the original program arguments in its original order;
 *  - supports optional long-named options;
 *  - supports optional short-named options; and
 *  - supports abbreviated names for long-named options.
 *
 *  (Suppose that a program supports three long-named options "--html", "--html-false" and
 *  "--html-true". For various incomplete options given, the library behaves as intuitively as
 *  possible, for example, "--html-f" is considered "--html-false", "--html" is recognized as it is
 *  and "--html-" results in a warning for its ambiguity. This feature is called "abbreviated names
 *  for long-named options.")
 *
 *  The Option Parsing Library reserves identifiers starting with @c opt_ and @c OPT_, and imports
 *  no other libraries except for the standard library.
 *
 *  @subsection subsec_concept Concepts
 *
 *  There are several concepts used to specify the Option Parsing Library.
 *
 *  "Program arguments" or "arguments" for brevity refer to anything given to a program being
 *  executed.
 *
 *  "Operands" refer to arguments not starting with a hyphen character or to "-" that denotes the
 *  standard input stream. These are sometimes referred to as "non-option arguments."
 *
 *  "Options" refer to arguments starting with a hyphen character but excluding "-". "Short-named
 *  options" are options that start with a single hyphen and have a single character following as
 *  in "-x"; several short-named options can be grouped after a hyphen as in "-xyz" which is
 *  equivalent to "-x -y -z". "Long-named options" are options that start with two hyphens and
 *  have a non-empty character sequence following; for example, "--long-option".
 *
 *  If an option takes an additional argument which may immediately follow (possibly with an
 *  intervening equal sign) or appear as a separate argument, the argument is called an
 *  "option-argument." For long-named options, option-arguments must follow an equal sign unless
 *  they appear as separate ones. (See IEEE Std 1003.1, 2004 Edition, 12. Utility Conventions.)
 *
 *  @warning    Note that, if an option takes an option-argument that is negative thus starts with
 *              a minus sign, the argument cannot be a separate one, since the separate argument is
 *              to be recognized as another option.
 *
 *  An "option description table" is an array that has a sequence of options to recognize and their
 *  properties.
 *
 *
 *  @section sec_usage How to Use The Library
 *
 *  Most programs parse program options in very similar ways. A typical way to handle options is
 *  given as the boilerplate code below. You can simply copy it and modify to add options your
 *  program supports to the option description table and case labels.
 *
 *  The storage used to parse program arguments is managed by the library.
 *
 *  @subsection subsec_order Ordering Modes
 *
 *  By default, the library processes options and operands as if they were permutated so that
 *  operands always follow options. That is, the following two invocations of "util" (where no
 *  options take option-arguments) are equivalent (i.e., program cannot tell the difference):
 *
 *  @code
 *      util -a -b inputfile outputfile
 *      util inputfile -a outputfile -b
 *  @endcode
 *
 *  This behavior canned "argument permutation," in most cases, helps users to flexibly place
 *  options among operands. Some programs, however, require options always proceed operands; for
 *  example, given the following line,
 *
 *  @code
 *      util -a util2 -b
 *  @endcode
 *
 *  it might be wanted to interpret this as giving "-a" to "util" but "-b" to "util2" which cannot
 *  be achieved with argument permutation. For such a case, this library provdes two modes to keep
 *  the order in which options and operands are given: the POSIX-compliant mode and the
 *  "return-in-order" mode which are denoted by @c REQUIRE_ORDER and @c RETURN_IN_ORDER in a
 *  typical implementation of getopt().
 *
 *  In the POSIX-compliant mode, parsing options stops immediately whenever an operand is
 *  encountered. This behavior is what POSIX requires, as its name implies.
 *
 *  In the "return-in-order" mode, encountering operands makes the character valued 1 returned as
 *  if the operand is an option-argument for the option whose short name has the value 1.
 *
 *  This ordering mode can be controlled by marking a desired ordering mode in an option
 *  description table or setting an environment variable (see @c opt_t).
 *
 *  @subsection subsec_odt Option Description Tables
 *
 *  An option description table specifies what options should be recognized with their long and
 *  short names and what should be done when encountering them, for example, whether an additional
 *  option-argument is taken and what its type is, or whether a flag is set and what should be
 *  stored into it. Including the ordering mode, all behaviors of the library can be controlled by
 *  the table. See @c opt_t for more detailed explanation.
 *
 *
 *  @section sec_boilerplate Boilerplate Code
 *
 *  Using the library starts with invoking opt_init(). It takes an option description table,
 *  pointers to parameters of main(), a pointer to an object to which additional information goes
 *  during parsing arguments, a default program name used when no program name is available from
 *  the environment and a directory separator. If succeeds, it returns a program name; it can be
 *  used to issue messages for example.
 *
 *  After the library initialized, opt_parse() insepcts each program argument and performs what
 *  specified by the option description table for it. In most cases, this process is made up of a
 *  loop containing jumps based on the return value of opt_parse().
 *
 *  When it is necessary to compare a string argument (that is resulted from an argument of the @c
 *  OPT_TYPE_STR type) with a set of strings to set a variable to an integral value depending on
 *  the string, opt_val() would help in most cases; see an example in opt.c.
 *
 *  As opt_prase() reports that all options have been inspected, a program is granted an access to
 *  remaining non-option arguments. These operands are inspected as if they were only arguments to
 *  the program.
 *
 *  Since opt_init() allocates storages for duplicating pointers to program arguments, opt_free()
 *  should be invoked in order to avoid memory leakage after handling operands has finished.
 *
 *  opt_abort() is a function that stops recognization of options being performed by conf_parse().
 *  All remaining options are regarded as operands. It is useful when a program introduces an
 *  option stopper like "--" for its own purposes.
 *
 *  opt.c contains an example designed to use as many facilities of the library as possible in a
 *  disabled part and a bolierplate code that is a simplified version of the example is given here:
 *
 *  @code
 *      static struct {
 *          const char *prgname;
 *          int verbose;
 *          int connect;
 *          ...
 *      } option;
 *
 *      int main(int argc, char *argv[])
 *      {
 *          opt_t tab[] = {
 *              "verbose", 0,           &(option.verbose), 1,
 *              "add",     'a',         OPT_ARG_NO,        OPT_TYPE_NO,
 *              "create",  'c',         OPT_ARG_REQ,       OPT_TYPE_STR,
 *              "number",  'n',         OPT_ARG_OPT,       OPT_TYPE_REAL,
 *              "connect", UCHAR_MAX+1, OPT_ARG_REQ,       OPT_TYPE_STR,
 *              "help",    UCHAR_MAX+2, OPT_ARG_NO,        OPT_TYPE_NO,
 *              NULL,
 *          }
 *
 *          option.prgname = opt_init(tab, &argc, &argv, &argptr, PRGNAME, '/');
 *          if (!option.prgname) {
 *              fprintf(stderr, "%s: failed to parse options\n", PRGNAME);
 *              return EXIT_FAILURE;
 *          }
 *
 *          while ((c = opt_parse()) != -1) {
 *              switch(c) {
 *                  case 'a':
 *                      ... --add or -a given ...
 *                      break;
 *                  case 'c':
 *                      printf("%s: option -c given with value '%s'\n, option.prgname,
 *                             (const char *)argptr);
 *                      break;
 *                  case UCHAR_MAX+1:
 *                      {
 *                          opt_val_t t[] = {
 *                              "stdin",   0, "standard input",  0,
 *                              "stdout",  1, "standard output", 1,
 *                              "stderr",  2, "standard error",  2,
 *                              NULL,     -1
 *                          };
 *                          option.connect =
 *                              opt_val(t, argptr, OPT_CMP_CASEIN | OPT_CMP_NORMSPC);
 *                          if (option.connect == -1) {
 *                              printf("%s: `stdin', `stdout' or `stderr' must be given for "
 *                                     "--connect\n", option.prgname);
 *                              opt_free();
 *                              return EXIT_FAILURE;
 *                          }
 *                      }
 *                      break;
 *                  case 'n':
 *                      printf("%s: option -n given", option.prgname);
 *                      if (argptr)
 *                          printf(" with value '%f'\n", *(const double *)argptr);
 *                      else
 *                          putchar('\n');
 *                      break;
 *                  case UCHAR_MAX+2:
 *                      printf("%s: option --help given\n", option.prgname);
 *                      opt_free();
 *                      return 0;
 *
 *                  case 0:
 *                      break;
 *                  case '?':
 *                  case '-':
 *                  case '+':
 *                  case '*':
 *                      fprintf(stderr, "%s: ", option.prgname);
 *                      fprintf(stderr, opt_errmsg(c), (const char *)argptr);
 *                      opt_free();
 *                      return EXIT_FAILURE;
 *                  default:
 *                      assert(!"not all options covered -- should never reach here");
 *                      break;
 *              }
 *          }
 *
 *          if (option.verbose)
 *              puts("verbose flag is set");
 *          printf("connect option is set to %d\n", option.connect);
 *
 *          if (argc > 1) {
 *              printf("non-option ARGV-arguments:");
 *              for (i = 1; i < argc; i++)
 *                  printf(" %s", argv[i]);
 *              putchar('\n');
 *          }
 *
 *          opt_free();
 *
 *          return 0;
 *      }
 *  @endcode
 *
 *  The struct object @c option manages all objects set by program arguments. Note that it has the
 *  static storage duration; since its member is used as an initializer for the option description
 *  table that is an array, it has to have the static storage duration; C99 has removed this
 *  restriction.
 *
 *  Each row in the option description table specifies options to recognize:
 *  - "--verbose" has no short name and has a flag variable set to 1 when encountered;
 *  - "--add" has a short name "-a" and takes no option-arguments;
 *  - "--create" has a short name "-c" and requires an option-argument of the string type;
 *  - "--number" has a short name "-n" and takes an optional option-argument of the real type; and
 *  - "--help" has no short name and takes no option-arguments.
 *
 *  Because the failure of opt_init() means that memory allocation failed, you do not have to call
 *  opt_free() before terminating the program.
 *
 *  The case labes above one handling 0 are for options given in @c tab. Those labels below them
 *  are for exceptional cases and opt_errmsg() helps to construct appropriate messages for them. In
 *  addtion, there are other ways to handle those cases; see opt_errmsg() for details. Remember
 *  that, if invoked, opt_free() should be invoked after all program arguments including non-option
 *  arguments have been processed. Since opt_init() makes copies of pointers in @c argv and
 *  opt_free() releases storages for them, any access to them gets invalidated by opt_free().
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
 *  Copyright (C) 2009-2013 by Jun Woong.
 *
 *  This package is an option parser implementation by Jun Woong. The implementation was written so
 *  as to conform with the Standard C published by ISO 9899:1990 and ISO 9899:1999.
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
