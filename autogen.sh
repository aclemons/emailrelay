#!/bin/sh
#
# autogen.sh
#
# Autogenerates stuff.
#
aclocal
autoconf
autoheader
automake -ac -Woverride -Wportability
