[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4
    Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

Sample program: Dumping the descriptor of a USB device

<Description>

        This sample program shows how to dump USB device descriptors.
	The following descriptors are dumped.
	
	1. Device Descriptor
	2. Configuration Descriptor
	3. Interface Descriptor
	4. Class Specific Descriptor (HID etc.)
	5. Report Descriptor
	6. Endpoint Descriptor
	
	(*) String Descriptors are not supported.

<Files>

        usbdesc.c	Main program (dumps standard descriptors)
	hid.c		Dumps HID and REPORT descriptors

<Usage>

1. Make sure that nothing is connected to the PlayStation 2 USB port.
2. $ make run
3. Plug the USB device for which the descriptor is to be dumped into the PlayStation 2.
   The following message will be displayed on the console.

    New device plug-in!!!
    DEVICE-ID:2
    Dump end

4. Look at the files in the current directory.
   "dev_id2.dsc" is the device descriptor file.

    dsidb R> quit
    $ ls
    Makefile      dev_id2.dsc   readme_j.txt  usbdesc.irx
    PathDefs      readme.txt  usbdesc.c     usbdesc.o
    $ 

5. If the portion of the Makefile corresponding to the "run:" label is rewritten as follows, output can also be sent to stdout of the debugger.
   (*)  A hyphen (-) has been added in the fourth line.

   run:	$(PROGNAME)
	dsreset 0 0 ;\
	dsistart /usr/local/sce/iop/modules/usbd.irx conf=2048 reportd=1;\
	dsistart $(PROGNAME) -
	dsidb -nr

