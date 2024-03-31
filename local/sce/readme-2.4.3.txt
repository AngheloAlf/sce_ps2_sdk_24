[SCEI CONFIDENTIAL DOCUMENT]
"PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
                Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                                   All Rights Reserved
                                                             Dec. 2001

======================================================================
The following libraries and modules are released provisionally as 
Release2.4.3.  
Please make sure to replace the previous version with this version.

========================================================================
Precaution
========================================================================
- Make sure to use the flash t10000-rel243.bin for developing the
  program with the libraries of Release 2.4.3.
  Replace the IOP default module with the replacement module using
  ioprp243.img.

For the description on module replacement, refer to SIF System (sif.pdf) and the technical information area of the developer support website.

< Notes on Using the Flash in Release 2.4.3 >
  The following message displayed upon exit from the program does not 
  present any problem.
 	# TLB spad=0 kernel=1:12 default=13:36 extended=37:47

< libmc.h >
- The following structure and members have been added.

  sceMcStDateTime		: Structure of date/time 
  sceMcTblGetDir._Create	: Member for entry creation date/time
  sceMcTblGetDir._Modify	: Member for entry modification date/time

  The macros listed below, that had been defined in the libmc.h file up to 
  this release, have been deleted due to the addition of the above 
  structure and members.

  As for user applications that have been using these old macros, define 
  -D__SCE_MC_OLD_DEFINE__ as a compile option to be able to use them in 
  conventional way. 

  However, since these macros will be excluded from the next release, make 
  sure to replace them with the above structure and members by then.

  #define CSec	_Create.Sec
  #define CMin	_Create.Min
  #define CHour	_Create.Hour
  #define CDay	_Create.Day
  #define CMonth _Create.Month
  #define CYear	_Create.Year
  #define MSec	_Modify.Sec
  #define MMin	_Modify.Min
  #define MHour	_Modify.Hour
  #define MDay	_Modify.Day
  #define MMonth _Modify.Month
  #define MYear _Modify.Year

