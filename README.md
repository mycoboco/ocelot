ocelot: A Language Extension Library
====================================

`ocelot` is a collection of libraries to provide features that the C language
lacks, various data structures that most programs use in common, and facilities
for interaction between a program and its environment.

This package collects libraries into three categories called `CBL`, `CDSL` and
`CEL`. Libraries belonging to `CBL`(C Basic Library) provide features that the
C language lacks and include alternative memory allocators and an exception
handling facility. Those to `CDSL`(C Data Structure Library) implement various
data structures frequently used by most programs. Those to `CEL`(C Environment
Library) aid interaction between programs and the execution environment. Each
library has its own version number and modification logs.

The `cbl`, `cdsl` and `cel` directories contain sub-directories for the
libraries of each category:

- `cbl`: C Basic Library
    - `arena`: The Arena Library (lifetime-based memory allocator)
    - `assert`: The Assertion Library
    - `except`: The Exception Handling Library
    - `memory`: The Memory Management Library
    - `text`: The Text Library (high-level string manipulation)
- `cdsl`: C Data Structure Library
    - `bitv`: The Bit-vector Library
    - `dlist`: The Doubly-linked List Library
    - `hash`: The Hash Library
    - `list`: The List Library (singly-linked list)
    - `set`: The Set Library
    - `stack`: The Stack Library
    - `table`: The Table Library
- `cel`: C Environment Library
    - `conf`: The Configuration File Library (configuration file parser)
    - `opt`: The Option Parsing Library (option parser)

Libraries are documented by [`Doxygen`](http://www.doxygen.org). The `doc`
directory contains HTML, PDF and `man`-page versions of the documents.

`INSTALL.md` explains how to build and install the libraries. For the copyright
issues, see the accompanying `LICENSE.md` file.

If you have a question or suggestion, do not hesitate to contact me via email
(woong.jun at gmail.com) or web (http://code.woong.org/).