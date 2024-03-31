[SCEI CONFIDENTIAL DOCUMENT]
"PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
                Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                                   All Rights Reserved
                                                             Nov. 2001

======================================================================
The following libraries and modules are released provisionally as 
Release2.4.2.
Please replace the previous version with this release.

========================================================================
Precaution
========================================================================
- If you develop program using Release2.4.2 libraries, be sure to use 
  FLASH t10000-rel242.bin or replace IOP default module by using 
  ioprp242.img.
  Please refer to SIF system (sif.pdf) for details of replacing the modules.

<Caution on using Release2.4.2 Flash>
  The following message will appear after the program finishes, however, 
  it does not affect the program.
  
  "# TLB spad=0 kernel=1:12 default=13:36 extended=37:47"
    
<About an986.irx>
- Release2.4.1 is currently being released and can be downloaded from 
  the web site.
  As an986.irx from the previous version does not operate correctly on 
  Release2.4.2 package, download the latest version before using.

- Since the use of an986.irx in titles is prohibited,
  an986.irx is not included in the release package 
  (under sce/iop/modules). an986.irx can be downloaded from the web site.
 
- To use USB modem drivers, download the drivers provided by Tools & 
  Middleware licensees the web site.

<About ee/sample/graphics/hig sample>
- In the files of sample10.c and sample11.c, old macro definition, 
  *_PROCESS has been changed to new macro definition, SCE_HIG_*_PROCESS.
  For compatibility, both macros are used for the library header file 
  (libhig.h) in this release. However, be sure to use the new macro 
  eventually as the old macro definition *_PROCESS will be deleted from 
  the next release.

========================================================================
Restrictions
========================================================================
<Low level sound library (libsd) data transfer>
- With sceSdBlockTrans(), if the buffer size is specified for 
  SD_BLOCK_LOOPby the unit of 1024 bytes , the sound may not be played 
  back correctly. For SD_BLOCK_LOOP, the buffer size must be specified  
  by the unit of 2048 bytes.

- A modification has been made; with sceSdVoiceTrans(), if SD_TRANS_BY_IO 
  is specified for the transfer device, the transfer can be performed by 
  the unit of 64 bytes.
  When both of SD_TRANS_BY_DMA and SD_TRANS_BY_IO are specified, data can 
  be transferred by the unit of 64 bytes (rounded up to 64-byte multiple).

  Due to this modification, the existing data in local memory may be 
  overwritten depending on the size of the transferred data as the 
  addresses of SPU2 local memory are 16-byte multiples.

  Also, note that calculation of the SPU2 local memory usage rate must be 
  done by the unit of 64 bytes (rounded up to 64-byte multiple).
  

