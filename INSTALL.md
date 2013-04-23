How To Build and Install ocelot
===============================

This package does not provide an automated way to install the libraries. After
building them, you need to copy by yourself the resulting files into
appropriate places.

Executing the `make` utility with no targe specified lists supported targets:

-  `all`: builds all the libraries into the build directory. The directory has
   two sub-directories, one for header files and the other for static and
   shared objects.

-  `cbl`: builds only the libraries belonged to `CBL`. Since the Memory
   Management Library has two versions, two versions of `CBL` are generated
   with different names; cbl for production code and cbld for debugging code.
   Necessary headers are also placed in the `build/include` directory.

- `cdsl`: builds only the libraries belonged to `CDSL`. Necessary headers are
   also placed in the `build/include` directory.

- `cel`: builds only the libraries belonged to `CEL`. Necessary headers are
   also placed in the `build/include` directory.

- `clean`: deletes all files generated when buliding the libraries. Unless
   you add files, this leaves only the files that were in the distribution.

Even if separate building of libraries is specified, all libraries in `CDSL`
and most in `CEL` depend on `CBL`, thus require it.

Some libraries in `CBL` should be informed of the maximum alignment requirement
imposed by the execution environment. If the macro named `MEM_MAXALIGN` is not
defined, they try to guess the requirement. If the guess fails (and it often
does), a program that depends on them also might fail. In such a case, an
explicit value should be given by setting the `CFLAGS` variable as in:

    CFLAGS="-DMEM_MAXALIGN=8" make all

After the libraries built, you can install them by placing the library files
into a library directory for your system (e.g., `/usr/local/lib`), the headers
into an include directory (e.g., `/usr/local/include`) and the `man`-pages into
a `man`-page directory (e.g., `/usr/local/share/man/man3`; the environmental
variable `$MANPATH` contains a list of paths for `man` pages). For example, on
my machine, the following instructions install the libraries with their
headers:

    cp -R build/include/* /usr/local/include/
    cp build/lib/* /usr/local/lib/
    ldconfig

where it is assumed that ld.so.conf has `/usr/local/lib` in it, and the
following ones install `man` pages for them:

    cp doc/man3/cbl/* /usr/local/share/man/man3/
    cp doc/man3/cdsl/* /usr/local/share/man/man3/
    cp doc/man3/cel/* /usr/local/share/man/man3/

Installed successfully, you can use the libraries by including necessary
headers in your code as in:

    #include <cbl/arena.h>    /* use the Arena Library */
    #include <cdsl/hash.h>    /* use the Hash Library */
    ...

and invoking your compiler with an option specifying the libraries to use, for
example:

    cc myprg.c -lcdsl -lcbl

Note that the order in which libraries are given to the compiler is
significant; all of `CDSL` depend on `CBL`, which is the reason `-lcbl` follows
`-lcdsl` in the arguments for the compiler.