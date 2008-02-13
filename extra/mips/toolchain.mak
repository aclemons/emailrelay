#
## Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
## 
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or 
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
# Makefile / toolchain.mak
#
# A makefile for building a cross-compiling toolchain for little-endian 
# mips ("mipsel") using:
#
#  * gcc 3.4.6
#  * linux 2.4.20
#  * uclibc 0.9.27
#  * uclibc++ 0.2.2
#  * binutils 2.18-ubuntu
#
# These versions of linux and uclibc correspond to those pre-installed
# on the Buffalo WHR-G54S router as of late 2007. The version of gcc
# is 3.4.6 rather than 3.3.3 because of compiler bugs in the c++
# code generation.
#
# Usage: make -f toolchain.mak
#
# Note that, unlike other toolchain build scripts, the required source 
# packages (linux, uclibc, etc) are not downloaded automatically, so
# the following package tarballs must be made available under the
# 'tar-dir' directory (see below):
#
#  * uClibc-0.9.27.tar.bz2
#  * uClibc++-0.2.2.tar.bz2
#  * linux-2.4.20.tar.gz
#  * binutils_2.18.orig.tar.gz
#  * gcc-3.4.6.tar.bz2
#  * gcc-core-3.4.6.tar.bz2
#  * gcc-g++-3.4.6.tar.bz2
#
# Uses perl with the MIME::Base64 package to prepare patch files.
#
# The final cross-compiler ends up in "gcc/2/bin", with a gcc wrapper 
# script for using uclibc++ in "uclibc/usr/uClibc++/bin".
#
# In addition to the cross-compiler there are various bits and bobs
# which are built to run on the target machine:
#  * uclibc utilities in "uclibc/*/utils"
#  * binutils under "binutils/build-for-target"
#  * simple hello-world test programs in the cwd
#
# This makefile works by defining the following top-level build tasks:
#  1. binutils
#  2. linux headers -- simple untar and make config
#  3. gcc, pass 1 -- C only, no run-time library
#  4. uclibc -- built using gcc-1
#  5. gcc, pass 2 -- includes C++ language, refers to uclibc, builds stdlibc++-v3
#  6. uclibc++  -- built using gcc-2
#  7. test programs
#
# Top-level directories roughly correspond to the top-level build tasks:
#  * gcc
#    + gcc/gcc-x.x.x -- gcc source tree
#    + gcc/build-1 -- gcc-1 build tree
#    + gcc/1 -- gcc-1 install root
#    + gcc/build-2 -- gcc-2 build tree
#    + gcc/2 -- gcc-2 install root
#  * uclibc -- uclibc/uclibc++ install root
#    + uclibc/uClibc-x.x.x -- uclibc source and build tree
#    + uclibc/uClibc++-x.x.x -- uclibc++ source and build tree
#  * linux
#    + linux/linux-x.x.x -- linux source tree
#  * binutils -- binutils install root
#    + binutils/binutils-x.x -- binutils source tree 
#    + binutils/build -- binutils build tree
#
# When a top-level build task completes a "done" file is created in an
# appropriate sub-directory. This "done" file is used for makefile 
# dependencies between the top-level tasks, so "touch"-ing a "done"
# file can be used to trigger rebuilding of selected top-level
# build tasks. 
#
# A key feature for this build system is that whole directory trees 
# can be deleted in order to get them and their dependents to rebuild:
# if source trees are deleted they will get restored from the tar file, 
# if build trees are deleted they will get rebuilt from source, if 
# install trees are deleted they will get reinstalled, etc.
#
# Deleting directory trees is the preferred way to trigger rebuilds,
# but there are also a set of pseudo-targets defined to help with 
# debugging this makefile.
#
# Note that "make clean" does not clean the binutils build. This
# is because the binutils build is only dependent on native tools 
# so it tends to be very stable in comparison to the rest of the 
# toolchain. A "make vclean" can be used to clean up everything, 
# leaving (roughly speaking) only this makefile.
#
# Bzipped source tar files for the various packages must be made 
# available in the directories configured below...
#
#####
# configure these...
tar_dir = /usr/share/data/packages
binutils_tar_dir = $(tar_dir)/development/binutils
gcc_tar_dir = $(tar_dir)/development/gcc
uclibc_tar_dir = $(tar_dir)/development/uclibc
linux_tar_dir = $(tar_dir)/linux
#####

# define TEE=|tee for more verbosity, but note that the pipe messes up the exit codes
TEE:=>
TEEE:=2>&1

# gnu sed, or a wrapper that supports --in-place
SED=sed

mk_root = $(shell pwd)
gcc_configure_1 = --program-suffix=-mips --with-gnu-as --with-gnu-ld --with-abi=32
gcc_configure_2 = --target=mipsel-elf-linux-gnu
gcc_configure_3 = --with-as=$(mk_root)/binutils/mipsel-elf-linux-gnu/bin/as 
gcc_configure_4 = --with-ld=$(mk_root)/binutils/mipsel-elf-linux-gnu/bin/ld 
gcc_configure = $(gcc_configure_1) $(gcc_configure_2) $(gcc_configure_3) $(gcc_configure_4) 
gcc_1_configure = $(gcc_configure) --disable-threads --enable-languages=c --without-headers --with-newlib
gcc_2_configure = $(gcc_configure) --enable-languages=c,c++ --enable-sjlj-exceptions --enable-threads=posix --with-sysroot=$(mk_root)/uclibc 

