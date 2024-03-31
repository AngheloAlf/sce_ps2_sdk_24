[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4.3
   Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample that activates categories (USB module autoloader)

<Description>

This program is sample software for the USB module autoloader. The program shows how to use the USB module autoloader (usbmload.irx).

<Specifications>

Activates categories specified by arguments.

<Files>
        activate.c     Main program

<Run method>
Execute the sample with dsidb as shown below.


	dsidb R> reset 0 0
	dsidb R> mstart /usr/local/sce/iop/modules/usbd.irx conf=2048
	dsidb R> mstart /usr/local/sce/iop/modules/usbmload.irx conffile=host1:/usr/local/sce/conf/usb/usbdrvho.cnf debug=1

At this time usbmload.irx will be loaded, although permission to activate the categories and perform an autoload will not be granted, so autoloading will not work.

	   dsidb R> mstart activate.irx Mouse Keyboard
will activate the Mouse and Keyboard categories.


Note: This is also possible by executing the following in the Linux command line.

        $ make -i run
	

