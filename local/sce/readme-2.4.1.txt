[SCEI CONFIDENTIAL DOCUMENT]
"PlayStation 2" Programmer Tool Runtime Library Release 2.4.1
                Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                                   All Rights Reserved
                                                             Oct. 2001

======================================================================
The following libraries and modules are released provisionally as 
Release2.4.1.
Please replace the previous version with this release.

========================================================================
Precaution
========================================================================
- Please be sure to use FLASH t10000-rel241.bin if you develop program
  using Release2.4.1 libraries. Or replace IOP default module using 
  ioprp241.img.
  Please refer to SIF system (sif.pdf) and "Technical Information" on 
  the support website for details of replacing the modules.
	
  <Cautions on Using Release 2.4.1 Flash>
  The following message will appear after the program finishes, however, 
  it does not affect the program.
  
  "# TLB spad=0 kernel=1:12 default=13:36 extended=37:47"
  
  The contents and changes of the package are shown below:
----------------------------------------------------------------------
Contents of This Package
----------------------------------------------------------------------
sce 
/
|---readme_e.txt
|---doc(pdf)
|  |--ee
|  |  |--libref
|  |  |  |--graphics
|  |  |  |  +---gp_rf
|  |  |  +---inet
|  |  |      +---insck_rf
|  |  +---overview
|  |      +---graphics
|  |          +---libgp
|  +---tool
|      +---dsnet
|---common
|  +---include
|     +---libver.h
|---ee
|  |----include
|  |  |----eestruct.h
|  |  |----libdev.h
|  |  |----libgp.h
|  |  |----libhip.h
|  |  |----libinsck
|  |  |  |----arpa
|  |  |  |  +---inet.h
|  |  |  |----netdb.h
|  |  |  |----netinet
|  |  |  |  |----in.h
|  |  |  |  +---tcp.h
|  |  |  +---sys
|  |  |     +---socket.h
|  |  |----libinsck.h
|  |  |----libmpeg.h
|  |  |----libmrpc.h
|  |  |----libpad.h
|  |  |----libvu0.h
|  |  |----ntguicnf.h
|  |  +---sifdev.h
|  |----lib
|  |  |----libdev.a
|  |  |----libgp.a
|  |  |----libhip.a
|  |  |----libinsck.a
|  |  |----libkernl.a
|  |  |----libmpeg.a
|  |  |----libmrpc.a
|  |  |----libpad.a
|  |  |----libvu0.a
|  |  |----ntgui_e.a
|  |  +----ntgui_j.a
|  |----sample
|  |  +----graphics
|  |  |  +---gp
|  |  |     |----balls
|  |  |     |----data
|  |  |     |----filter
|  |  |     |----multiwin
|  |  |     |----texsort
|  |  |     |----withhig
|  |  |     +---zsort
|  |  |----inet
|  |  |  +---socket
|  |  |     +---echo_server
|  |  +---mpeg
|  |     +---ezmpegstr
|  +---src
|     +---lib
|        |----hip
|        +---vu0
|---iop
|  |----gcc/mipsel-scei-elfl
|  |  |----include
|  |  |  |----inet
|  |  |  |  |----arp.h
|  |  |  |  |----dhcp.h
|  |  |  |  |----dns.h
|  |  |  |  |----ether.h
|  |  |  |  |----icmp.h
|  |  |  |  |----in.h
|  |  |  |  |----inet.h
|  |  |  |  |----inetctl.h
|  |  |  |  |----ip.h
|  |  |  |  |----modem.h
|  |  |  |  |----netdev.h
|  |  |  |  |----pppctl.h
|  |  |  |  |----tcp.h
|  |  |  |  +----udp.h
|  |  |  |----libsd.h
|  |  |  |----modsesq.h
|  |  |  +---msifrpc.h
|  |  +----lib
|  |     |----inet.ilb
|  |     |----inetctl.ilb
|  |     |----libsd.ilb
|  |     |----modsesq.ilb
|  |     +---msifrpc.ilb
|  |----modules
|  |  |----dbcman.irx
|  |  |----inet.irx
|  |  |----inetctl.irx
|  |  |----ioprp241.img
|  |  |----libsd.irx
|  |  |----modsesq.irx
|  |  |----msifrpc.irx
|  |  |----ntguicnf.irx
|  |  |----padman.irx
|  |  |----ppp.irx
|  |  +---smap.irx
|  +---util
|     +---inet
|        +---ifconfig.irx
|---t10000-rel241.bin

-----------------------------------------------------------------------
New Additions
-----------------------------------------------------------------------
- Basic Graphics Library (libgp) has been added.
     
