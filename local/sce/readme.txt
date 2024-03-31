[SCE CONFIDENTIAL DOCUMENT]
"PlayStation 2" Programmer Tool Runtime Library Release 2.5.4
                   Copyright (C) 2002 Sony Computer Entertainment Inc.
                                                   All Rights Reserved
                                                             July 2002

======================================================================
The following libraries and modules have been released provisionally 
as Release 2.5.4. Be sure to replace the old version with this release 
before using.

======================================================================
Precautions
======================================================================
- Development using Release 2.5.4 library requires flash 
  t10000-rel254.bin. Use ioprp254.img to replace IOP default module.

  For more details of module replacement, refer to SIF system (sif.pdf) 
  and [Tech Notes] on the developers support website.

<Notes on the use of Release 2.5.4 flash>
  The following message appears when terminating the program. However, 
  this does not cause any problem.
 
   "# TLB spad=0 kernel=1:12 default=13:36 extended=37:47"


Contents and changes of the package are as follows.
----------------------------------------------------------------------
Contents of This Package
----------------------------------------------------------------------
sce
/
|---common
|    +---include
|         +---libver.h
|---ee
|    |---include
|    |    |---libmc.h
|    |    +---sifdev.h
|    +---lib
|         |---libkernl.a
|         +---libmc.a
|---iop
|    |---install
|    |    |---include
|    |    |    +---netcnf.h
|    |    +---lib
|    |         +---netcnf.ilb
|    +---modules
|         |---ioprp254.img
|         |---mcman.irx
|         |---mcserv.irx
|         +---netcnf.irx
+---t10000-rel254.bin

----------------------------------------------------------------------
Changes in the Library and Module
----------------------------------------------------------------------
Flash/Replacement Module
----------------------------------------------------------------------
< fileio.irx >
- With sceSifSendCmd(), a part of the supporting process for 
  multithread safe was insufficient. This problem has been fixed.

< sifman.irx >
- With sceSifDmaStat(), DMA termination often cannot be determined 
  correctly. This problem has been fixed.

----------------------------------------------------------------------
EE Kernel
----------------------------------------------------------------------
- With sceSifSendCmd(), a part of the supporting process for 
  multithread safe was insufficient. This problem has been fixed.

----------------------------------------------------------------------
Common Network Configuration Library / Module (netcnf)
----------------------------------------------------------------------
- 'Your Network Configuration file' which is to be created as a new 
  file in the Memory card for "PlayStation 2" (8MB) has been changed 
  to be created with copy protection attribute.

----------------------------------------------------------------------
Memory Card Library / Module
----------------------------------------------------------------------
- In creating and upgrading files and directories, accessing in an 
  extremely short time fails to obtain real-time clock, which causes 
  the time stamp to be abnormal value. This problem has been fixed.


----------------------------------------------------------------------
Usage rights and restrictions
----------------------------------------------------------------------
All rights and restrictions regarding the use of this software are
according to the contract concluded between your company and Sony
Computer Entertainment Inc.

----------------------------------------------------------------------
Written notice related to trademarks 
----------------------------------------------------------------------
"PlayStation" and PlayStation logos are registered trademarks of Sony
Computer Entertainment Inc.  Product names that are mentioned in this
software are trademarks and registered trademarks of their respective
owners.
----------------------------------------------------------------------
Copyrights
----------------------------------------------------------------------
The copyright of this software belongs to Sony Computer Entertainment. 
Inc.

