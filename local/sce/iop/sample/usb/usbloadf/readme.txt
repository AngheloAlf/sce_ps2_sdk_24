[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample that registers module load functions in USB module autoloader

<Description>
This program (usbloadf.irx) is sample software for the USB module autoloader. The program shows how to use the USB module autoloader (usbmload.irx). 

<Specifications>
When the sample program is executed, the following processes occur.
	
	1. Register module load function.
        2. Register driver information.
        3. Activate Keyboard and Mouse categories.
        4. Enable autoloading.
        5. Exit resident.

<Files>

        usbloadf.c     Main program

        Under /usr/local/sce/conf/usb/
	usbdrvho.cnf  Driver database files (reads drivers from HOST)
        usbdrvcd.cnf  Driver database files (reads drivers from CD)

<Notes on execution>
1. Make must be performed on sample drivers of USB keyboards and mice on the IOP side (located under these directories).
           /usr/local/sce/iop/sample/usb/usbkeybd
           /usr/local/sce/iop/sample/usb/usbmouse

2. The following files must be burned into the root directory of a CD-R when testing driver loading from the CD-R.
           /usr/local/sce/iop/sample/usb/usbkeybd/usbkeybd.irx
           /usr/local/sce/iop/sample/usb/usbmouse/usbmouse.irx
           /usr/local/sce/conf/usb/usbdrvho.cnf
           /usr/local/sce/conf/usb/usbdrvcd.cnf
* Be sure to burn approximately 100 MB of dummy data on the periphery of the CD-R.

3. Compile the sample.
           % make     :Compile

<Run method>
There are eight execution patterns.
This is because usbmload.irx returns a non-zero value to support Unload even if the -i option is specified.
	
1. When specifying a configuration file as an argument of usbmload.irx.
	% make -i run00 (configuration file:HOST , driver:HOST)
        % make -i run01 (configuration file:HOST , driver:CD)

2. When specifying a configuration file using the API (sceUsbmlLoadConffile) of usbmload.irx.

           % make -i run10 (configuration file:HOST , driver:HOST)
           % make -i run11 (configuration file:HOST , driver:CD)
           % make -i run12 (configuration file:CD   , driver:HOST)
           % make -i run13 (configuration file:CD   , driver:CD)

3. Without using a configuration file, using only the API (sceUsbmlRegisterDevice) of usbmload.irx to perform registration.

           % make -i run20 (driver:HOST)
           % make -i run21 (driver:CD)

<Unloading method>
	[External unload]
	First, compile /usr/local/sce/iop/sample/kernel/module/*.
	
	To unload the driver, open a separate terminal and enter the following.
	% dsistart /usr/local/sce/iop/sample/kernel/module/removemd.irx USB_load_function_sample
	
	[Verify unload]
	To verify that the driver can be unloaded, execute the following.
	% dsilist
	
	Using mlist -m under dsidb is not recommended because dsidb does not fully support unloading.
	To use mlist -m, you must terminate dsidb, then run dsidb -nr.
	
<Notes>
	None