The contents and changes of the package are shown below:
----------------------------------------------------------------------
Contents of this package
----------------------------------------------------------------------
tlib_242
/
|---common
|  +---include
|     |---cslse.h
|     |---libcdvd.h
|     |---libver.h
|     |---modhsyn.h
|     |---modsein.h
|     |---sdmacro.h
|     +---sk
|        |---common.h
|        +---errno.h
|---conf
|  +---usb
|     +---usbdrvho.cnf
|---ee
|  |---include
|  |  |---libhig.h
|  |  |---libhip.h
|  |  |---libhttp
|  |  |  |---http_methods.h
|  |  |  |---http_options.h
|  |  |  +---http_status.h
|  |  |---libhttp.h
|  |  |---libinsck
|  |  |  |---arpa
|  |  |  |  +---inet.h
|  |  |  |---netdb.h
|  |  |  |---netinet
|  |  |  |  |---in.h
|  |  |  |  +---tcp.h
|  |  |  +---sys
|  |  |     +---socket.h
|  |  |---libinsck.h
|  |  |---libmpeg.h
|  |  |---libnet.h
|  |  |---libsein.h
|  |  |---netglue
|  |  |  |---arpa
|  |  |  |  +---inet.h
|  |  |  |---netdb.h
|  |  |  |---netinet
|  |  |  |  |---in.h
|  |  |  |  +---tcp.h
|  |  |  +---sys
|  |  |     +---socket.h
|  |  |---netglue.h
|  |  |---ntguicnf.h
|  |  |---sifdev.h
|  |  +---sk
|  |     |---sk.h
|  |     +---sound.h
|  |---lib
|  |  |---libcdvd.a
|  |  |---libhig.a
|  |  |---libhip.a
|  |  |---libhttp.a
|  |  |---libinsck.a
|  |  |---libkernl.a
|  |  |---libmpeg.a
|  |  |---libnet.a
|  |  |---libsein.a
|  |  |---libsk.a
|  |  |---netglue_insck.a
|  |  |---ntgui_e.a
|  |  +---ntgui_j.a
|  |---sample
|  |  |---graphics
|  |  |  +---hig
|  |  |---inet
|  |  |  |---ball_game
|  |  |  |---echo_server
|  |  |  |---http_get
|  |  |  |---libhttp
|  |  |  |  |---auth
|  |  |  |  |---base64
|  |  |  |  |---blocking
|  |  |  |  |---chunk
|  |  |  |  |---cookie
|  |  |  |  |---mime
|  |  |  |  |---normal
|  |  |  |  |---proxy
|  |  |  |  |---qp
|  |  |  |  |---redirect
|  |  |  |  +---urlesc
|  |  |  |---libnet
|  |  |  |---load_test
|  |  |  |  |---client
|  |  |  |  +---daemon
|  |  |  |---ntguicnf
|  |  |  |---setapp
|  |  |  +---socket
|  |  |     |---echo_server
|  |  |     +---http_get
|  |  +---mpeg
|  |     +---ezmpegstr
|  +---src
|     +---lib
|        +---hip
|---iop
|  |---install
|  |  |---include
|  |  |  |---inet
|  |  |  |  |---arp.h
|  |  |  |  |---dhcp.h
|  |  |  |  |---dns.h
|  |  |  |  |---ether.h
|  |  |  |  |---icmp.h
|  |  |  |  |---in.h
|  |  |  |  |---inet.h
|  |  |  |  |---inetctl.h
|  |  |  |  |---ip.h
|  |  |  |  |---modem.h
|  |  |  |  |---netdev.h
|  |  |  |  |---pppctl.h
|  |  |  |  |---tcp.h
|  |  |  |  +---udp.h
|  |  |  |---libsd.h
|  |  |  |---modsesq.h
|  |  |  +---netcnf.h
|  |  +---lib
|  |     |---cdvdman.ilb
|  |     |---inet.ilb
|  |     |---inetctl.ilb
|  |     |---libsd.ilb
|  |     |---modhsyn.ilb
|  |     |---modsein.ilb
|  |     |---modsesq.ilb
|  |     +---netcnf.ilb
|  |---modules
|  |  |---atad.irx
|  |  |---dev9.irx
|  |  |---hdd.irx
|  |  |---inet.irx
|  |  |---inetctl.irx
|  |  |---ioprp242.img
|  |  |---libnet.irx
|  |  |---libsd.irx
|  |  |---modhsyn.irx
|  |  |---modsein.irx
|  |  |---modsesq.irx
|  |  |---netcnf.irx
|  |  |---ntguicnf.irx
|  |  |---pfs.irx
|  |  |---ppp.irx
|  |  |---skhsynth.irx
|  |  |---skmidi.irx
|  |  |---skmsin.irx
|  |  |---sksesq.irx
|  |  |---sksound.irx
|  |  +---smap.irx
|  |---sample
|  |  +---inet
|  |     |---libnet
|  |     |---ntguicnf
|  |     |  +---setinit
|  |     +---setapp
|  +---util
|     +---inet
|        +---ifconfig.irx
|---t10000-rel242.bin

-----------------------------------------------------------------------
New additions
-----------------------------------------------------------------------
- HTTP library (libhttp.a) have been added.

- Network wrapper API (netglue_insck.a) for inet has been added.

