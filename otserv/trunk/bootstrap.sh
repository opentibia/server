#!/bin/sh
echo "This file is deprecated, next time use ./autogen.sh instead of ./bootstrap.sh"

aclocal
autoconf
autoheader
automake -a -c