gcc_files = gcc/gcc-3.4.6/README
gcc_diff = gcc-3.4.6.diff
gcc_patch = gcc/gcc-3.4.6/.gcc_patch.done
gcc_1_config = gcc/build-1/Makefile
gcc_1_make = gcc/build-1/.gcc_1_make.done
gcc_1_install = gcc/1/.gcc_1_install.done
gcc_2_config = gcc/build-2/Makefile
gcc_2_make = gcc/build-2/.gcc_2_make.done
gcc_2_install = gcc/2/.gcc_2_install.done

binutils_files = binutils/binutils-2.18/README
binutils_config = binutils/build/Makefile
binutils_make = binutils/build/.binutils_make.done
binutils_install = binutils/.binutils_install.done
binutils_for_target_config = binutils/build-for-target/Makefile
binutils_for_target_make = binutils/build-for-target/.binutils_for_target_make.done

linux_files = linux/linux-2.4.20/README
linux_config = linux/linux-2.4.20/.config

uclibc_files = uclibc/uClibc-0.9.27/README
uclibc_config = uclibc/uClibc-0.9.27/.config
uclibc_patch = uclibc/uClibc-0.9.27/.uclibc_patch.done
uclibc_make = uclibc/uClibc-0.9.27/.uclibc_make.done
uclibc_install = uclibc/.uclibc_install.done
uclibc_for_target = uclibc/uClibc-0.9.27/utils/.uclibc_for_target.done

uclibcpp_files = uclibc/uClibc++-0.2.2/README
uclibcpp_patch = uclibc/uClibc++-0.2.2/.uclibcpp_patch.done
uclibcpp_config = uclibc/uClibc++-0.2.2/.config
uclibcpp_make = uclibc/uClibc++-0.2.2/.uclibcpp_make.done
uclibcpp_install = uclibc/.uclibcpp_install.done

test_c_for_target = test-c 
test_cpp_for_target = test-c++
test_cpp_for_target_static = test-c++-s
test_cpp_for_target_uclibcpp = test-c++-u
tests = $(test_c_for_target) $(test_cpp_for_target) $(test_cpp_for_target_static) $(test_cpp_for_target_uclibcpp)

.PHONY: all

all: $(uclibc_for_target) $(binutils_for_target_make) $(tests) $(uclibcpp_install) configure-mips.sh

# ==

$(linux_files):
	@echo
	@echo ++ untaring linux
	@mkdir linux 2>/dev/null || true
	tar -C linux -xzf $(linux_tar_dir)/linux-2.4.20.tar.gz
	@rm -f $(linux_config) 2>/dev/null || true
	@touch $(linux_files)

$(linux_config): $(linux_files)
	@echo
	@echo ++ configuring linux
	@cd linux/linux-2.4.20 && make oldconfig ARCH=mips $(TEE) ../../linux_config.out $(TEEE)

# ==

$(binutils_files):
	@echo
	@echo ++ untaring binutils
	@mkdir binutils 2>/dev/null || true
	tar -C binutils -xzf $(binutils_tar_dir)/binutils_2.18.orig.tar.gz
	-zcat $(binutils_tar_dir)/binutils_2.18-0ubuntu3.diff.gz | ( cd binutils/binutils-2.18 && patch -p1 -s )
	@rm -f $(binutils_config) 2>/dev/null || true
	@touch $(binutils_files)

$(binutils_config): $(binutils_files)
	@echo
	@echo ++ configuring binutils
	@if test -d binutils/build ; then : ; else mkdir binutils/build ; fi
	@cd binutils/build && ../binutils-2.18/configure --prefix=`dirname \`pwd\`` --target=mipsel-elf-linux-gnu $(TEE) ../../binutils_config.out $(TEEE)

$(binutils_make): $(binutils_config)
	@echo
	@echo ++ building binutils
	@cd binutils/build && make $(TEE) ../../binutils_make.out $(TEEE)
	@touch $(binutils_make)

$(binutils_install): $(binutils_make)
	@echo
	@echo ++ installing binutils
	@cd binutils/build && make install $(TEE) ../../binutils_install.out $(TEEE)
	@touch $(binutils_install)

$(binutils_for_target_make): $(binutils_make) $(gcc_2_install) $(binutils_for_target_config)
	@echo
	@echo ++ building binutils for target
	@cd binutils/build-for-target && PATH="`dirname \`pwd\``/bin:$$PATH" CC=`dirname \`pwd\``/../gcc/2/bin/gcc-mips make LDFLAGS="-Xlinker --dynamic-linker=/lib/ld-uClibc.so.0" $(TEE) ../../binutils_for_target_make.out $(TEEE)
	@touch $(binutils_for_target_make)

$(binutils_for_target_config):
	@echo
	@echo ++ configuring binutils for target
	@if test -d binutils/build-for-target ; then : ; else mkdir binutils/build-for-target ; fi
	@cd binutils/build-for-target && PATH="`dirname \`pwd\``/bin:$$PATH" CC=`dirname \`pwd\``/../gcc/2/bin/gcc-mips ../binutils-2.18/configure --with-build-time-tools=`dirname \`pwd\``/bin --with-build-sysroot=`dirname \`pwd\``/../uclibc --target=mipsel-elf-linux-gnu --host=mipsel-elf-linux-gnu $(TEE) ../../binutils_for_target_config.out $(TEEE)
	@$(SED) -e 's/^CFLAGS_FOR_BUILD *=.*/CFLAGS_FOR_BUILD = /' --in-place binutils/build-for-target/Makefile