- The following sample programs that use the basic graphics library have 
  been added.
     ee/sample/graphics/gp/balls       	Sample of balls
     ee/sample/graphics/gp/filter      	Sample of image filtering process
     ee/sample/graphics/gp/multiwin    	Sample that operates using HiG GS 
					service drawing environment
     ee/sample/graphics/gp/texsort     	Sample of texture sort
     ee/sample/graphics/gp/withhig     	Sample of combination with HiG  
     ee/sample/graphics/gp/zsort       	Z sort sample

-----------------------------------------------------------------------
Changes on Library Module
-----------------------------------------------------------------------
Flash
------------------------------------------------------------------------
- In interrupt handler, it has been changed to display the context of 
  thread that is in the state of RUN. 
  
  Restrictions on EE thread expansion of DBGP
  'When breaking program with interrupt hander, the current value is 
  returned if register that is in the state of RUN is received.' 
  is not necessary any more.

- A feature that enables switching RGB/YCrCb by using EE boot parameter 
  of dsedb/dsidb has been added.

- A feature that enables switching the memory size of EE, 128MB/32MB by 
  using EE boot parameter of dsedb/dsidb has been added.

  Please refer to the document, "DSNET Overview" for details.


-----------------------------------------------------------------------
EE Kernel
-----------------------------------------------------------------------
- The process that used the function printf() has been changed to a process
  which uses scePrintf().

- In the event that stack is short when thread is switched or is created, 
  it has been changed to issue the following notice and then stop the 
  operation.
  "#tid 3 stack overflow: stack 0x00111d80 > stackpointer 0x00111c0c"

- In the event that the value of stack pointer is not a multiple of 16 bytes 
  when thread is switched, it has been changed to issue the following notice 
  and then stop the operation.

   "#tid 1 stackpointer 0x07ff69a8 must be devided by 1"

- There was a failure that TLB exception was issued when a start interrupt 
  of V blank occurred with the combination of sceGsSyncV() and 
  sceGsSyncVCallBack(). This problem has been fixed.

<Change of eestruct.h>
- Definitions of the structure of GIF packet mode register has been added.

-----------------------------------------------------------------------
SIF RPC that Supports Multhithread
-----------------------------------------------------------------------
- In case that sceSifMBindRpc() and sceSifMBindRpcParam() stopped in error,
  it has been changed to delete semaphore created internally before 
  the operation stops .

- The program may hang when using sceSifMUnBindRpc() since the member of 
 sceSifMServeEntry structure has not been initialized. 

-----------------------------------------------------------------------
High Level Graphics Plugin Library
-----------------------------------------------------------------------
- With sceHiPlugHrchy(), there was a failure that conversion matrix was 
  incorrect when pivot data existed. This problem has been fixed.

-----------------------------------------------------------------------
Controller Library/Module
-----------------------------------------------------------------------
- With scePadSetActDirect(), even if the submission of actuator 
  information was failed, a return value is returned to notify normal
  termination. This problem has been fixed.

-----------------------------------------------------------------------
MPEG Library
-----------------------------------------------------------------------
- With the argument of sceMpegCreate(), a bus error occurred in case the 
  transferred work area had not been initialized. 
  This problem has been fixed.

-----------------------------------------------------------------------
VU0 Library
-----------------------------------------------------------------------
- With vu0 macro instruction of sceVu0Normalize(), there was a failure 
  in the value of Q register when an exception (including interrupt) 
  occurred. This problem has been fixed.

-----------------------------------------------------------------------
DBC Library/Module
-----------------------------------------------------------------------
- In case the controller driver on IOP detected the connection of the 
  controller prior to scePad2CreateSocket() execution, the connecting 
  information of the controller was not notified to libpad2 correctly. 
  This problem has been fixed.

-----------------------------------------------------------------------
Network Configuration GUI Library (ntgui_j.a/ntgui_e.a)
-----------------------------------------------------------------------
- Since a standard function, malloc/free was being used in the library, 
  it has been changed to use a callback function defined in the 
  application.
-----------------------------------------------------------------------
Network Socket Library
-----------------------------------------------------------------------
- The following functions have been added.
     sceInsockSetRecvTimeout()		Timeout setting for reception
     sceInsockSetSendTimeout()		Timeout setting for submission
     sceInsockAbort()			Abort process
     sceInsockTerminate()		Release memory area 

- With accept() for connection that received SYN, an error 
  occurred with EISCONN(=127). This problem has been fixed.

