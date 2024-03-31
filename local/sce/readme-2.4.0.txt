[SCEI CONFIDENTIAL DOCUMENT]
"PlayStation 2" Programmer Tool Runtime Library Release 2.4
                Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                                   All Rights Reserved
                                                             Sep. 2001
======================================================================
This is a software environment to develop and execute applications 
with the DTL-T10000.  The release revision of this software is 2.4.  

A setup of the DTL-T10000 must be completed prior to installation.
For details, please refer to the setup manual (doc/general/setup).
For the latest software information, please refer to the Developer
Support Web Site.

Precautions
----------------------------------------------------------------------
[Power-Off Process]
- The capability of unloading the cdvdfsv.irx module has been included 
  from Release 2.3.4.  When unloading cdvdfsv.irx, libcdvd cannot be 
  called from the EE.  This has disabled the conventional process of 
  powering off the hard disk drive with sceCdPOffCallback() and 
  sceCdPowerOff().    

  The following is the power-off process after unloading the cdvdfsv.irx 
  module.  For details, see the overview of the CD(DVD)-ROM library and 
  the reference of the standard I/O functions (EE).
 
    1. Detect an interrupt with scePowerOffHandler().
    2. Close files.
    3. Turn off dev9 power.
    4. Turn off "PlayStation 2" MAIN POWER with CDIOC_POWEROFF of 
       the standard function sceDevctl().

- Make sure to use the flash t10000-rel24.bin for developing the
  program with the libraries of Release 2.4 or replace the IOP
  default modules with replacement modules using ioprp24.img.
  For the description on module replacement, please see SIF system 
  (sif.pdf).

  <Notes on Using the Flash in Release 2.4>
  The following message displayed upon exit from the program does not 
  present any problem.
 	# TLB spad=0 kernel=1:12 default=13:36 extended=37:47

<Disk Boot with DTL-T10000/DTL-T10000H>
- With the flash change in this release, it has been enabled to change 
  the IOP memory size from 8 MB to 2 MB when starting an application from 
  the ordinary start-up disk through the use of the boot parameter of 
  dsedb/dsidb.

- With the flash change in this release, it has been enabled to start 
  an application independently of the flash version when starting it 
  from the ordinary start-up disk through the use of the boot parameter 
  of dsedb/dsidb.

  For details, refer to the "Disk Activation with DTL-T10000" and "DSNET 
  Overview" documents.

Additions since Release 2.3
----------------------------------------------------------------------
Libraries
----------------------------------------------------------------------
<Release 2.4>
- The following controller-related libraries and modules have been added.
  They have been created by dividing the conventional controller library 
  by feature.  The numbers inside the parentheses show thread priority.
  Using these libraries and modules can reduce memory consumption.
  They do not support the multitap.

  Note: The following libraries and modules cannot be used with 
        the conventional controller library. 
        When using these libraries or modules, sio2man.irx in this 
	release or later is required.

   ee/lib/libpad2.a
   ee/lib/libvib.a
   ee/lib/libdbc.a
   iop/modules/dbcman.irx   (20)
   iop/modules/sio2d.irx    (46)
   iop/modules/dgco.irx     (46)
   iop/modules/ds1u.irx     (46)
   iop/modules/ds2u.irx     (46)
   iop/modules/ds1o.irx     (46)
   iop/modules/ds2o.irx     (46)


- The environment conventionally announced as "IOP standard module 
  environment" has been newly added as "standard kit library/sound 
  system".
  This facilitates sound processing on the EE without necessity of 
  programming on the IOP.  The number inside the parentheses shows 
  thread priority.

   ee/lib/libsk.a
   iop/modules/sksound.irx  (32)
   iop/modules/skhsynth.irx
   iop/modules/skmidi.irx
   iop/modules/sksesq.irx
   iop/modules/skmsin.irx


- The SPU2 local memory management library has been added.
     ee/lib/libspu2m.a

- The CSL SE stream generation library (EE) has been added.
     ee/lib/libsein.a

- A header file has been added to define versions of the currently-
  installed libraries.  
     common/include/libver.h
     
- The English version of the network configuration GUI library 
  (ntgui_e.a) has been added.
  Herewith this change, the Japanese version has been renamed from 
  ntguicnf.a to ntgui_j.a.

- The Japanese-version and English-version icon files for 
  the common network configuration library/modules (netcnf) have been 
  added to the following directories.
  [Japanese Version]
     conf/neticon/japanese/icon.sys
     conf/neticon/japanese/SYS_NET.ICO
  [English Version]
     conf/neticon/english/icon.sys
     conf/neticon/english/SYS_NET.ICO

<Release 2.3.3>
- The official version of the network configuration GUI library 
  (ntguicnf.a) has been added.
  There are significant changes to structures and functions since
  beta version. Make sure to replace the previous version with this
  version and rebuild your program.
  In the samples of this library, UTF8 character code conversion
  library package is used (libccc).

----------------------------------------------------------------------
Samples
----------------------------------------------------------------------
ee/sample/pad2/basic
- A sample program to display controller information using the 
  controller library 2 (libpad2) has been added.

ee/sample/pad2/vib
- A sample program to control the actuator using the vibration 
  library has been added.

ee/sample/sk/ctrlsq
- A sample program to play back various types of music data 
  with standard kit/sound system has been added.