Contents and changes of this package are as follows.
For document changes, refer to docchg_e.txt in this package.
----------------------------------------------------------------------
Contents of This Package
----------------------------------------------------------------------
tlib_243
/
|---doc
|---common
|     +---include
|           |---ilink.h
|           |---ilsock.h
|           |---ilsocksf.h
|           |---libver.h
|           +---modsein.h
|---ee
|     |---include
|     |     |---eeregs.h
|     |     |---libdbc.h
|     |     |---libgp.h
|     |     |---libhig.h
|     |     |---libhip.h
|     |     |---libhttp
|     |     |     |---http_methods.h
|     |     |     |---http_options.h
|     |     |     +---http_status.h
|     |     |---libhttp.h
|     |     |---libinsck
|     |     |     |---arpa
|     |     |     |     +---inet.h
|     |     |     |---netdb.h
|     |     |     |---netinet
|     |     |     |     |---in.h
|     |     |     |     +---tcp.h
|     |     |     +---sys
|     |     |           +---socket.h
|     |     |---libinsck.h
|     |     |---libmc.h
|     |     |---libmrpc.h
|     |     |---libsein.h
|     |     |---libusbkb.h
|     |     |---ntguicnf.h
|     |     +---sifdev.h
|     |---lib
|     |     |---libdbc.a
|     |     |---libgp.a
|     |     |---libhig.a
|     |     |---libhip.a
|     |     |---libhttp.a
|     |     |---libinsck.a
|     |     |---libkernl.a
|     |     |---libmc.a
|     |     |---libmrpc.a
|     |     |---libsein.a
|     |     |---libusbkb.a
|     |     |---ntgui_e.a
|     |     +---ntgui_j.a
|     |---sample
|     |     |---advanced
|     |     |     |---anti
|     |     |     |     |---doc
|     |     |     |     |---refmap-448
|     |     |     |     |---refmap-4times
|     |     |     |     |---refmap-noAA
|     |     |     |     |---refmap-onepass-AAline
|     |     |     |     |---silhouette
|     |     |     |     +---texmap-onepass-AAline
|     |     |     +---collision
|     |     |---basic3d
|     |     |     +---vu1
|     |     |---graphics
|     |     |     |---bumpmap
|     |     |     |---clip_vu1
|     |     |     |---gp
|     |     |     |     +---multiwin
|     |     |     |---hig
|     |     |     |---jointm
|     |     |     |     +---intr_key
|     |     |     |           +---runtime
|     |     |     |---mipmap
|     |     |     |---point_l
|     |     |     |---refmap
|     |     |     |---spot_l
|     |     |     +---tex_swap
|     |     |---ilink
|     |     |---inet
|     |     |     |---echo_server
|     |     |     |---libhttp
|     |     |     |     |---auth
|     |     |     |     |---blocking
|     |     |     |     |---chunk
|     |     |     |     |---cookie
|     |     |     |     |---mime
|     |     |     |     |---normal
|     |     |     |     |---proxy
|     |     |     |     |---qp
|     |     |     |     +---redirect
|     |     |     |---ntguicnf
|     |     |     +---socket
|     |     |           |---echo_server
|     |     |           +---http_get
|     |     +---mc
|     |           +---basic
|     +---src
|           +---lib
|                 +---hip
|---iop
|     |---install
|     |     |---include
|     |     |     |---libsd.h
|     |     |     |---modmidi.h
|     |     |     |---msifrpc.h
|     |     |     |---netcnf.h
|     |     |     |---stdio.h
|     |     |     |---usb.h
|     |     |     +---usbd.h
|     |     +---lib
|     |           |---ilink.ilb
|     |           |---ilsock.ilb
|     |           |---iop.ilb
|     |           |---libsd.ilb
|     |           |---modsein.ilb
|     |           |---msifrpc.ilb
|     |           |---netcnf.ilb
|     |           +---usbd.ilb
|     |---modules
|     |     |---dbcman.irx
|     |     |---ilink.irx
|     |     |---ilsock.irx
|     |     |---ioprp243.img
|     |     |---libsd.irx
|     |     |---mcman.irx
|     |     |---mcserv.irx
|     |     |---modsein.irx
|     |     |---msifrpc.irx
|     |     |---netcnf.irx
|     |     |---ntguicnf.irx
|     |     |---usbd.irx
|     |     +---usbkb.irx
|     +---sample
|           |---ilink
|           +---usb
|                 |---activate
|                 |---selfunld
|                 |---usbkeybd
|                 |---usbmouse
|                 +---usbspkr
+---t10000-rel243.bin

-----------------------------------------------------------------------
New Additions
-----------------------------------------------------------------------
- iop/sample/usb/usbspkr
  A sample program to perform Isochronous transfer has been added.

- ee/sample/advanced/collision
  A sample program to judge collision has been added.

- ee/sample/advanced/anti/silhouette
  A sample program to perform antialiasing to object edges via a 
  two-dimensional filter has been added.

- ee/sample/graphics/hig/sample24.c
  A rendering sample program using a fisheye lens effect has been added.

- ee/sample/graphics/hig/sample25.c
  A sample program to clone an object has been added.

-----------------------------------------------------------------------
Changes Made to Libraries/Modules
-----------------------------------------------------------------------
Flash/Replacement Module
------------------------------------------------------------------------
- A change has been made to return NULL when reaching the end of the 
  file and attempting to read it with fdgets()/gets().

- A change has been made to enable stack check features of the threads on 
  the EE kernel to be switched using the EE boot parameter of dsedb/dsidb.
  Default is disabled.

  For details, please refer to the "DSNET Overview" document in this 
  package. 

[Simple Thread Monitor]
- With the thlist command, when specifying an incorrect thread ID, 
  the system went down. This problem has been fixed.
  In addition, a change has been made to enable specification with 
  the thread number displayed in the list, instead of the thread ID.

- The following options have been added to the semlist and evlist 
  commands.
  -w option	: Displays the thread list connected with the waiting queue
  -v option 	: Specifies detailed display of the thread list

-----------------------------------------------------------------------
EE Kernel
-----------------------------------------------------------------------
< Changes made to eeregs.h >
- The type of IPU_in_FIFO and IPU_out_FIFO has been changed from 
  (volatile u_long*) to (volatile u_long128*).

