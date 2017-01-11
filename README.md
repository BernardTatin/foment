# about this fork
The only file which is modified is the _unix/makefile_. Usage is :

```bash
make [compiler=gnu|intel|sun] [debug=yes|no] [all|]install|clean]
default values :
  compiler: gnu
  debug   : yes
```
Sun compiler is a work in progress.

Without debug (option _debug=no_), Intel compiler is really faster than _gcc_.

# original README
[Foment](https://github.com/leftmike/foment/wiki/Foment) is an implementation of Scheme.

* Full R7RS.
* Libraries and programs work.
* Native threads and some synchronization primitives.
* Memory management featuring guardians and trackers. Guardians protect objects from being collected and trackers follow objects as they get moved by the copying part of the collector.
* Full Unicode including reading and writing unicode characters to the console. Files in UTF-8 and UTF-16 encoding can be read and written.
* The system is built around a compiler and VM. There is support for prompts and continuation marks.
* Network support.
* Editing at the REPL including ( ) matching.
* Portable: Windows, Mac OS X, Linux, and FreeBSD.
* 32 bit and 64 bit.
* SRFI 1: List Library
* SRFI 60: Integers as Bits
* SRFI 106: Basic socket interface.
* SRFI 111: Boxes.
* SRFI 112: Environment Inquiry.
* SRFI 124: Ephemerons.
* SRFI 128: Comparators.

See [Foment](https://github.com/leftmike/foment/wiki/Foment) for more details.

Future plans include
* Providing line numbers and stack traces on errors.
* R7RS large SRFIs.
* composable continuations

Please note that this is very much a work in progress. Please let me know if
you find bugs and omissions. I will do my best to fix them.

mikemon@gmail.com
