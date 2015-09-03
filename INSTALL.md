How to build and install ocelot
===============================

This package does not provide an automated way to install the libraries. After
building them, you need to copy by yourself the resulting files into
appropriate places.

Executing the `make` utility with no target specified lists supported targets:

-  `all`: builds all the libraries into the build directory. The directory has
   two sub-directories, one for header files and the other for static(`.a`) and
   shared objects(`.so`).

-  `cbl`: builds only the libraries belonged to `cbl`. Because the memory
   library has two versions, two versions of `cbl` are generated with different
   names; `cbl` for production and `cbld` for debugging. Necessary headers are
   also placed in the `build/include` directory.

- `cdsl`: builds only the libraries belonged to `cdsl`. Necessary headers are
   also placed in the `build/include` directory.

- `cel`: builds only the libraries belonged to `cel`. Necessary headers are
   also placed in the `build/include` directory.

- `clean`: deletes all files generated when building the libraries. Unless
   you add files, this leaves only the files that were in the distribution.

Even if separate building of libraries is specified, all libraries in `cdsl`
and most in `cel` depend on `cbl`, thus require it.

Some libraries in `cbl` should be informed of the maximum alignment requirement
imposed by the execution environment. If the macro named `MEM_MAXALIGN` is not
defined, they try to guess the requirement. If the guess fails (and it often
does), a program that depends on them also might fail. In such a case, an
explicit value should be given by setting the `CFLAGS` variable as in:

    CFLAGS="-DMEM_MAXALIGN=8" make all

If you are on a 64-bit environment with support for 32-bit emulations and want
32-bit builds of the libraries, add `-m32` to `CFLAGS` and `-m elf_i386` to
`LDFLAGS` as in:

    CFLAGS="-m32 -DMEM_MAXALIGN=8" LDFLAGS="-m elf_i386" make all

(You can see what emulations your `ld` supports by running it with `-V` or
`--verbose`.)

You can also build them as 64-bit binaries without `-m` flags. _Note that,
however, even if the build is successful, `ocelot` does not take full advantage
of 64-bit environments yet._

_If the build fails with an error saying "undefined reference to
`__stack_chk_fail_local`", add `-fno-stack-protector` to `CFLAGS` above._

After the libraries built, you can use them by linking and delivering with
your product, or install them on your system. For system-wide installation, you
need to identify proper places to put the libraries (e.g., `/usr/local/lib` in
most cases, `/usr/local/lib32` for 32-bit builds on a 64-bit machine and
`/usr/local/lib64` for 64-bit builds) and headers (e.g., `/usr/local/include`),
and have permissions to place files there.

For example, on my 64-bit [gentoo](http://www.gentoo.org) machine, the
following instructions install the 32-bit builds with their headers:

    cp -R build/include/* /usr/local/include/
    cp build/lib/* /usr/local/lib32/
    ldconfig

where it is assumed that ld.so.conf has `/usr/local/lib32` in it.

Installed successfully, you can use the libraries by including necessary
headers in your code as in:

    #include <cbl/arena.h>    /* use arena */
    #include <cdsl/hash.h>    /* use hash */
    ...

and invoking your compiler with an option specifying the libraries to use, for
example:

    cc -m32 myprg.c -lcdsl -lcbl

Note that the order in which libraries are given to the compiler is
significant; all of `cdsl` depend on `cbl`, which is the reason `-lcbl` follows
`-lcdsl` in the arguments for the compiler.
