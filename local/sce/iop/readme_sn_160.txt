Win32 IOP Tools
---------------
The GNU tools in this release are based on the 1.6.0/ 2.8.1 release.  They will work with the 1.6.0 and current (2.x.x series) libraries.

IOP C++ Compiler
----------------
This is a new feature of the SN tools, you should read at least the first couple of paragraphs of readme_sn_C++_iop.txt as it contains important info on getting C++ working correctly (how to call global constructors and wrapping the SCE headers in extern 'C', with examples).

*** NEW for this release of ProDG - STL for IOP.  Follow the basic instructions in the readme_sn_C++.txt (which only involves changing one line of your sn.ini if you installed ProDG from a zip file instead of the installer - if you used the installer then STL will work as normal, with the exception of IOstream, which is not implemented). ***


Installation
------------
If you are installing these tools from a zip, follow this procedure:

1.  Ensure the relevant library release from Sony have been installed.
2.  Unzip this file onto the root.
3.  Add the absolute path of \usr\local\sce\iop\gcc\bin to your autoexec PATH setting, eg 
SET PATH=%PATH%;c:\usr\local\sce\iop\gcc\bin
4.  Now you must set the PS2_DRIVE environment, unless it is already set up (by the EE tools) eg
set PS2_DRIVE=c
(NB no colon-slash after the drive letter)
And add the following two lines:
SET IOP_CPATH=%PS2_DRIVE%:\usr/local/sce/iop/gcc/lib/gcc-lib/mipsel-scei-elfl/2.8.1/include
SET IOP_CPATH=%IOP_CPATH%;%PS2_DRIVE%:\usr/local/sce/iop/gcc/mipsel-scei-elfl/include
5.  Now unzip the tools from the zipfile.  Extract these files into the drive where you have the GNU software.  The relative paths will then place the files in the correct directories.

If you have installed the IOP tools through InstallShield, you do not need to do this since InstallShield will have done it for you.

IMPORTANT !!!:
--------------

Setting up install directories.
-------------------------------

As with linux, after installing the 2.x.x libs, you must copy the contents of the 'iop\install' to 'iop\gcc\mipsel-scei-elfl'.


Building IOP demos
------------------
To build the IOP demos, you must change the makefile line that reads:

iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)

	to

iop-path-setup

This is because DOS can't execute the command as written.

Note also that the iop-path-setup program is currently just a batch 
tool which creates the correct path defs into the current directory.  In the
release tools this will be replaced but if you don't change the positions of the
build tools it will work perfectly.


ASSUMPTIONS:

1.  It is assumed that you have already installed the Win32 EE Tools, purely for the make.exe
program.  If you have not, then obtain make.exe from the internet or from our Win32 EE tools,
and put it in the \usr\local\sce\iop\gcc\bin directory.

Regards,

Dave Brown, 
SN Systems.
26 Feb 2001.
