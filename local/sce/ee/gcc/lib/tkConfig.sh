# tkConfig.sh --
# 
# This shell script (for sh) is generated automatically by Tk's
# configure script.  It will create shell variables for most of
# the configuration options discovered by the configure script.
# This script is intended to be included by the configure scripts
# for Tk extensions so that they don't have to figure this all
# out for themselves.  This file does not duplicate information
# already provided by tclConfig.sh, so you may need to use that
# file in addition to this one.
#
# The information in this file is specific to a single platform.
#
# RCS: @(#) $Id: tkConfig.sh.in,v 1.11 1999/02/11 02:16:51 spolk Exp $

# Tk's version number.
TK_VERSION='8.0'
TK_MAJOR_VERSION='8'
TK_MINOR_VERSION='0'
TK_PATCH_LEVEL='.4'

# -D flags for use with the C compiler.
TK_DEFS=' -DHAVE_UNISTD_H=1 -DHAVE_LIMITS_H=1 -DSTDC_HEADERS=1 -DHAVE_SYS_TIME_H=1 -DTIME_WITH_SYS_TIME=1 '

# Flag, 1: we built a shared lib, 0 we didn't
TK_SHARED_BUILD=0

# The name of the Tk library (may be either a .a file or a shared library):
TK_LIB_FILE='libtk8.0.a'

# The full path to the Tk library for dependency tracking
TK_LIB_FULL_PATH='/usr/local/sce/ee/gcc/build/tk/unix/libtk8.0.a'

# Additional libraries to use when linking Tk.
TK_LIBS='-L/usr/X11R6/lib -lX11 -ldl  -lieee -lm'

# Top-level directory in which Tcl's platform-independent files are
# installed.
TK_PREFIX='/usr/local/sce/ee/gcc'

# Top-level directory in which Tcl's platform-specific files (e.g.
# executables) are installed.
TK_EXEC_PREFIX='/usr/local/sce/ee/gcc'

# CYGNUS LOCAL
# -I switch(es) to pick up the tk.h header file from its build
# directory.
TK_BUILD_INCLUDES='-I/usr/local/sce/ee/gcc/src/tk/generic'
# END CYGNUS LOCAL

# -I switch(es) to use to make all of the X11 include files accessible:
TK_XINCLUDES='-I/usr/X11R6/include'

# Linker switch(es) to use to link with the X11 library archive.
TK_XLIBSW='-L/usr/X11R6/lib -lX11'

# String to pass to linker to pick up the Tk library from its
# build directory.
TK_BUILD_LIB_SPEC='-L/usr/local/sce/ee/gcc/build/tk/unix -ltk8.0'

# String to pass to linker to pick up the Tk library from its
# installed directory.
TK_LIB_SPEC=''

# Location of the top-level source directory from which Tk was built.
# This is the directory that contains a README file as well as
# subdirectories such as generic, unix, etc.  If Tk was compiled in a
# different place than the directory containing the source files, this
# points to the location of the sources, not the location where Tk was
# compiled.
TK_SRC_DIR='/usr/local/sce/ee/gcc/src/tk'

# Needed if you want to make a 'fat' shared library library
# containing tk objects or link a different wish.
TK_CC_SEARCH_FLAGS=''
TK_LD_SEARCH_FLAGS=''