- In the event that socket() is called from a different thread while the 
  accept() is being called, there was a failure that the socket ID in the 
  accept() had been recognized as "Unused socket ID."  This problem has 
  been fixed.

<Change of libinsck.h>
- A declaration that informs prevention of multiplex reading has been added.
 
-----------------------------------------------------------------------
Network Library/Module
-----------------------------------------------------------------------
- With TCP, there was a failure that a function had not been returned even 
  after the timeout specified with sceInetClose() elapsed when the link was 
  "down."  This problem has been fixed.

-----------------------------------------------------------------------
Network Configuration Library/Module
-----------------------------------------------------------------------
- When the interface of PPP becomes "up" immediately after the interface 
  became "down," sceINETE_BUSY error occurred, and therefore re-connection 
  could not been made. This problem has been fixed.

-----------------------------------------------------------------------
PPP Module
-----------------------------------------------------------------------
- With PPP interface, modem driver that has time restriction was applied 
  in case that the interface becomes "up" immediately after it became 
  "down."

- In case the modem driver changed to abnormal state while the submission 
  is being processed, the state may enter infinite loop. This problem has 
  been fixed.

-----------------------------------------------------------------------
Network Driver Module(smap.irx)
-----------------------------------------------------------------------
- With link status process, the status value that is obtained with 
  sceInetNDCC_GET_LINK_STATUS may not be correct. 
  This problem has been fixed.

- A process, sceInetNDCC_SET_MULTICAST_LIST has been added.

-----------------------------------------------------------------------
ifconfig.irx Utility
-----------------------------------------------------------------------
- A process for testing sceInetNDCC_SET_MULTICAST_LIST has been added.

-----------------------------------------------------------------------
Low Level Sound Library
-----------------------------------------------------------------------
- With sceSdBlockTrans(), after specifying SD_TRANS_MODE_READ, there was 
  a failure that transfer could not be stopped with SD_TRANS_MODE_STOP. 
  This problem has been fixed.

-----------------------------------------------------------------------
CSL Sound Effect Sequencer (modsesq)
-----------------------------------------------------------------------
- With sound effect sequence, there was a failure that SE sequence set 
  volume and SE sequence set panpot specified by SQ files were not 
  reflected correctly when the sound is played. This problem has been 
  fixed.

-----------------------------------------------------------------------
Changes of Samples
-----------------------------------------------------------------------

ee/sample/inet/socket/echo_server 
  The process to call ExitThread() has been changed to a process to call 
  ExitDeleteThread() and sceInsockTerminate(0); has been changed to process
  release of calling resource. 

ee/sample/mpeg/ezmpegstr
  The program may hang occasionally due to a problem of management of non-
  multiplex buffer.

-----------------------------------------------------------------------
Abbreviations
-----------------------------------------------------------------------
Putting a priority on the legibility, the following abbreviations are 
used in documents contained in a release package.

Hard disk drive
 Unless specifically referred to otherwise, an external hard disk drive 
 (** GB) (for PlayStation 2) and hard disk drive (EXPANSION BAY type ** GB)
 (for PlayStation 2) are described as the hard disk drive.

Network adaptor
 A network adaptor (EXPANSION BAY type)(for PlayStation 2) and a network 
 adaptor (PC CARD type) (for PlayStation 2) are described as the network 
 adaptor.

Memory card (PS2)
 A memory card (8MB) (for PlayStation 2) is described as 
 the memory card (PS2).

Multitap
 A multitap (for PlayStation 2) is described as the multitap.
   
DUALSHOCK
 An analog controller (DUALSHOCK) is described as the DUALSHOCK.
   
DUALSHOCK 2
 An analog controller (DUALSHOCK 2) is described as the DUALSHOCK 2.

Debugging station
 A debugging station (for PlayStation 2) is described as the debugging 
 station.
  
-----------------------------------------------------------------------
Usage rights and restrictions
-----------------------------------------------------------------------
All rights and restrictions regarding the use of this software are
according to the contract concluded between your company and Sony
Computer Entertainment Inc.

-----------------------------------------------------------------------
Written notice related to trademarks
-----------------------------------------------------------------------
"PlayStation" and PlayStation logos are registered trademarks of Sony
Computer Entertainment Inc. All other company names and product names 
that are mentioned in this software are trademarks and registered 
trademarks of their respective owners.

-----------------------------------------------------------------------
Written notice related to copyrights
-----------------------------------------------------------------------
Copyright of this software is attributed to Sony Computer Entertainment 
Inc.

PPP module uses MD4 and MD5 Message-Digest Algorithm of 
RSA Data Security., Inc.

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