------------------------------------------------------------------------
Memory Card Library/Module
------------------------------------------------------------------------
- When executing sceMcGetDir() with a negative value specified to the 
  argument maxent, the number of entries of files could be obtained in 
  normal cases. When executing to the PocketStation, however, this 
  function did not return. This problem has been fixed.  

- When accessing a Memory Card (PS2) from an IOP program with mcserv.irx 
  loaded, file access functions such as open did not return. This problem 
  has been fixed.
 
- The following function has been added.

  sceMcEnd()	: Function for end process of memory card environment

< Change made to libmc.h >
- The const modifier has been added to the second argument (void*) in 
  the prototype declaration of sceMcWrite().

- The following structure and members have been added.

  sceMcStDateTime		: Structure of date/time 
  sceMcTblGetDir._Create	: Member for entry creation date/time
  sceMcTblGetDir._Modify	: Member for entry modification date/time

- The macros listed below, that had been defined in the libmc.h file up to 
  this release, have been deleted due to the addition of the above 
  structure and members.

  As for user applications that have been using these old macros, define 
  -D__SCE_MC_OLD_DEFINE__ as a compile option to be able to use them in 
  conventional way. 

  However, since these macros will be excluded from the next release, make 
  sure to replace them with the above structure and members by then.

  #define CSec	_Create.Sec
  #define CMin	_Create.Min
  #define CHour	_Create.Hour
  #define CDay	_Create.Day
  #define CMonth _Create.Month
  #define CYear	_Create.Year
  #define MSec	_Modify.Sec
  #define MMin	_Modify.Min
  #define MHour	_Modify.Hour
  #define MDay	_Modify.Day
  #define MMonth _Modify.Month
  #define MYear _Modify.Year

-----------------------------------------------------------------------
Basic Graphics Library (libgp)
-----------------------------------------------------------------------
- The following functions have been added.
 sceGpKickChain2()		: Transfers chain (Does not check whether 
				  a DMA channel is available or not) 
 sceGpSetEndLevel()		: Sets transfer end level of chain
 sceGpSetTexEnvByDrawEnv()	: Sets texture environment configuration 
				  packet
 sceGpAddChain2()		: Registers sub chain after the specified 
				  position 
 sceGpAddPacket2()		: Registers packet after the specified 
				  position
 sceGpCallChain2()		: Registers call of sub chain after the 
				  specified position
 sceGpGetTail()			: Gets tail pointer of packet 
 sceGpGetTailChain()		: Gets tail pointer of the specified level 
				  of chain
 sceGpSearchTailToRemove()	: Gets tail pointer for removal 
 sceGpRemoveNextPacket()	: Removes packet from chain
 sceGpRemoveNextChain() 	: Removes chain from main chain
 sceGpSetPacketMode()   	: Changes transfer mode of packet
 sceGpSyncPacket()/sceGpSyncPacketI()	
				: Flushes cache within packet area
 sceGpKickPacket()		: Transfers a single packet
 sceGpKickPacket2()		: Transfers a single packet

-----------------------------------------------------------------------
High Level Graphics Library
-----------------------------------------------------------------------
- sceHiDMARegist() has been sped up.
  ID of the return value has been changed from number to address.

- sceHiMemInit() and sceHiDMAInit() hung up when exceeding the heap area.
  This problem has been fixed.
  
- sceHiGsStdCtx display set to "double" was kept unchanged when changing
  the setting to "single". This problem has been fixed.

- With sceHiGsServiceSetRegistFunc(), a change has been made to return 
  the setting to the initial value when specifying the argument NULL. 

- The following function has been added.
  sceHiGsCtxSetHalfOffset() 	: Sets drawing offset data in interlace 
				  mode

< Change made to libhig.h >
- *_PROCESS defined as an old macro up to this release has been deleted 
  from libhig.h.
  Make sure to replace *_PROCESS with SCE_HIG_*_PROCESS.

