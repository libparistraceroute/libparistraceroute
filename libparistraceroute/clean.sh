#!/bin/sh

#test -f Makefile && make clean 
rm -f config.* configure 
rm -rf aclocal.m4 autom4te.cache
rm -f depcomp install-sh libtool ltmain.sh missing stamp-h1
rm -f {doc/,man/,paris-traceroute/,libparistraceroute/,}Makefile{.in,}
rm -rf libparistraceroute/.deps