- Following sample programs of the HTTP library have been added.

  ee/sample/inet/libhttp/auth      Sample of authentication processing
  ee/sample/inet/libhttp/base64    Sample of BASE64 processing
  ee/sample/inet/libhttp/blocking  Blocking version of http_test 
  ee/sample/inet/libhttp/chunk     Sample of chunk transfer processing
  ee/sample/inet/libhttp/cookie    Sample of cookie processing
  ee/sample/inet/libhttp/mime      Gets a specified URI and performs 
				   MIME processing  
  ee/sample/inet/libhttp/normal    Performs GET, HEAD, or POST for a 
				   specified URI  			    
  ee/sample/inet/libhttp/proxy     Gets a specified URI via proxy
  ee/sample/inet/libhttp/qp        Sample of quoted-printable processing
  ee/sample/inet/libhttp/redirect  Sample of redirection processing
  ee/sample/inet/libhttp/urlesc    Sample of URL escape and unescape 
				   processing
    				     
-----------------------------------------------------------------------
Changes on library module
-----------------------------------------------------------------------
Flash
------------------------------------------------------------------------
- An error handling (retry processing) has been added to sceSifSendCmd() 
  function.

- When executing standard input-output in EE, an error handling failure 
  occurred in allocating memory and generating thread in IOP. 
  This problem has been fixed.

-----------------------------------------------------------------------
CD(DVD)-ROM library
-----------------------------------------------------------------------
- A modification has been made; if sceCdStSeekF() function is called 
  when the tray is open, an error will be returned.

-----------------------------------------------------------------------
dev9 module
-----------------------------------------------------------------------
- DMA interrupt setting was changed incorrectly. 
  This problem has been fixed.

-----------------------------------------------------------------------
atad module
-----------------------------------------------------------------------
- There was a failure in recovery processing when timeout occurs. 
  This problem has been fixed.

-----------------------------------------------------------------------
High level graphics library
-----------------------------------------------------------------------
- With sceHiDMAMake_LoadGSLump(), internal buffer was not allocated 
  correctly. This problem has been fixed.

- From this release, the ID conflict problem has been resolved by 
  implementing ID management feature to DMA service.

-----------------------------------------------------------------------
High level graphics plugin library
-----------------------------------------------------------------------
- sceHiPlugShapeMasterChainSetting() function which changes Shape packet 
  string setting has been added.

- With Tim2 plugin, some texflush instructions were not inserted in the 
  packet. This problem has been fixed.

- With Tim2 plugin, an extra code that created a packet, which might 
  cause Gif to stop, has been deleted.

<Change of libhip.h>
- SCE_HIP_SHAPE_MASTER_CHAIN_IN_STATIC_O flag constant has been added.

-----------------------------------------------------------------------
MPEG library
-----------------------------------------------------------------------
- A buffer for writing DMAref tag often did not become 64byte alignment. 
  This problem has been fixed.

-----------------------------------------------------------------------
Standard kit library/sound system
-----------------------------------------------------------------------
- The following functions have been added.
  sceSkSsVoiceSetPitchBend()   Adds pitch bend effect to a voice 
			       generating
  sceSkSsVoiceSetExpression()  Specifies expression to a single tone

-----------------------------------------------------------------------
Network configuration GUI library (ntgui_j.a/ntgui_e.a/ntguicnf.irx)
-----------------------------------------------------------------------
- At the connection test, even if the telephone number [1-3] was set, 
  the number could not be redialed. 
  This problem has been fixed. 

- During the test, the number of the connection destination now can be 
  displayed.

- A modification has been made; an appropriate message is output when 
  IOERROR was detected.

-----------------------------------------------------------------------
Network socket library
-----------------------------------------------------------------------
- Due to the change of libnet, the library has been rebuilt.

<Change of libinsck/arpa/inet.h>
- #define inet_lnaof  sceInsockInetLnatof has been changed to  
  #define inet_lnaof  sceInsockInetLnaof.

-----------------------------------------------------------------------
Network library/module
-----------------------------------------------------------------------
- With TCP in resend processing, the receipt of small delayed ACK 
  caused an abnormal increase of the number of resending times. 
  This problem has been fixed. 