-----------------------------------------------------------------------
High Level Graphics Plugin Library
-----------------------------------------------------------------------
- The following functions and structures have been added.

 sceHiPlugShapeHead_t			: SHAPE Header Union 
 sceHiPlugShapeMatrix_t			: BASEMATRIX Data Structure 1
 sceHiPlugHrchyHead_t			: HRCHY Header Structure 
 sceHiPlugHrchyData_t			: HRCHY Data Structure 
 sceHiPlugTex2DHead_t			: TEX2D Header Structure 
 sceHiPlugTex2DData_t			: TEX2D Data Structure 
 sceHiPlugMicroLight_t			: MICRO Light Structure   
 sceHiPlugMicroData_t			: MICRO Data Structure  
 sceHiPlugShadowBoxData_t		: SHADOWBOX Data Structure  
 sceHiPlugClutBumpHead_t		: CLUTBUMP Header Structure 
 sceHiPlugClutBumpData_t		: CLUTBUMP Data Structure  
 sceHiPlugTim2Head_t			: TIM2 Header Structure  
 sceHiPlugTim2Data_t			: TIM2 Data Structure
 sceHiPlugAnimeHead_t			: ANIME Header Union
 sceHiPlugAnimeData_t			: ANIME Data Structure
 sceHiPlugShareHead_t			: SHARE Header Union
 sceHiPlugShareData_t			: SHARE Data Union
 sceHiPlugShapeGetHead()		: Gets SHAPE Header 
 sceHiPlugShapeGetDataHead()		: Gets Shape Data Header 
 sceHiPlugShapeGetMaterialHead()	: Gets Material Data Header 
 sceHiPlugShapeGetGeometryHead()	: Gets Geometry Data Header 
 sceHiPlugShapeGetMaterialGiftag()	: Gets Material Giftag 
 sceHiPlugShapeGetMaterialAttrib()	: Gets Material Attribute 
 sceHiPlugShapeGetGeometryVertex()	: Gets Geometry Vertex Data  
 sceHiPlugShapeGetGeometryNormal()	: Gets Geometry Normal Data  
 sceHiPlugShapeGetGeometryST()		: Gets Geometry Texture Coordinates
 sceHiPlugShapeGetGeometryColor()	: Gets Geometry Vertex Color 
 sceHiPlugShapeGetMatrix()		: Gets Matrix Data 
 sceHiPlugHrchyGetHead()		: Gets HRCHY Header 
 sceHiPlugHrchyGetData()		: Gets HRCHY Data 
 sceHiPlugHrchyGetPivot()		: Gets PIVOT Data 
 sceHiPlugTex2DGetHead()		: Gets TEX2D Header 
 sceHiPlugTex2DGetData()		: Gets TEX2D Data 
 sceHiPlugTex2DGetTexel()		: Gets TEX2D Texel Data
 sceHiPlugTex2DGetClut()		: Gets TEX2D CLUT Data
 sceHiPlugTex2DGetEnv()			: Gets TEX2D Texture Environment 
					  Data
 sceHiPlugMicroGetData()		: Gets MICRO Data
 sceHiPlugShadowBoxGetData()		: Gets SHADOWBOX Data
 sceHiPlugClutBumpGetHead()		: Gets CLUTBUMP Header
 sceHiPlugClutBumpGetData()		: Gets CLUTBUMP Data
 sceHiPlugClutBumpGetNormal()		: Gets CLUTBUMP Normal Table 
 sceHiPlugTim2GetHead()			: Gets TIM2 Header 
 sceHiPlugTim2GetData()			: Gets TIM2 Data
 sceHiPlugAnimeGetHead()		: Gets ANIME Header
 sceHiPlugAnimeGetData()		: Gets ANIME Data
 sceHiPlugAnimeGetKeyHead()		: Gets ANIME Key Header
 sceHiPlugAnimeGetFrame()		: Gets ANIME Frame Data
 sceHiPlugAnimeGetValue()		: Gets ANIME Value Data
 sceHiPlugShareGetHead()		: Gets SHARE Header
 sceHiPlugShareGetData()		: Gets SHARE Data
 sceHiPlugShareGetShare()		: Gets Share Data
 sceHiPlugShareGetIndex()		: Gets SHARE Index Data
 sceHiPlugShareGetSrc()			: Gets SHARE Vertex/Normal Source 
					  Data
 sceHiPlugShareGetDst()			: Gets SHARE Vertex/Normal 
					  Destination Data


- The declaration of sceVu0FMATRIX in the Hermite() function in 
  ee/src/lib/hip/anime.c file has been changed to the static declaration. 
