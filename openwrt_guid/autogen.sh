#!/bin/sh
aclocal
autoheader
libtoolize --automake --copy --debug --force
automake --foreign -a -c
autoconf 