# ==

$(uclibc_files):
	@echo
	@echo ++ untaring uclibc
	@mkdir uclibc 2>/dev/null || true
	tar -C uclibc -xjf $(uclibc_tar_dir)/uClibc-0.9.27.tar.bz2
	@touch $(uclibc_files)

$(uclibc_config): $(uclibc_files) $(linux_config) $(gcc_1_install)
	@echo
	@echo ++ configuring uclibc
	@$(SED) -e 's/\( *\)default TARGET_i386/\1default TARGET_mips/' --in-place=.orig uclibc/uClibc-0.9.27/extra/Configs/Config.in
	@cd uclibc/uClibc-0.9.27 && make defconfig $(TEE) ../../uclibc_config.out $(TEEE)
	@$(SED) -e 's:^KERNEL_SOURCE=.*:KERNEL_SOURCE="'"`pwd`/linux/linux-2.4.20"'":' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:^SHARED_LIB_LOADER_PREFIX=.*:SHARED_LIB_LOADER_PREFIX="'"/lib"'":' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:^RUNTIME_PREFIX=.*:RUNTIME_PREFIX="'"`pwd`/uclibc"'":' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:^DEVEL_PREFIX=.*:DEVEL_PREFIX="'"`pwd`/uclibc/usr"'":' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:^LDSO_BASE_FILENAME=.*:LDSO_BASE_FILENAME="'"ld-uClibc.so"'":' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*CONFIG_MIPS_ISA_MIPS32.*:CONFIG_MIPS_ISA_MIPS32=y:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:^CONFIG_MIPS_ISA_1.*:# CONFIG_MIPS_ISA_1 is not set:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_WCHAR.*:UCLIBC_HAS_WCHAR=y:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_LOCALE.*:# UCLIBC_HAS_LOCALE is not set:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_TM_EXTENSIONS.*:# UCLIBC_HAS_TM_EXTENSIONS is not set:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_RPC.*:UCLIBC_HAS_RPC=y\nUCLIBC_HAS_FULL_RPC=y:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_FULL_RPC.*:UCLIBC_HAS_FULL_RPC=y:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_CTYPE_TABLES.*:UCLIBC_HAS_CTYPE_TABLES=y:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_CTYPE_SIGNED.*:# UCLIBC_HAS_CTYPE_SIGNED is not set:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_CTYPE_UNSAFE.*:# UCLIBC_HAS_CTYPE_UNSAFE is not set:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_GLIBC_CUSTOM_PRINTF.*:UCLIBC_HAS_GLIBC_CUSTOM_PRINTF=y:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_HEXADECIMAL_FLOATS.*:# UCLIBC_HAS_HEXADECIMAL_FLOATS is not set:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_GLIBC_CUSTOM_STREAMS.*:UCLIBC_HAS_GLIBC_CUSTOM_STREAMS=y:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UNIX98PTY_ONLY.*:# UNIX98PTY_ONLY is not set:' --in-place uclibc/uClibc-0.9.27/.config
	@$(SED) -e 's:.*UCLIBC_HAS_FTW.*:UCLIBC_HAS_FTW=y:' --in-place uclibc/uClibc-0.9.27/.config
	@rm uclibc/uClibc-0.9.27/include/bits/uClibc_config.h 2>/dev/null || true

$(uclibc_patch): $(uclibc_files)
	@echo
	@echo ++ patching uclibc
	@: # no patching required -- it used to add -EL to Rules.mak
	@touch $(uclibc_patch)

$(uclibc_make): $(uclibc_config) $(uclibc_patch)
	@echo
	@echo ++ building uclibc
	@cd uclibc/uClibc-0.9.27 && echo PATH=\"`pwd`/../../binutils/bin:$$PATH\" make CROSS=mipsel-elf-linux-gnu- CC=`pwd`/../../gcc/1/bin/gcc-mips \"$$\@\" > make.sh # for convenience
	@cd uclibc/uClibc-0.9.27 && PATH="`pwd`/../../binutils/bin:$$PATH" make CROSS=mipsel-elf-linux-gnu- CC=`pwd`/../../gcc/1/bin/gcc-mips $(TEE) ../../uclibc_make.out $(TEEE)
	@touch $(uclibc_make)

$(uclibc_install): $(uclibc_make)
	@echo
	@echo ++ installing uclibc
	@cd uclibc/uClibc-0.9.27 && PATH="`pwd`/../../binutils/bin:$$PATH" make CROSS=mipsel-elf-linux-gnu- CC=`pwd`/../../gcc/1/bin/gcc-mips install $(TEE) ../../uclibc_install.out $(TEEE)
	@touch $(uclibc_install)