-----------------------------------------------------------------------
HTTP Library
-----------------------------------------------------------------------
- The descriptions of the following members have been added to 
  the structure sceHTTPClient_t.

  t_notify_opt	   : User-defined argument to the callback function 
		     for end-of-transaction notification
  t_busy	   : Transaction busy flag
  chunkf_opt	   : User-defined argument to the callback function 
		     for chunk receive notification

- Values of void * type have been newly added as arguments.  They are 
  specified for a callback function when registering the transaction end 
  callback and chunk receive callback functions with the sceHTTPSetOption() 
  function. 

  sceHTTPO_EndOfTransactionCB
  sceHTTPO_ReceiveChunkCB

- A change has been made to receive a value of void * type as well, 
  which has been specified at the time of registration, when obtaining 
  the transaction end callback and chunk receive callback functions 
  with the sceHTTPGetOption() function. 

  sceHTTPO_EndOfTransactionCB
  sceHTTPO_ReceiveChunkCB

- The sceHTTPGetClientError() function returned an incorrect error code. 
  This problem has been fixed. 

- When attempting to process a new transaction with the sceHTTPRequest() 
  function while a transaction is in process in non-blocking mode, it 
  resulted in an abnormal operation, not an error. This problem has been 
  fixed. 

- Operation sometimes became abnormal at the end of a transaction in 
  non-blocking mode. This problem has been fixed.

- Error handling has been enhanced in MIME processing.
  A failure in obtaining memory did not cause an error. This problem has 
  been fixed.

- A change has been made to automatically free resources allocated for 
  the Response structure by an old transaction, if there are some, before 
  executing a transaction with the sceHTTPRequest() function. 

- The following function has been added.

  sceHTTPTerminate()		: Terminates library

-----------------------------------------------------------------------
Common Network Configuration Library/Module
-----------------------------------------------------------------------
- When specifying Hard Disk Drive with available memory space checking 
  features including sceNetCnfCheckCapacity(), a change has been made 
  to return sceNETCNF_CAPACITY_ERROR if less than 244 Kbytes are
  available.

- If a Memory Card (PS2) was removed while executing sceNetCnfDeleteEntry(), 
  only files disappeared but the entries remained occasionally. This 
  problem has been fixed.

-----------------------------------------------------------------------
Network Configuration GUI Library (ntgui_j.a/ntgui_e.a/ntguicnf.irx)
-----------------------------------------------------------------------

- The error message for detecting an error on available memory space in the  
  Hard Disk Drive has been changed as follows:

 "Insufficient free space. Check again when you secure free space of 
  244 Kbytes or more."

- A change has been made to return the selected combination name, hardware 
  setting name, network service provider setting name, and the device in 
  which settings are stored.
  (The sceNetGuiCnf_Arg structure in ntguicnf.h has been changed.)

- SCE_NETGUICNF_FLAG_USE_USB_KB in the flag member of the sceNetGuiCnf_Arg 
  structure was not referred in the library. This problem has been fixed.

-----------------------------------------------------------------------
Network Socket Library
-----------------------------------------------------------------------
< Changes made to libinsck.h >
- The description of extern for C++ has been added to the prototype 
  declaration of functions.

- When automatic allocation mode was specified to the local port with 0 
  bound via listen(), a port number different from the one allocated to 
  the first port was allocated to the local port for the second socket 
  and after. This problem has been fixed. 

- getsockname() could not be executed for a listen socket. This problem 
  has been fixed.
 
- getsockname() and getpeername() did not return 0 under normal operating 
  conditions but accessed an undefined address. This problem has been 
  fixed.

- With socket.h, the following symbols have been added to the argument how 
  for shutdown(). 

  #define SHUT_RD    0
  #define SHUT_WR    1
  #define SHUT_RDWR  2

- The size judgment value for *paddrlen in accept(), recvfrom(), 
  getsockname(), and getpeername() was too small. This problem has been 
  fixed.
  (To be more specific, this error occurs to a value lower than 
  sizeof(struct sockaddr_in). In the above case, the value specified for 
  the size judgment was 4 bytes.)

- Since memory for a new socket replenished by accept() was not fully 
  initialized, an unauthorized memory access was sometimes made. This 
  problem has been fixed. 

- There was a possibility that a socket ID for a new socket replenished by 
  accept() might be used in duplicate elsewhere. This problem has been 
  fixed.