- A log feature of send/receive packets to PPP has been added.

- Resend was performed even when ACK has been received normally. 
  This problem has been fixed. 

- A modification has been made for a process NOT to discard RST packets, 
  which has invalid SEQ numbers.

- A process that verifies other NETDEV interface version numbers except 
  for version No. 2 as error and that rejects such connections has 
  been added.

-----------------------------------------------------------------------
Network configuration library/module
-----------------------------------------------------------------------
- Redial process could not be performed normally. 
  This problem has been fixed. 

- sceInetCtlGetState() function for getting the transition state of the 
  interface has been added. This function enables us to verify if the 
  redial is in process in PPP.

-----------------------------------------------------------------------
Common network configuration library/module
-----------------------------------------------------------------------
- With sceNetCnfAddEntry() and sceNetCnfEditEntry(), a load argument 
  option (-no_check_capacity) of netcnf.irx, which invalidates capacity 
  check has been added.
  
- sceNetCnfCheckCapacity() function for checking the remaining capacity 
  has been added.

-----------------------------------------------------------------------
PPP module
-----------------------------------------------------------------------
- Redial process could not be performed correctly. 
  This problem has been fixed. 

- When a submission is performed with excess load for the line transfer 
  rate, the packets piled up. This problem has been fixed. 

-----------------------------------------------------------------------
ifconfig.irx utility
-----------------------------------------------------------------------
- For case when the interface of the modem driver is serial interface 
  for PPP connection, the following processes for representing the 
  number of errors have been added.
     overrun-error
     parity-error
     framing-error
     buffer-overrun

-----------------------------------------------------------------------
USB module auto loader configuration file
-----------------------------------------------------------------------
- conf/usb/usbdrvho.cnf
  A list of the following devices newly added in the an986.irx 
  (Release 2.4.1) module has been added.
  
	Elecom		LD-USBL/TX
	PLANEX		UE-200TX
	Melco		LUA2-TX
	Linksys		USB100TX B
	D-Link		DU-E100 B1

-----------------------------------------------------------------------
Low level sound library
-----------------------------------------------------------------------
- printf() contained in the library has been deleted.

- sceSdStopTrans() function that stops transfer process to SPU2 local 
  memory has been added.

- A modification has been made to the following functions to enable us 
  to verify cause of errors from the return values when terminated with 
  error.

  sceSdBlockTrans(), sceSdClearEffectWorkArea(), sceSdInit(),
  sceSdSetEffectAttr(), sceSdVoiceTrans(), and sceSdVoiceTransStatus(),

- With sceSdProcBatch() and sceSdProcBatchEx() functions, if SD_WRITE_EE 
  and SD_RETURN_EE were specified for the command, the data could not be 
  transferred correctly. This problem has been fixed.

- With sceSdBlockTrans() function, the following problems have been 
  resolved:

    * With a transfer channel in process of transferring, when an attempt 
      had been made to perform a transfer start processing (regardless of 
      WRITE/READ/WRITE_FROM) to the transferring channel by using 
      sceSdBlockTrans(), a negative value was returned to notify that the 
      channel was already in transferring process, which caused the 
      transfer state to be irregular.
  
    * When SD_TRANS_MODE_STOP was specified, the transfer did not stop 
      correctly.
  
    * When SD_TRANS_MODE_STOP was specified, the bit that represents a 
      buffer in transferring process could not be set in the return value 
      correctly.

- With sceSdBlockTransStatus(), the bit that represents a buffer in 
  transferring process could not be set in the return value correctly. 
  This problem has been fixed.

- With sceSdGetEffectAttr(), when each EVOL had been changed by using 
  sceSdSetEffectAttr() and sceSdSetParam(), depth_L and depth_R were 
  not returned correctly. This problem has been fixed.

- With sceSdVoiceTransStatus(), even if SD_TRANS_STATUS_WAIT had been 
  specified, multiple threads could not be waited. 
  This problem has been fixed.

