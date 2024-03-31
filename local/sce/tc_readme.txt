[SCEI CONFIDENTIAL DOCUMENT]
"PlayStation 2" Programmer Tool Tool Chain EE 2.96-ee-001003-1
                Copyright (C) 2002 Sony Computer Entertainment Inc.
                                                All Rights Reserved

======================================================================

The problems in the Tool Chain EE (version ee-001003-update-01) have been
fixed in this package.
For detailed changes, please refer to gccchg_e.txt.


----------------------------------------------------------------------
Contents of this Package
----------------------------------------------------------------------
/sce
 |--1st_read
 |   |--bldgcc_e.txt
 |   |--gccchg_e.txt
 |   +--gdb_e.txt
 |--doc
 |   +--tips
 |       +--ee_tools.pdf
 +--ee
     +--gcc


----------------------------------------------------------------------
Notes on Transferring Data from the Previous Version 
(version Tool Chain ee-001003-update-01)
----------------------------------------------------------------------
- As of this package, the recommended work environment has been changed 
  from RedHat LINUX 5.2 to RedHat LINUX 6.2 or later.

  Due to this change, this package does not operate on the RedHat 
  LINUX 5.2.
  If you are still using it, please upgrade to RedHat LINUX 6.2 or later.

  Also, when using this package with other LINUX, be sure to use Libc6 
  or later before use.

----------------------------------------------------------------------
Notes
----------------------------------------------------------------------
- SCEI made some modifications to a part of GNU tools provided in a
  binary format.  All the modified codes are provided as patch files,
  and the source codes are provided as in the original state.  
  GNU tool copyright holders are not responsible for any problems that
  may occur due to these modifications.

-----------------------------------------------------------------------
Permission and Limitation on Use
-----------------------------------------------------------------------
The permission, limitation, etc. on using this software conform to the 
contract concluded between your company and our company (Sony Computer 
Entertainment Inc.)


-----------------------------------------------------------------------
Notes on Trademarks
-----------------------------------------------------------------------
"PlayStation" and PlayStation logos are registered trademarks of Sony
Computer Entertainment Inc.
All product names included in this software are trademarks and 
registered trademarks of their respective owners. 
