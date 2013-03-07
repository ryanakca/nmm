NMM, TMM, and TWMM
==================

`NMM`, `TMM`, and `TWMM` are a C and curses implementation of the
popular games Nine Man Morris, Three Man Morris, and Twelve Man
Morris.

Compiling
---------

To compile, you will need a C compiler, and a copy of the ncurses
header files available. Then a simple

	make

will cause `nmm` and company to be compiled (in fact, `tmm` and `twmm`
are no more than symlinks to `nmm`, displaying the correct game based
on the basename). To install `nmm`, go

	make install

which will install under the prefix of `/usr/local` (see `Makefile`
for details). If this isn't the desired location, you can override
this setting with

	make install -ePREFIX=/your/prefix install

Man pages for `nmm` and company will be installed under
`$(PREFIX)/man6/` and the `whatis` database will be updated with a
call to `/usr/libexec/makewhatisdb`. If you wish to change this,
please override the `MANPATH` and `MAKEWHATIS` variables when you call
the install target.

LICENSE
-------

`nmm` and company are distributed Copyright (C) 2013 Ryan Kavanagh
<rak@debian.org> and are distributed under a 3-clause revised-BSD
license. Please see the file `LICENSE` for details.