------------------------------------------------------------------------
USB Driver Library/Module
------------------------------------------------------------------------
- The following structures have been added.
  sceUsbdIsochronousPswLen	  : Packet structure for multiple 
				    Isochronous transfer
  sceUsbdMultiIsochronousRequest  : Structure for multiple Isochronous 
				    transfer request

- The following function has been added.
  sceUsbdMultiIsochronousTransfer()  : Takes the huge load off the CPU 
				       during Isochronous transfer

-----------------------------------------------------------------------
USB Keyboard Library/Module
-----------------------------------------------------------------------
- If USB keyboards are connected and disconnected repeatedly when the 
  number of USB ports is more than the maximum number of connectable 
  devices set by the user, the system sometimes hung up. This problem 
  has been fixed.

-----------------------------------------------------------------------
Low Level Sound Library
-----------------------------------------------------------------------
- If both CORE0 and CORE1 used an SPU2 interrupt, user's interrupt handler 
  might not be called. This problem has been fixed.
 
- sceSdInit() wrote data exceeding the reserved area in SPU2 local memory. 
  This problem has been fixed. 
-----------------------------------------------------------------------
CSL SE Stream Generation (libsein/modsein)
-----------------------------------------------------------------------
- Libraries could not link. This problem has been fixed using an undefined 
  function.
 
-----------------------------------------------------------------------
CSL MIDI Sequencer
-----------------------------------------------------------------------
< Change made to modmidi.h >
- The constant macro sceMidi_MidiVolumeChange_AllMIDIChannel has been 
  added to sceMidi_MidiVolumeChange() to specify all MIDI channels. 

-----------------------------------------------------------------------
i.LINK Driver Library
-----------------------------------------------------------------------
- ilink.irx module has been made unloadable. 

-----------------------------------------------------------------------
i.LINK Socket Library
-----------------------------------------------------------------------
- ilsock.irx module has been made unloadable. 

-----------------------------------------------------------------------
Device Control Library
-----------------------------------------------------------------------
< Change made to dbcman.irx >
- With scePad2CreateSocket(), when a controller port was specified by the 
  following macros, driver module could not properly control connection 
  and disconnection of the controller. This problem has been fixed.

  SCE_PAD2_PORT_1C
  SCE_PAD2_PORT_2C
  SCE_PAD2_PORT_USB

-----------------------------------------------------------------------
SIF RPC that Supports Multithread
-----------------------------------------------------------------------
- Calling conditions of functions inside the modules were incorrect.  
  This error has been fixed. 

-----------------------------------------------------------------------
Sample Changes
-----------------------------------------------------------------------
- ee/sample/graphics/gp/multiwin 
  This sample program has been changed due to the new addition of 
  functions to the basic graphics library (libgp). 

- ee/sample/graphics/hig/sample6.c, sample8.c, sample16.c, sample18.c
  ee/sample/graphics/hig/sample19.c, sample20.c, sample21.c, sample22.c
  ee/sample/graphics/hig/sample23.c, util.h

  1. The extern __attribute__(.vudata) declaration has been removed. 
  2. Fogging and antialiasing features have been added to sample6.c. 
  3. sample8.c has been changed to a data rewriting sample using a data 
     access feature.
 
- ee/sample/graphics/hig/camera.c
  The inline function performing external reference with extern has 
  been removed. 

- ee/sample/inet/libhttp/auth
  ee/sample/inet/libhttp/blocking
  ee/sample/inet/libhttp/chunk
  ee/sample/inet/libhttp/cookie
  ee/sample/inet/libhttp/mime
  ee/sample/inet/libhttp/normal
  ee/sample/inet/libhttp/proxy
  ee/sample/inet/libhttp/redirect
  ee/sample/inet/libhttp/qp

  1. With auth, blocking, chunk, cookie, mime, normal, proxy, and 
     redirect samples, the type of end callback function has been 
     changed to add a user-defined argument at the time of registration. 

  2. The redirect sample omitted freeing resources. It has been corrected.

  3. With the chunk sample, the type of chunk receive function has been 
     changed to add a user-defined argument at the time of registration.

  4. #include <libhttp.h> has been added to the qp sample. 
     A line break process was omitted while decoding. This problem has 
     been fixed. 