- With sceSdBlockTrans() and sceSdVoiceTrans(), when SD_TRANS_MODE_READ
  was specified, the data that had been read from SPU2 local memory 
  could not be read correctly. 
  This problem has been fixed.

- A modification has been made; with sceSdVoiceTrans(), if SD_TRANS_BY_IO 
  is specified for the transfer device, the transfer can be performed by 
  the unit of 64 bytes.
  When both of SD_TRANS_BY_DMA and SD_TRANS_BY_IO are specified, data can 
  be transferred by the unit of 64 bytes (rounded up to 64-byte multiple).
    
-----------------------------------------------------------------------
CSL sound effect stream generation (libsein/modsein)
-----------------------------------------------------------------------
- The following functions for generating "SE message: Note on status" 
  have been added.
   sceSEIn_MakeNoteOnZero()   Writes note on message in output port 
			      buffer
   sceSEIn_MakePitchOnZero()  Writes note on message (pitch specified) 
			      in output port buffer

<Change of cslse.h>
- sceSEMsg_STATUS_NOTE0 which represents "SE message: Note on status"
  has been added.

-----------------------------------------------------------------------
CSL sound effect sequencer (modsesq)
-----------------------------------------------------------------------
- "SE message: Note on status (0x9n)" in SQ data has been supported.

-----------------------------------------------------------------------
CSL hardware synthesizer (modhsyn)
-----------------------------------------------------------------------
- "SE message: Note on status (0x9n)" has been supported.

-----------------------------------------------------------------------
Changes of samples
-----------------------------------------------------------------------
ee/sample/inet/setapp
iop/sample/inet/setapp
- At the connection test, even if the telephone number [1-3] was set, 
  the number could not be redialed. This problem has been fixed.

ee/sample/mpeg/ezmpegstr
- With clearBackground() in playpss.c, a display list could not be 
  created correctly. This problem has been fixed. 

ee/sample/graphics/hig
- In the files of sample10.c and sample11.c, old macro definition, 
  *_PROCESS has been changed to new macro definition, SCE_HIG_*_PROCESS.
  For compatibility, both macros are used for the library header file 
  (libhig.h) in this release. However, be sure to use the new macro 
  eventually as the old macro definition *_PROCESS will be deleted from 
  the next release.

ee/sample/graphics/hig/data
- With shadowmapmap1.s, the texture function is the data, which would 
  not be affected by a light source. Thereby, DECAL has been modified 
  to MODULATE.

