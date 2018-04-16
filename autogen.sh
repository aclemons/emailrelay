#!/bin/sh
#
# autogen.sh
#
# Autogenerates stuff.
#
aclocal -I .
autoconf
autoheader
automake -ac -Woverride -Wportability