ee/sample/sk/playsq
- A sample program to play back sequence data with standard 
  kit/sound system has been added. 

ee/sample/mpeg/ezmpegstr
- A minimal sample program to play back an MPEG2 stream with sound for 
  PlayStation 2 (PSS) has been added.

iop/sample/sound/sqsong
- A sample program to play SONG chunks in sequence data (SQ file) has 
  been added.  The following sample data has also been added.
     data/sound/seq/overlo_s.sq
     data/sound/wave/overlo_s.bd
     data/sound/wave/overlo_s.hd

<Release 2.3.4>
- A sample program to access CD/DVD media via the standard I/O has 
  been added.  This sample has unloading capability built into it.
     ee/sample/cdvd/fio_ee
     
- A sample program to enable self-deletion of the USB device driver 
  (applicable to usbmload.irx) and a configuration file have been added.
     iop/sample/usb/selfunld
     conf/usb/selfunld.cnf

<Release 2.3.3>
- The following sample programs using the network configuration GUI 
  library have been added.
     iop/sample/inet/ntguicnf/setinit
     iop/sample/inet/ntguicnf/usbinit

<Release 2.3.1>
- A sample program to communicate with the remote host via File Transfer 
  Protcol (FTP) using the inet library has been added.
	iop/sample/inet/ftp
	
----------------------------------------------------------------------
Documents
----------------------------------------------------------------------
- The controller library 2 document has been added.

- The DBC library document has been added.

- The vibration library document has been added.

- The SPU2 local memory management library document has been added.

- The CSL SE stream generation document has been added.

- The overview of the development tools (systool) has been added.

- The reference of the C standard functions has been added.

- The EE programming tool (ee_util) document has been added.

- The documents of programming tips and movie data creation/playback 
  tips have been added.
  
<Release 2.3.3>
- The official version of the network configuration GUI library 
  document has been added.

- The document of hard disk drive operation rule (hdd_rule) has been 
  added.

  For more information on the changes and known bugs, please refer
  to the respective changes files (libchg_e.txt, docchg_e.txt,
  smpchg_e.txt, tlchg_e.txt and gccchg_e.txt).

----------------------------------------------------------------------
The Available Memory Space of IOP in the Release 2.4
----------------------------------------------------------------------

(A)   Startup			1.63 Mb free
(B)   A + sio2man + padman	1.57 Mb free
(C)   B + mcman	+ mcserv	1.40 Mb free
(D)   C + libsd + sdrdrv	1.37 Mb free

* When using the Multi Tap, additional 30 Kbytes are consumed.

Please refer to setup2_e.txt for the source code of the sample program 
to calculate the free memory space.


----------------------------------------------------------------------
Installation
----------------------------------------------------------------------

To install this software, please follow the steps below.

1. Downloading Release 2.4 Package

2. Copying Files to Development Machine

	Copy the files under /sce to /usr/local.
	
	In this case, chmod should be executed to the related 
	directories in the succeeding work (e.g. recompiling samples) 
	as occasion demands.

	When copying to a desired location (e.g. /home/xxx/sce), 
	execute the following with the root authority:
	
	# ln -s /home/xxx/sce /usr/local
	
	This is because the gcc-related tools have been created 
	assuming that the software environment is expanded under 
	/usr/local/sce/ee (iop). 

3. Installing Tool Chain

	Download the compilers below from the Developer Support
	website. Expand each compiler under 
        /usr/local/sce/ee (iop). 

		Tool Chain EE 2.96-ee-001003
                Tool Chain IOP 2.8.1

4. Updating dsnet

	Start up "T10000 control tool" with WWW browser in a 
	development machine and update by specifying rpm package.
	For details, see setup.txt.

5. Environment Variables Settings

	Specify the host name of the DTL-T10000 to the environment
	variable DSNETM.  By setting the environment variable DSNETM,
	the user do not have to specify the target name with an option 
	when using dsnet-related commands.

6. Setting Up Software

        sce/1st_read/setup.txt
        sce/doc/pdf/general/setup.pdf

	Please refer to the "Setup Guide to Development Environment"
	document.

----------------------------------------------------------------------
Notes
----------------------------------------------------------------------
- The recommended work environment is RedHat LINUX 5.2.  
  When using this software with other LINUX series, ensure to use
  Libc6 or later.

-----------------------------------------------------------------------
Permission and Restrictions on Use
-----------------------------------------------------------------------
The permission, restrictions, etc. on using this software conform to
the contract concluded between your company and our company (Sony
Computer Entertainment Inc).

-----------------------------------------------------------------------
Note on Trademarks
-----------------------------------------------------------------------
"PlayStation" and PlayStation logos are registered trademarks of Sony
Computer Entertainment Inc.
All other company names and product names included in this software are 
trademarks and registered trademarks of their respective owners. 

-----------------------------------------------------------------------
Copyrights
-----------------------------------------------------------------------
The copyright of this software belongs to Sony Computer Entertainment. 
Inc.

PPP module uses MD4 and MD5 message-digest algorithms of RSA Data 
Security, Inc.

RSA Data Security, Inc., MD4 message-digest algorithm
Copyright (C) 1990-2, RSA Data Security, Inc. All rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD4 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD4 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.


RSA Data Security, Inc., MD5 message-digest algorithm
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991.
All rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