- ee/sample/inet/socket/http_get
  This sample program has been changed to specify SHUT_RDWR, not 2, to 
  the argument how of shutdown(). 

- ee/sample/inet/socket/echo_server
  This sample program has been changed to specify SHUT_RDWR, not 2, to 
  the argument how of shutdown(). 
  A process to free the stack area has also been added. 

- ee/sample/inet/echo_server
  1. maxCount of fin_sema in main.c has been changed to MAX_CONNECTION. 

  2. The relationship between end of thread and thread stack release has 
     been corrected. 

- ee/sample/inet/ntguicnf
  1. Changes made to the USE_USBKB macro were not reflected in 
     SCE_NETGUICNF_FLAG_USE_USB_KB in the flag member of 
     the sceNetGuiCnf_Arg structure. This problem has been fixed. 

  2. A change has been made to the network configuration GUI library to 
     return the selected combination name, hardware setting name, network 
     service provider setting name, and the device in which settings are 
     stored. Due to this change, this sample program has been changed.  
     
- ee/sample/graphics/jointm/intr_key/runtime
  Transfer packets were occasionally rewritten during DMA transfer 
  depending on the timing. This problem has been fixed. 

- ee/sample/graphics/jointm/intr_key/runtime
  ee/sample/basic3d/vu1
  ee/sample/graphics/mipmap 

  VIF input occasionally rewrote the VUMEM in active use depending on the 
  timing. This problem has been fixed. 

- ee/sample/advanced/anti/refmap-448
  ee/sample/advanced/anti/refmap-4times
  ee/sample/advanced/anti/refmap-noAA
  ee/sample/advanced/anti/refmap-onepass-AAline
  ee/sample/advanced/anti/texmap-onepass-AAline
  ee/sample/basic3d/vu1
  ee/sample/graphics/bumpmap
  ee/sample/graphics/clip_vu1
  ee/sample/graphics/jointm/intr_key/runtime
  ee/sample/graphics/mipmap
  ee/sample/graphics/point_l
  ee/sample/graphics/refmap
  ee/sample/graphics/spot_l
  ee/sample/graphics/tex_swap

  The microprograms in the above samples lacked a slot for an instruction 
  that is necessary between an instruction for setting branch conditions 
  and conditional branch instruction. 

- ee/sample/mc/basic 
  This sample program has been modified due to the change of libmc.h. 

- ee(iop)/sample/ilink
  Processes to unload ilink.irx and ilsock.irx modules have been added. 

- iop/sample/usb/activate/readme.txt
  A description on execution of make run has been added.

- iop/sample/usb/usbmouse
  iop/sample/usb/usbkeybd
  iop/sample/usb/selfunld

  If USB devices are connected and disconnected repeatedly when the 
  number of USB ports is more than the maximum number of connectable 
  devices set by the user, the system sometimes hung up. This problem 
  has been fixed.

-----------------------------------------------------------------------
Abbreviations
-----------------------------------------------------------------------
For legibility, the following abbreviations are used in documents 
contained in the release packages.

Hard disk drive
 Unless otherwise specified, an external hard disk drive (** GB) (for 
 PlayStation 2) with network adaptor (PC CARD type) and an internal hard 
 disk drive (** GB) (for PlayStation 2) with network adaptor are described 
 as the hard disk drive.

Network adaptor
 Unless otherwise specified, a network adaptor (for PlayStation 2) and a 
 network adaptor (PC CARD type) (for PlayStation 2) are described as the 
 network adaptor.

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
Usage Rights and Restrictions
-----------------------------------------------------------------------
All rights and restrictions regarding the use of this software are
according to the contract concluded between your company and Sony
Computer Entertainment Inc.

-----------------------------------------------------------------------
Notice on Trademarks
-----------------------------------------------------------------------
"PlayStation" and PlayStation logos are registered trademarks of Sony
Computer Entertainment Inc. 
All other company names and product names that are mentioned in this 
software are trademarks and registered trademarks of their respective 
owners.

-----------------------------------------------------------------------
Notice on Copyright
-----------------------------------------------------------------------
Copyright of this software is attributed to Sony Computer Entertainment 
Inc.