- If data/*.s was converted to *.bin by using ee-objcopy in Tool Chain 
  EE 2.96-ee-001003, an invalid binary with the first several bytes
  cleared to 0 has been generated. This problem has been fixed.   

iop/sample/inet/ntguicnf/setinit
- In network configuration GUI library, when an attempt was made to 
  connect by returning to the sample after selecting the telephone 
  number [1-3], the number could not be redialed. 
  This problem has been fixed. 

ee/sample/inet/libnet
iop/sample/inet/libnet
- The following functions have been added for "up" and "down" of the 
  interface.
     up_interface()
     down_interface()
     load_set_conf_only()
     wait_get_addr_only()
     get_interface_id()

- extern C has been added to the declaration that had no "extern C."

ee/sample/inet/echo_server
ee/sample/inet/http_get
ee/sample/inet/ball_game
ee/sample/inet/load_test/client
ee/sample/inet/load_test/daemon
ee/sample/inet/socket/echo_server
ee/sample/inet/socket/http_get

- A feature that brings the interface "down" was added to the above 
  programs. Also, initialization and termination methods for libnet have
  been standardized.
  
-----------------------------------------------------------------------
Changes of documents
-----------------------------------------------------------------------
<Network configuration application creation guideline (setapp_r)>
- Precautions for the use of a redial process for connection have been 
  added.
- Precautions for the use of a connection process have been added in case
  there is a difference between the types of the  hardware configuration 
  file and the network service provider configuration file.

<CSL sound effect stream generation (esein_rf/isein_rf)>  
- Description for the following functions that generate "SE message: Note
  on status" has been added. 
     sceSEIn_MakeNoteOnZero()
     sceSEIn_MakePitchOnZero()

- An explanation on the use of sound effect timbre chunk has been added 
  to the description in the "Argument" sections for the following functions.
     sceSEIn_MakeAmpLFO()
     sceSEIn_MakeNoteOn()
     sceSEIn_MakePitchLFO()
     sceSEIn_MakePitchOn()
     sceSEIn_MakeTimePanpot()
     sceSEIn_MakeTimePitch()
     sceSEIn_MakeTimeVolume()
     sceSEIn_NoteOn()
     sceSEIn_PitchOn()

<CSL overview>
- Note on status (0x9n) description has been added to the SE message.

<Standard kit library/sound system (sk_rf)>
- Description for the following functions has been added.
    sceSkSsVoiceSetPitchBend()
    sceSkSsVoiceSetExpression()

<High level graphics plugin library (libhip)>
- Description for Tim2 plugin has been added to "1. Library Overview."

- "Tim2 Plugin" has been added to "2. Feature Overview for Each Plugin."

<High level graphics plugin library (hip_rf)>
- Description for sceHiPlugShapeMasterChainSetting() has been added.

<Higl level graphics service funcitons (higserv)>
- Descripiton for packet buffer has been changed in "Consumed memory,"  
  "3. DMA Service Functions."

- The following items have been added in "3. DMA Service Functions."
      Dynamic/Static
      sceHiDMAWait
      Double Buffering

- Description regarding "ID problem: ID conflict restrictions" has been 
  removed from "Restrictions" in "3. DMA Service Functions."

<Network configuration GUI library (ntgui_rf)>
- An explanation has been added to peer_name member of 
  sceNetGuiCnfEnvData structure.

- Description of the name of sceNetGuiCnf_SendSoftKBMessage() function 
  has been modified to sceNetGuiCnf_SendKBMessage().

<Low level sound library (sd_rf)>
- "Calling conditions" for the respective functions have been revised.
  Refer to "Calling conditons" and "Notes" sections of the function
  description for details.

- Description for sceSdStopTrans() function that stops transfer 
  processing has been added.

- Description of return values including the value in case of error 
  for the following functions has been modified.
   
   sceSdBlockTrans()
   sceSdClearEffectWorkArea()
   sceSdInit()
   sceSdSetEffectAttr()
   sceSdVoiceTransStatus()
   sceSdVoiceTrans()
   sceSdGetSpu2IntrHandlerArgument()
   sceSdGetTransIntrHandlerArgument()
   sceSdSetSpu2IntrHandler()
   sceSdSetTransIntrHandler()

- Description of transfer channel has been added to the following 
  funcitons.
   sceSdBlockTrans()
   sceSdVoiceTrans()

- An explanation on a transfer device has been added to sceSdVoiceTrans()
  function.

<Common network configuration library (netcnf)>
- Description of -no_check_capacity option of a load argument has been 
  added to the activating method for "netcnf.irx" in "2. NET configuration 
  file overview." 

<Common network configuration library (netcnf_rf)>
- Description for sceNetCnfCheckCapacity() function has been added.

-----------------------------------------------------------------------
Abbreviations
-----------------------------------------------------------------------
For legibility, the following abbreviations are used in documents 
contained in the release packages.

Hard disk drive
 Unless specifically referred to otherwise, an external hard disk drive 
 (** GB) (for PlayStation 2) with network adaptor (PC CARD type) and an 
 internal hard disk drive (** GB) (for PlayStation 2) with network 
 adaptor are described as the hard disk drive.

Network adaptor
 A network adaptor (for PlayStation 2) and a network adaptor (PC CARD 
 type) (for PlayStation 2) are described as the network adaptor.

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

RSA(R) BSAFE(TM) SSL-C software from RSA Security Inc. has been
installed. RSA and BSAFE are registered trademarks of RSA Security Inc. 
in the United States and other countries.

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


