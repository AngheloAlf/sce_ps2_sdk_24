[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4.3
    Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                       All Rights Reserved
 
Isochronous transfer sample program

<Description>
This program is a sample driver that performs isochronous transfers. It is not meant to show how to use the Audio class, but it is intended for target devices satisfying the following condition:

* Devices with an Audio class as well as subclasses having an Audio Streaming PCM output interface.

<Specifications>
1. Only one device can be connected at a time.

2. Playback starts if a setting exists that matches the WAV file specified in the argument. There are no special restrictions on the WAV file's channel count, number of bits, sampling frequency, etc, but it must be compatible with the device for playback. The most trouble-free setup is 16 bits/sample, 2 channels (stereo), and 44.1 KHz.

3. The WAV file should be small enough to fit in IOP memory, generally about 1 MB.

4. The sample will allocate a buffer in 8-second increments and read ahead in 1-second increments. Transfer requests for up to 32 frames of data will be issued simultaneously.

5. By default, data transfers use sceUsbdMultiIsochronousTransfer(). If at the start you change

	#define USE_MULTI_ISOCH_TRANSFERS
to
	#undef USE_MULTI_ISOCH_TRANSFERS

in the header, then transfers will use sceUsbdIsochronousTransfer().

6. This sample does not support the USB module auto-loader (usbmload.irx).

7. This sample does not support Unload from the EE or from other modules.

<Files>
	audio.h		Definition header for USB audio class
	usbspkr.c	Main program

<Run Method>
Use the following procedure to start dsidb:
	$ make
	$ dsidb
	dsidb R> reset
	dsidb R> mstart /usr/local/sce/iop/modules/usbd.irx conf=1024
	dsidb R> mstart usbspkr.irx host1:XXX.wav

(The file "XXX.wav" should be replaced with the actual name of the WAV file)

<Controller Operation>
None

<Notes>
Generally speaking, compared to other devices, audio devices require a relatively large number of static descriptors. For this reason, rather than using the USBD defaults asis, you should explicitly allow for this from the start, such as setting conf=1024.
