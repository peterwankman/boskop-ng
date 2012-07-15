#!/bin/bash

CHOST=i686-pc-linux-gnu
CBUILD=$CHOST

export CLFAGS="-I/usr/include/libxml2 -O0 -ggdb $ADD"
export CPPFLAGS="-I/usr/include/libxml2 -O0 -ggdb $ADD"

svn up

if [ $1 == "--cross" ]; then
	CHOST=arm-unknown-linux-gnu
	shift 1
fi

if [ $1 == --clean ]; then
	aclocal
	autoheader
	autoconf
	automake
	make clean
fi

./configure --enable-xml --enable-ipv6 --host=$CHOST --build=$CBUILD
make clean
make -j3
make install

cp src/*.so bin/
cp src/boskop-ng bin/
