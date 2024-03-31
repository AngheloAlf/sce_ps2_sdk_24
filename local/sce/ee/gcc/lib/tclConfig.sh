# tclConfig.sh --
# 
# This shell script (for sh) is generated automatically by Tcl's
# configure script.  It will create shell variables for most of
# the configuration options discovered by the configure script.
# This script is intended to be included by the configure scripts
# for Tcl extensions so that they don't have to figure this all
# out for themselves.
#
# The information in this file is specific to a single platform.
#
# RCS: @(#) $Id: tclConfig.sh.in,v 1.10 1999/02/11 02:16:13 spolk Exp $

# Tcl's version number.
TCL_VERSION='8.0'
TCL_MAJOR_VERSION='8'
TCL_MINOR_VERSION='0'
TCL_PATCH_LEVEL='.3'

# C compiler to use for compilation.
TCL_CC='/usr/cygnus/gnupro-99r1/H-i686-pc-linux-gnu/bin/gcc'

# -D flags for use with the C compiler.
TCL_DEFS=' -DHAVE_GETCWD=1 -DHAVE_OPENDIR=1 -DHAVE_STRSTR=1 -DHAVE_STRTOL=1 -DHAVE_TMPNAM=1 -DHAVE_WAITPID=1 -DHAVE_UNISTD_H=1 -DUSE_TERMIOS=1 -DHAVE_SYS_TIME_H=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_TM_ZONE=1 -DHAVE_TM_GMTOFF=1 -DHAVE_TIMEZONE_VAR=1 -DHAVE_TIMEZONE_DECL=1 -DHAVE_ST_BLKSIZE=1 -DSTDC_HEADERS=1 -DNEED_MATHERR=1 -DRETSIGTYPE=void -DHAVE_SIGNED_CHAR=1 -DHAVE_SYS_IOCTL_H=1 '

# If TCL was built with debugging symbols, generated libraries contain
# this string at the end of the library name (before the extension).
TCL_DBGX=@TCL_DBGX@

# Default flags used in an optimized and debuggable build, respectively.
TCL_CFLAGS_DEBUG='@CFLAGS_DEBUG@'
TCL_CFLAGS_OPTIMIZE='@CFLAGS_OPTIMIZE@'

# Flag, 1: we built a shared lib, 0 we didn't
TCL_SHARED_BUILD=0

# The name of the Tcl library (may be either a .a file or a shared library):
TCL_LIB_FILE='libtcl8.0.a'

# The fullpath of the Tcl library (used for dependency checking)
TCL_LIB_FULL_PATH='/usr/local/sce/ee/gcc/build/tcl/unix/libtcl8.0.a'

# Additional libraries to use when linking Tcl.
TCL_LIBS='-ldl  -lieee -lm'

# Top-level directory in which Tcl's platform-independent files are
# installed.
TCL_PREFIX='/usr/local/sce/ee/gcc'

# Top-level directory in which Tcl's platform-specific files (e.g.
# executables) are installed.
TCL_EXEC_PREFIX='/usr/local/sce/ee/gcc'

# Flags to pass to cc when compiling the components of a shared library:
TCL_SHLIB_CFLAGS='-fPIC'

# Extra flags to pass to cc:
TCL_EXTRA_CFLAGS='@EXTRA_CFLAGS@'

# Base command to use for combining object files into a shared library:
TCL_SHLIB_LD='/usr/cygnus/gnupro-99r1/H-i686-pc-linux-gnu/bin/gcc -shared'

# Either '$LIBS' (if dependent libraries should be included when linking
# shared libraries) or an empty string.  See Tcl's configure.in for more
# explanation.
TCL_SHLIB_LD_LIBS=''

# Suffix to use for the name of a shared library.
TCL_SHLIB_SUFFIX='.so'

# Library file(s) to include in tclsh and other base applications
# in order to provide facilities needed by DLOBJ above.
TCL_DL_LIBS='-ldl'

# Flags to pass to the compiler when linking object files into
# an executable tclsh or tcltest binary.
TCL_LD_FLAGS='-rdynamic'

# Flags to pass to ld, such as "-R /usr/local/tcl/lib", that tell the
# run-time dynamic linker where to look for shared libraries such as
# libtcl.so.  Used when linking applications.  Only works if there
# is a variable "LIB_RUNTIME_DIR" defined in the Makefile.
TCL_LD_SEARCH_FLAGS=''

# Additional object files linked with Tcl to provide compatibility
# with standard facilities from ANSI C or POSIX.
TCL_COMPAT_OBJS=''

# Name of the ranlib program to use.
TCL_RANLIB='ranlib'

# -l flag to pass to the linker to pick up the Tcl library
TCL_LIB_FLAG=''

# String to pass to linker to pick up the Tcl library from its
# build directory.
TCL_BUILD_LIB_SPEC='-L/usr/local/sce/ee/gcc/build/tcl/unix -ltcl8.0'

# String to pass to linker to pick up the Tcl library from its
# installed directory.
TCL_LIB_SPEC='-L/usr/local/sce/ee/gcc/lib -ltcl8.0'

# Indicates whether a version numbers should be used in -l switches
# ("ok" means it's safe to use switches like -ltcl7.5;  "nodots" means
# use switches like -ltcl75).  SunOS and FreeBSD require "nodots", for
# example.
TCL_LIB_VERSIONS_OK='ok'

# String that can be evaluated to generate the part of a shared library
# name that comes after the "libxxx" (includes version number, if any,
# extension, and anything else needed).  May depend on the variables
# VERSION and SHLIB_SUFFIX.  On most UNIX systems this is
# ${VERSION}${SHLIB_SUFFIX}.
TCL_SHARED_LIB_SUFFIX='${VERSION}${SHLIB_SUFFIX}'

# String that can be evaluated to generate the part of an unshared library
# name that comes after the "libxxx" (includes version number, if any,
# extension, and anything else needed).  May depend on the variable
# VERSION.  On most UNIX systems this is ${VERSION}.a.
TCL_UNSHARED_LIB_SUFFIX='${VERSION}.a'

# Location of the top-level source directory from which Tcl was built.
# This is the directory that contains a README file as well as
# subdirectories such as generic, unix, etc.  If Tcl was compiled in a
# different place than the directory containing the source files, this
# points to the location of the sources, not the location where Tcl was
# compiled.
TCL_SRC_DIR='/usr/local/sce/ee/gcc/src/tcl'

# List of standard directories in which to look for packages during
# "package require" commands.  Contains the "prefix" directory plus also
# the "exec_prefix" directory, if it is different.
TCL_PACKAGE_PATH='/usr/local/sce/ee/gcc/lib'
