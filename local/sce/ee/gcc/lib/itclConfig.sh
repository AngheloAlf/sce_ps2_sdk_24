# itclConfig.sh --
# 
# This shell script (for sh) is generated automatically by Itcl's
# configure script.  It will create shell variables for most of
# the configuration options discovered by the configure script.
# This script is intended to be included by the configure scripts
# for Itcl extensions so that they don't have to figure this all
# out for themselves.  This file does not duplicate information
# already provided by tclConfig.sh, so you may need to use that
# file in addition to this one.
#
# The information in this file is specific to a single platform.

# Itcl's version number.
ITCL_VERSION='3.0'
ITCL_MAJOR_VERSION='3'
ITCL_MINOR_VERSION='0'
ITCL_RELEASE_LEVEL='0'

# The name of the Itcl library (may be either a .a file or a shared library):
ITCL_LIB_FILE=libitcl3.0.a

# String to pass to linker to pick up the Itcl library from its
# build directory.
ITCL_BUILD_LIB_SPEC='-L/usr/local/sce/ee/gcc/build/itcl/itcl/unix -litcl3.0'

# String to pass to linker to pick up the Itcl library from its
# installed directory.
ITCL_LIB_SPEC='-L/usr/local/sce/ee/gcc/lib -litcl3.0'

# Location of the top-level source directories from which [incr Tcl]
# was built.  This is the directory that contains generic, unix, etc.
# If [incr Tcl] was compiled in a different place than the directory
# containing the source files, this points to the location of the sources,
# not the location where [incr Tcl] was compiled.
ITCL_SRC_DIR='/usr/local/sce/ee/gcc/src/itcl/itcl'

# Name and location of the incr tcl shell. Used during the build process.
ITCL_SH='/usr/local/sce/ee/gcc/build/itcl/itcl/unix/itclsh'

# Full path to itcl library for dependency checking.
ITCL_LIB_FULL_PATH='/usr/local/sce/ee/gcc/build/itcl/itcl/unix/libitcl3.0.a'