$(uclibc_for_target): $(gcc_2_install) $(uclibc_make)
	@echo
	@echo ++ building uclibc utils
	@cd uclibc/uClibc-0.9.27/utils && PATH="`pwd`/../../../binutils/bin:$$PATH" make CROSS=mipsel-elf-linux-gnu- CC=`pwd`/../../../gcc/2/bin/gcc-mips clean > /dev/null
	@cd uclibc/uClibc-0.9.27/utils && PATH="`pwd`/../../../binutils/bin:$$PATH" make CROSS=mipsel-elf-linux-gnu- CC=`pwd`/../../../gcc/2/bin/gcc-mips CFLAGS="-static" RUNTIME_PREFIX=/ $(TEE) ../../../uclibc_for_target_make.out $(TEEE)
	@touch $(uclibc_for_target)

# ==

$(uclibcpp_files):
	@echo
	@echo ++ untaring uclibc++
	@mkdir uclibc 2>/dev/null || true
	tar -C uclibc -xjf $(uclibc_tar_dir)/uClibc++-0.2.2.tar.bz2
	@touch $(uclibcpp_files)

$(uclibcpp_patch): $(uclibcpp_files)
	@echo
	@echo ++ patching uclibc++
	@: no-op
	@touch $(uclibcpp_patch)

$(uclibcpp_config): $(uclibcpp_patch) $(uclibc_config)
	@echo
	@echo ++ configuring uclibc++
	@cd uclibc/uClibc++-0.2.2 && make defconfig $(TEE) ../../uclibcpp_config.out $(TEEE)
	@$(SED) 's:.*UCLIBCXX_HAS_LONG_DOUBLE.*:# UCLIBCXX_HAS_LONG_DOUBLE is not set:' --in-place uclibc/uClibc++-0.2.2/.config
	@$(SED) 's:.*UCLIBCXX_HAS_TLS.*:# UCLIBCXX_HAS_TLS is not set:' --in-place uclibc/uClibc++-0.2.2/.config
	@$(SED) 's:.*UCLIBCXX_HAS_LFS.*:# UCLIBCXX_HAS_LFS is not set:' --in-place uclibc/uClibc++-0.2.2/.config
	@rm -f uclibc/uClibc++-0.2.2/include/system_configuration.h

$(uclibcpp_make): $(uclibcpp_config) $(gcc_2_install)
	@echo
	@echo ++ building uclibc++
	@cd uclibc/uClibc++-0.2.2 && echo PATH="$(mk_root)/binutils/bin:$$PATH" make CROSS=mipsel-elf-linux-gnu- CXX=$(mk_root)/gcc/2/bin/g++-mips CC=$(mk_root)/gcc/2/bin/gcc-mips LIBS=\"-lc -Bstatic -ldl_pic\" \"$$\@\" > make.sh
	@cd uclibc/uClibc++-0.2.2 && PATH="$(mk_root)/binutils/bin:$$PATH" make CROSS=mipsel-elf-linux-gnu- CXX=$(mk_root)/gcc/2/bin/g++-mips CC=$(mk_root)/gcc/2/bin/gcc-mips LIBS="-lc -Bstatic -ldl_pic" $(TEE) ../../uclibcpp_make.out $(TEEE)
	@touch $(uclibcpp_make)

$(uclibcpp_install): $(uclibcpp_make) $(uclibc_config)
	@echo
	@echo ++ installing uclibc++
	@cd uclibc/uClibc++-0.2.2 && PATH="`pwd`/../../binutils/bin:$$PATH" make CROSS=mipsel-elf-linux-gnu- CC=`pwd`/../../gcc/2/bin/gcc-mips DESTDIR=$(mk_root)/uclibc install $(TEE) ../../uclibcpp_install.out $(TEEE)
	@touch $(uclibcpp_install)

# ==

$(gcc_files):
	@echo
	@echo ++ untaring gcc
	@mkdir gcc 2>/dev/null || true
	tar -C gcc -xjf $(gcc_tar_dir)/gcc-3.4.6.tar.bz2
	tar -C gcc -xjf $(gcc_tar_dir)/gcc-core-3.4.6.tar.bz2
	tar -C gcc -xjf $(gcc_tar_dir)/gcc-g++-3.4.6.tar.bz2
	@touch $(gcc_files)

$(gcc_patch): $(gcc_files) $(gcc_diff)
	@echo
	@echo ++ patching gcc
	@f=`echo gcc-3.4.6|tr -d .`.diff ; if test -f $$f ; then cat $$f | ( cd gcc/gcc-3.4.6 && patch -p1 -N -s ) ; fi
	@touch $(gcc_patch)

$(gcc_1_config): $(gcc_patch) $(binutils_install)
	@echo
	@echo ++ configuring gcc 1
	@if test -d gcc/build-1 ; then : ; else mkdir gcc/build-1 ; fi
	@echo configure --prefix=$(mk_root)/gcc/1 $(gcc_1_configure)
	@cd gcc/build-1 && ../gcc-3.4.6/configure --prefix=$(mk_root)/gcc/1 $(gcc_1_configure) $(TEE) ../../gcc_1_config.out $(TEEE)

