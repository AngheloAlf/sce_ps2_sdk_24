[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                     All Rights Reserved

                                       Sample Structure
=========================================================

Sample Directory Structure
---------------------------------------------------------
Samples marked with an "(*)" are new in this release.

sce/iop/sample/
|--cdvd
|   |--smp_iop
|   |--stmadpcm
|   |--stmread
|   +--stmspcm
|--hello
|--hdd
|   |--basic
|   +--shell
|--ilink
|--inet
|   |--ftp		(*)
|   |--http_get
|   |--libnet
|   |--load_test
|   |--setapp
|   |--inetlog
|   |--modemapi
|   |--netstat
|   |--ntguicnf
|   |   |--setinit	(*)
|   |   +--usbinit	(*)
|   +--ping
|--kernel
|   |--hardtime	
|   |--module
|   +--thread
|--sif
|   |--sifcmd
|   +--sifrpc
|--sound
|   |--bypass16
|   |--ezadpcm
|   |--ezbgm
|   |--ezmidi
|   |--sebasic
|   |--semidi
|   |--sesqhard
|   |--sqhard
|   |--sqsoft
|   |--sqsong		(*)
|   |--voice
|   +--enadpcm
|--OLD
|   +--spu2
|       |--autodma
|       |--seqplay
|       |--stream
|       +--voice
+--usb
    |--activate	
    |--selfunld		(*)
    |--usbdesc
    |--usbkeybd
    |--usbloadf	
    +--usbmouse


Sample Index
---------------------------------------------------------------
Sample codes are shown below.

cdvd:
	cdvd/smp_iop	Sample to call the CD/DVD-ROM Drive command
			function on the IOP.
        cdvd/stmadpcm	Sample to reproduce the ADPCM streaming
        cdvd/stmread	Sample to read the file using the stream 
                        functions from a CD/DVD drive.
        cdvd/stmspcm	Sample to reproduce the straight PCM streaming

hello:
	hello		Sample to display "hello !" 

hdd:
	hdd/basic	Sample to operate HDD files
	hdd/shell	Sample to operate HDD partitions

ilink:
	ilink		Sample for the communication via i.LINK(IEEE1394)

inet:
	inet/ftp	A sample for communicating with remote host via 
			File Transfer Protocol ( FTP ) using inet library
	inet/http_get	Sample Program for Obtaining a Remote File Using 
			the Http Protocol in the INET Library
	inet/libnet	Libnet Library Sample Program for Transparent 
			Handling of the INET Library in EE (IOP program)
	inet/load_test	Sample for sending/receiving packets between
			the client and server using TCP
	inet/setapp     Network setting application samples
        inet/inetlog	Sample Program for Displaying/Saving Log Messages
                        from INET library
        inet/modemapi   sample of API which directly operates a modem
                        from an application
        inet/netstat	Sample Program for Displaying Connection Information
                        in INET Library
	inet/ntguicnf/setinit	Sample Program for Network Configuration GUI 
				Library: inet Configuration Initialization 
				Module	
	inet/ntguicnf/usbinit	Sample Program for Network Configuration GUI 
				Library: USB Autoloader Starter
        inet/ping	Sample Program for Confirming Whether Data Has 
                        Reached the Host Using the INET Library

kernel:
	kernel/hardtime	Sample using hardware timer functions
	kernel/module   Sample to show how to create a resident library
			module
	kernel/thread	Sample to generate a thread and boot-up, 
			Operate a thread priority, Synchronize threads

sif:
	sif/sifcmd	SIF CMD protocol sample
	sif/sifrpc	SIF RPC protocol sample

sound:
	sound/bypass16	A sample for waveform data streaming via bypass 
			processing
	sound/ezadpcm	Sample to play back BGM with ADPCM data streaming 
			from the disk
	sound/ezbgm	Sample to reproduce the sound data (WAV) with 
			streaming from the disk
	sound/ezmidi	Sample to create music and sound effects with MIDI
	sound/sebasic	SE playback with hardware synthesizer
	sound/semidi	SE/MIDI playback with hardware synthesizer sample
	sound/sesqhard	Sample for SE sequence playback with 
			hardware synthesizer
	sound/sqhard	Sample to play the MIDI by the hardware synthesizer
	sound/sqsoft	Sample to play the MIDI by the software synthesizer
	sound/sqsong	Sample Program for Playing Back SONG via Hardware
			Synthesizer
	sound/voice	Sample to play the voice
        sound/enadpcm   sample of API which directly operates a modem from
                        an application
OLD/spu2:
	(Sample using libspu2)
	spu2/autodma	Sample to reproduce the sound by the straight PCM 
			input using the	AutoDMA transfer function of the
			Interim sound library (libspu2)
	spu2/seqplay	Sample to reproduce the MIDI sequence on the IOP
			using the Interim sound library(libspu2, 
			libsnd2)
	spu2/stream	Sample to reproduce the voice stream by the SPU2 
			on the IOP using the Interim sound library
			(libspu2)
	spu2/voice	Sample to reproduce the SPU2 voice on the IOP using
			the Interim sound library(libspu2)

usb:
	usb/activate	Sample program which activates categories 
			(for USB module auto-loader)
	usb/selfunld	A sample program enabling self-deletion of the USB 
			device driver
	usb/usbdesc	Sample to dump the static descriptor of the USB 
			device.
	usb/usbkeybd	Sample driver of the USB keyboard.
	usb/usbloadf	Sample which enters a module-loading function 
			in USB module auto-loader
	usb/usbmouse	Sample presenting how to use USB driver (USBD)


Preliminary Arrangements for Sample Compilation   
------------------------------------------------
None
