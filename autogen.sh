#!/bin/sh
#
# autogen.sh
#
# Autogenerates stuff.
#
aclocal
autoconf
autoheader
automake -a -Woverride -Wportability