$(gcc_1_make): $(gcc_1_config)
	@echo
	@echo ++ building gcc 1
	@for f in binutils/mips*/bin/* ; do ( cd gcc/build-1 && ln -fs ../../$$f `basename $$f`-mips ) ; done
	@mkdir -p gcc/build-1/gcc
	@touch gcc/build-1/gcc/crti.o
	@touch gcc/build-1/gcc/crtn.o
	@touch gcc/build-1/gcc/libc.a
	@cd gcc/build-1 && PATH="`pwd`:$$PATH" make $(TEE) ../../gcc_1_make.out $(TEEE)
	@touch $(gcc_1_make)

$(gcc_1_install): $(gcc_1_make)
	@echo
	@echo ++ installing gcc 1
	@cd gcc/build-1 && PATH="`pwd`:$$PATH" make install $(TEE) ../../gcc_1_install.out $(TEEE)
	@if test -f gcc/1/bin/mipsel-elf-linux-gnu-gcc -a ! -h gcc/1/bin/gcc-mips ; then ( cd gcc/1/bin && ln -s mipsel-elf-linux-gnu-gcc gcc-mips ) ; fi # for gcc-3.3.x
	@touch $(gcc_1_install)

$(gcc_2_config): $(gcc_1_install) $(uclibc_install)
	@echo
	@echo ++ configuring gcc 2
	@if test -d gcc/build-2 ; then : ; else mkdir gcc/build-2 ; fi
	@echo configure --prefix=$(mk_root)/gcc/2 $(gcc_2_configure)
	@cd gcc/build-2 && CXXFLAGS=-g ../gcc-3.4.6/configure --prefix=$(mk_root)/gcc/2 $(gcc_2_configure) $(TEE) ../../gcc_2_config.out $(TEEE)

$(gcc_2_make): $(gcc_2_config)
	@echo
	@echo ++ building gcc 2
	@for f in binutils/mips*/bin/* ; do ( cd gcc/build-2 && ln -fs ../../$$f `basename $$f`-mips ) ; done
	@cd gcc/build-2 && PATH="`pwd`:$$PATH" make $(TEE) ../../gcc_2_make.out $(TEEE)
	@touch $(gcc_2_make)

$(gcc_2_install): $(gcc_2_make)
	@echo
	@echo ++ installing gcc 2
	@cd gcc/build-2 && PATH="`pwd`:$$PATH" make install $(TEE) ../../gcc_2_install.out $(TEEE)
	@if test -f gcc/2/bin/mipsel-elf-linux-gnu-gcc -a ! -h gcc/2/bin/gcc-mips ; then ( cd gcc/2/bin && ln -s mipsel-elf-linux-gnu-gcc gcc-mips ) ; fi # for gcc-3.3.x
	@if test -f gcc/2/bin/mipsel-elf-linux-gnu-g++ -a ! -h gcc/2/bin/g++-mips ; then ( cd gcc/2/bin && ln -s mipsel-elf-linux-gnu-g++ g++-mips ) ; fi # for gcc-3.3.x
	@touch $(gcc_2_install)

$(gcc_diff): gcc-334.diff gcc-343.diff gcc-336.diff gcc-346.diff
	@cp `echo gcc-3.4.6 | tr -d .`.diff $(gcc_diff)

gcc-334.diff.tmp:
	@:
	@: # magic patch to mips/linux.h
	@:
	@echo ZGlmZiAtTmF1ciBvbGQvZ2NjL2NvbmZpZy9taXBzL2xpbnV4LmggbmV3L2djYy9jb25maWcvbWlw > $@
	@echo cy9saW51eC5oCg== >> $@
	@echo LS0tIG9sZC9nY2MvY29uZmlnL21pcHMvbGludXguaAkyMDA3LTExLTI5IDE0OjUyOjI1LjAwMDAw >> $@
	@echo MDAwMCArMDAwMAo= >> $@
	@echo KysrIG5ldy9nY2MvY29uZmlnL21pcHMvbGludXguaAkyMDA3LTExLTI5IDE0OjU0OjI5LjAwMDAw >> $@
	@echo MDAwMCArMDAwMAo= >> $@
	@echo QEAgLTIzOCwxMCArMjM4LDggQEAK >> $@
	@echo ICAgIHBzZXVkby1vcHMuICAqLwo= >> $@
	@echo ICNkZWZpbmUgRlVOQ1RJT05fTkFNRV9BTFJFQURZX0RFQ0xBUkVECg== >> $@
	@echo IAo= >> $@
	@echo LSNkZWZpbmUgQVNNX1BSRUZFUlJFRF9FSF9EQVRBX0ZPUk1BVChDT0RFLCBHTE9CQUwpICAgICAg >> $@
	@echo IAkJXAo= >> $@
	@echo LSAgKGZsYWdfcGljCQkJCQkJCQlcCg== >> $@
	@echo LSAgICA/ICgoR0xPQkFMKSA/IERXX0VIX1BFX2luZGlyZWN0IDogMCkgfCBEV19FSF9QRV9wY3Jl >> $@
	@echo bCB8IERXX0VIX1BFX3NkYXRhNFwK >> $@
	@echo LSAgIDogRFdfRUhfUEVfYWJzcHRyKQo= >> $@
	@echo Ky8qIGdodyBHSFcgaHR0cDovL2djYy5nbnUub3JnL21sL2djYy1wYXRjaGVzLzIwMDQtMDYvbXNn >> $@
	@echo MDA5NzAuaHRtbCAqLwo= >> $@
	@echo KyNkZWZpbmUgQVNNX1BSRUZFUlJFRF9FSF9EQVRBX0ZPUk1BVChDT0RFLCBHTE9CQUwpIERXX0VI >> $@
	@echo X1BFX2Fic3B0cgo= >> $@
	@echo IAo= >> $@
	@echo IC8qIFRoZSBnbGliYyBfbWNvdW50IHN0dWIgd2lsbCBzYXZlICR2MCBmb3IgdXMuICBEb24ndCBt >> $@
	@echo ZXNzIHdpdGggc2F2aW5nCg== >> $@
	@echo ICAgIGl0LCBzaW5jZSBBU01fT1VUUFVUX1JFR19QVVNIL0FTTV9PVVRQVVRfUkVHX1BPUCBkbyBu >> $@
	@echo b3Qgd29yayBpbiB0aGUK >> $@
	@:
	@: # ctype map data-type patch for c++
	@:
	@echo ZGlmZiAtTmF1ciBvbGQvbGlic3RkYysrLXYzL2NvbmZpZy9vcy9nbnUtbGludXgvY3R5cGVfYmFz >> $@
	@echo ZS5oIG5ldy9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgK >> $@
	@echo LS0tIG9sZC9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgJMjAw >> $@
	@echo Ny0xMi0wNyAxMzo1OTo1Mi4wMDAwMDAwMDAgKzAwMDAK >> $@
	@echo KysrIG5ldy9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgJMjAw >> $@
	@echo Ny0xMi0wNyAxNDowMDoyNC4wMDAwMDAwMDAgKzAwMDAK >> $@
	@echo QEAgLTM2LDcgKzM2LDcgQEAK >> $@
	@echo ICAgc3RydWN0IGN0eXBlX2Jhc2UK >> $@
	@echo ICAgewo= >> $@
	@echo ICAgICAvLyBOb24tc3RhbmRhcmQgdHlwZWRlZnMuCg== >> $@
	@echo LSAgICB0eXBlZGVmIGNvbnN0IGludCogCQlfX3RvX3R5cGU7Cg== >> $@
	@echo KyAgICB0eXBlZGVmIGNvbnN0IF9fY3R5cGVfdG91cGxvd190KiAJCV9fdG9fdHlwZTsK >> $@
	@echo IAo= >> $@
	@echo ICAgICAvLyBOQjogT2Zmc2V0cyBpbnRvIGN0eXBlPGNoYXI+OjpfTV90YWJsZSBmb3JjZSBhIHBh >> $@
	@echo cnRpY3VsYXIgc2l6ZQo= >> $@
	@echo ICAgICAvLyBvbiB0aGUgbWFzayB0eXBlLiBCZWNhdXNlIG9mIHRoaXMsIHdlIGRvbid0IHVzZSBh >> $@
	@echo biBlbnVtLgo= >> $@

gcc-334.diff: gcc-334.diff.tmp
	@perl -e 'use MIME::Base64;while(<>){print MIME::Base64::decode_base64($$_)}'< $< >.tmp && mv .tmp $@

gcc-343.diff: gcc-343.diff.tmp
	@perl -e 'use MIME::Base64;while(<>){print MIME::Base64::decode_base64($$_)}'< $< >.tmp && mv .tmp $@

gcc-343.diff.tmp:
	@:
	@: # ctype map data-type patch for c++
	@:
	@echo ZGlmZiAtTmF1ciBvbGQvbGlic3RkYysrLXYzL2NvbmZpZy9vcy9nbnUtbGludXgvY3R5cGVfYmFz > $@
	@echo ZS5oIG5ldy9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgK >> $@
	@echo LS0tIG9sZC9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgJMjAw >> $@
	@echo Ny0xMi0wMyAxNjoyNzowMy4wMDAwMDAwMDAgKzAwMDAK >> $@
	@echo KysrIG5ldy9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgJMjAw >> $@
	@echo Ny0xMi0wMyAxNjoyODowMS4wMDAwMDAwMDAgKzAwMDAK >> $@
	@echo QEAgLTM3LDcgKzM3LDcgQEAK >> $@
	@echo ICAgc3RydWN0IGN0eXBlX2Jhc2UK >> $@
	@echo ICAgewo= >> $@
	@echo ICAgICAvLyBOb24tc3RhbmRhcmQgdHlwZWRlZnMuCg== >> $@
	@echo LSAgICB0eXBlZGVmIGNvbnN0IGludCogCQlfX3RvX3R5cGU7Cg== >> $@
	@echo KyAgICB0eXBlZGVmIGNvbnN0IF9fY3R5cGVfdG91cGxvd190KiAJCV9fdG9fdHlwZTsK >> $@
	@echo IAo= >> $@
	@echo ICAgICAvLyBOQjogT2Zmc2V0cyBpbnRvIGN0eXBlPGNoYXI+OjpfTV90YWJsZSBmb3JjZSBhIHBh >> $@
	@echo cnRpY3VsYXIgc2l6ZQo= >> $@
	@echo ICAgICAvLyBvbiB0aGUgbWFzayB0eXBlLiBCZWNhdXNlIG9mIHRoaXMsIHdlIGRvbid0IHVzZSBh >> $@
	@echo biBlbnVtLgo= >> $@

gcc-336.diff: gcc-334.diff
	@cp $< $@

gcc-346.diff: gcc-346.diff.tmp
	@perl -e 'use MIME::Base64;while(<>){print MIME::Base64::decode_base64($$_)}'< $< >.tmp && mv .tmp $@

gcc-346.diff.tmp:
	@:
	@: # ctype map data-type patch for c++
	@:
	@echo ZGlmZiAtTmF1ciBvbGQvbGlic3RkYysrLXYzL2NvbmZpZy9vcy9nbnUtbGludXgvY3R5cGVfYmFz > $@
	@echo ZS5oIG5ldy9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgK >> $@
	@echo LS0tIG9sZC9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgJMjAw >> $@
	@echo Ny0xMi0wMyAxNjoyNzowMy4wMDAwMDAwMDAgKzAwMDAK >> $@
	@echo KysrIG5ldy9saWJzdGRjKystdjMvY29uZmlnL29zL2dudS1saW51eC9jdHlwZV9iYXNlLmgJMjAw >> $@
	@echo Ny0xMi0wMyAxNjoyODowMS4wMDAwMDAwMDAgKzAwMDAK >> $@
	@echo QEAgLTM3LDcgKzM3LDcgQEAK >> $@
	@echo ICAgc3RydWN0IGN0eXBlX2Jhc2UK >> $@
	@echo ICAgewo= >> $@
	@echo ICAgICAvLyBOb24tc3RhbmRhcmQgdHlwZWRlZnMuCg== >> $@
	@echo LSAgICB0eXBlZGVmIGNvbnN0IGludCogCQlfX3RvX3R5cGU7Cg== >> $@
	@echo KyAgICB0eXBlZGVmIGNvbnN0IF9fY3R5cGVfdG91cGxvd190KiAJCV9fdG9fdHlwZTsK >> $@
	@echo IAo= >> $@
	@echo ICAgICAvLyBOQjogT2Zmc2V0cyBpbnRvIGN0eXBlPGNoYXI+OjpfTV90YWJsZSBmb3JjZSBhIHBh >> $@
	@echo cnRpY3VsYXIgc2l6ZQo= >> $@
	@echo ICAgICAvLyBvbiB0aGUgbWFzayB0eXBlLiBCZWNhdXNlIG9mIHRoaXMsIHdlIGRvbid0IHVzZSBh >> $@
	@echo biBlbnVtLgo= >> $@
	@:
	@: # hack in an implementation of finitef
	@:
	@echo ZGlmZiAtTmF1ciBvbGQvbGlic3RkYysrLXYzL3NyYy9jdHlwZS5jYyBuZXcvbGlic3RkYysrLXYz >> $@
	@echo L3NyYy9jdHlwZS5jYwo= >> $@
	@echo LS0tIG9sZC9saWJzdGRjKystdjMvc3JjL2N0eXBlLmNjCTIwMDctMTItMTAgMTM6NTA6NDAuMDAw >> $@
	@echo MDAwMDAwICswMDAwCg== >> $@
	@echo KysrIG5ldy9saWJzdGRjKystdjMvc3JjL2N0eXBlLmNjCTIwMDctMTItMTAgMTM6NDg6MjUuMDAw >> $@
	@echo MDAwMDAwICswMDAwCg== >> $@
	@echo QEAgLTExMiwzICsxMTIsNCBAQAo= >> $@
	@echo ICNlbmRpZgo= >> $@
	@echo IH0gLy8gbmFtZXNwYWNlIHN0ZAo= >> $@
	@echo IAo= >> $@
	@echo K2V4dGVybiAiQyIgaW50IGZpbml0ZWYoZmxvYXQgeCkgeyB1bmlvbiB7dV9pbnQzMl90IGx2YWw7 >> $@
	@echo ZmxvYXQgZnZhbDt9IHo7ei5mdmFsPXg7cmV0dXJuICgoei5sdmFsJjB4N2Y4MDAwMDApIT0weDdm >> $@
	@echo ODAwMDAwKTt9IC8qIGdodyBHSFcgKi8K >> $@

# ==

test.c:
	@echo '#include <stdio.h>' > $@
	@echo 'int main() { printf("Hello, world!\\n") ; return 0 ; }' >> $@

test.cpp:
	@echo '#include <iostream>' > $@
	@echo 'int main() { std::cout << "Hello, world!" << std::endl ; return 0 ; }' >> $@

$(test_c_for_target): test.c $(gcc_2_install)
	@echo
	@echo ++ testing c
	gcc/2/bin/gcc-mips -Xlinker --dynamic-linker=/lib/ld-uClibc.so.0 -o $@ test.c -lm

$(test_cpp_for_target): test.cpp $(gcc_2_install)
	@echo
	@echo ++ testing c++
	gcc/2/bin/g++-mips -Xlinker --dynamic-linker=/lib/ld-uClibc.so.0 -o $@ test.cpp -lgcc_s -ldl_pic

$(test_cpp_for_target_static): test.cpp $(gcc_2_install)
	@echo
	@echo ++ testing c++ all-static
	gcc/build-2/mipsel-elf-linux-gnu/libstdc++-v3/libtool --quiet --tag=CXX --mode=link gcc/2/bin/g++-mips -all-static -Xlinker --dynamic-linker=/lib/ld-uClibc.so.0 -o $@ test.cpp -lgcc_eh -ldl

$(test_cpp_for_target_uclibcpp): test.cpp $(uclibcpp_install)
	@echo
	@echo ++ testing uclibc++
	uclibc/usr/uClibc++/bin/g++-uc -Xlinker --dynamic-linker=/lib/ld-uClibc.so.0 -Iuclibc/usr/uClibc++/include -Luclibc/usr/uClibc++/lib -o $@ test.cpp

configure-mips.sh:
	@test -f configure-mips.sh_ && cp configure-mips.sh_ $@ && chmod +x $@ || true

# ==

.PHONY: vclean

vclean: clean clean_binutils
	@rm -f *.out
	@rm -f *.tmp

.PHONY: clean_binutils

clean_binutils:
	@rm $(binutils_make) 2>/dev/null || true
	@rm $(binutils_install) 2>/dev/null || true
	@rm $(binutils_for_target_make) 2>/dev/null || true
	@rm -rf binutils/binutils-2.18
	@rm -rf binutils/build
	@rm -rf binutils/bin binutils/info binutils/lib binutils/man binutils/mips*-elf-linux-gnu binutils/share
	@rm -rf binutils/build-for-target
	@if test -d binutils ; then rmdir binutils ; fi

.PHONY: clean

clean:
	@rm $(gcc_1_make) 2>/dev/null || true
	@rm $(gcc_1_install) 2>/dev/null || true
	@rm $(gcc_2_make) 2>/dev/null || true
	@rm $(gcc_2_install) 2>/dev/null || true
	@rm $(uclibc_patch) 2>/dev/null || true
	@rm $(uclibc_make) 2>/dev/null || true
	@rm $(uclibc_install) 2>/dev/null || true
	@rm $(uclibcpp_install) 2>/dev/null || true
	@rm $(uclibc_for_target) 2>/dev/null || true
	@rm -rf linux/linux-2.4.20
	@rm -rf uclibc/uClibc-0.9.27
	@rm -rf uclibc/uClibc++-0.2.2
	@rm -rf uclibc/lib uclibc/usr uclibc/sbin
	@rm -rf gcc/gcc-3.*
	@rm -rf gcc/build-1
	@rm -rf gcc/build-2
	@rm -rf gcc/1
	@rm -rf gcc/2
	@rm -f gcc*.diff
	@rm -f *diff.tmp
	@if test -d linux ; then rmdir linux ; fi
	@if test -d uclibc ; then rmdir uclibc ; fi
	@if test -d gcc ; then rmdir gcc ; fi

.PHONY: done

done:
	touch $(linux_files) || true
	touch $(linux_config) || true
	touch $(gcc_files) || true
	touch $(gcc_diff) || true
	touch $(gcc_patch) || true
	touch $(gcc_1_config) || true
	touch $(gcc_1_make) || true
	touch $(gcc_1_install) || true
	touch $(binutils_files) || true
	touch $(binutils_config) || true
	touch $(binutils_make) || true
	touch $(binutils_install) || true
	touch $(uclibc_files) || true
	touch $(uclibc_config) || true
	touch $(uclibc_patch) || true
	touch $(uclibc_make) || true
	touch $(uclibc_install) || true
	touch $(gcc_2_config) || true
	touch $(gcc_2_make) || true
	touch $(gcc_2_install) || true
	touch $(binutils_for_target_config) || true
	touch $(binutils_for_target_make) || true
	touch $(uclibc_for_target) || true

.PHONY: gcc_files
.PHONY: gcc_diff
.PHONY: gcc_patch
.PHONY: gcc_1_config
.PHONY: gcc_1_make
.PHONY: gcc_1_install
.PHONY: gcc_2_config
.PHONY: gcc_2_make
.PHONY: gcc_2_install
.PHONY: binutils_files
.PHONY: binutils_config
.PHONY: binutils_make
.PHONY: binutils_install
.PHONY: binutils_for_target_config
.PHONY: binutils_for_target_make
.PHONY: linux_files
.PHONY: linux_config
.PHONY: uclibc_files
.PHONY: uclibc_config
.PHONY: uclibc_patch
.PHONY: uclibc_make
.PHONY: uclibc_install
.PHONY: uclibc_for_target
.PHONY: uclibcpp_files
.PHONY: uclibcpp_patch
.PHONY: uclibcpp_config
.PHONY: uclibcpp_make
.PHONY: uclibcpp_install

gcc_files: $(gcc_files)
gcc_diff: $(gcc_diff)
gcc_patch: $(gcc_patch)
gcc_1_config: $(gcc_1_config)
gcc_1_make: $(gcc_1_make)
gcc_1_install: $(gcc_1_install)
gcc_2_config: $(gcc_2_config)
gcc_2_make: $(gcc_2_make)
gcc_2_install: $(gcc_2_install)
binutils_files: $(binutils_files)
binutils_config: $(binutils_config)
binutils_make: $(binutils_make)
binutils_install: $(binutils_install)
binutils_for_target_config: $(binutils_for_target_config)
binutils_for_target_make: $(binutils_for_target_make)
linux_files: $(linux_files)
linux_config: $(linux_config)
uclibc_files: $(uclibc_files)
uclibc_config: $(uclibc_config)
uclibc_patch: $(uclibc_patch)
uclibc_make: $(uclibc_make)
uclibc_install: $(uclibc_install)
uclibc_for_target: $(uclibc_for_target)
uclibcpp_files: $(uclibcpp_files)
uclibcpp_patch: $(uclibcpp_patch)
uclibcpp_config: $(uclibcpp_config)
uclibcpp_make: $(uclibcpp_make)
uclibcpp_install: $(uclibcpp_install)

