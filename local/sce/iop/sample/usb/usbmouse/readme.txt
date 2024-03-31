[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4.3
     Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                        All Rights Reserved

USB mouse sample driver (usbmload.irx compatible)

<Description>

This program is the sample driver for the USB mouse, and shows how to use the USB driver (USBD).

This driver is used by the EE-side sample which is in the following directory: sce/ee/sample/usb/usbmouse

If only this IOP-side driver is executing, the display can be checked by using dsidb.

Before starting this program, connect the USB mouse to either USB port.

<Specifications>

1. A maximum of 8 USB mice can be connected at one time.
   This value can be changed by using the MAX_MOUSE_NUM 
   macro in "usbmouse.h".
2. A ring buffer provided for each mouse assists in 
   receiving EE data without missing any parts.  
   The size of the ring buffer is defined by the 
   RINGBUF_SIZE macro in "usbmouse.h". 
3. Compatible with the USB module autoloader (usbmload.irx)
   and with the "lmode" option argument. If no argument is
   specified, the usage method will be normal. For details
   refer to the document usbmload.irx.
	4. The driver can be unloaded from another module or from the EE.

<Files>

        usbmouse.c	Main program
        usbmouse.h	Header file

<Run method>
	After dsidb is started, execute the following.

        % make                                          : Compile
        % dsidb
        > reset 0 0
        > mstart /usr/local/sce/iop/modules/usbd.irx
        > mstart usbmouse.irx

	To perform the above in one operation, enter the following.
	% make run


<Unloading method>
	First, compile /usr/local/sce/iop/sample/kernel/module/*.
	
	To unload the driver, open a separate terminal and enter the following.
	% dsistart /usr/local/sce/iop/sample/kernel/module/removemd.irx SCE_USB_MOUSE_SAMPLE
	
	To confirm that the driver has been unloaded, enter the following.
	% dsilist
	
	
	Using mlist -m under dsidb is not recommended because dsidb does not fully support unloading.
	However, if you really want to use mlist -m, you must terminate dsidb, then run dsidb -nr.


<Messages>
        
	[Immediately after a mouse is connected]
          usbmouse0: attached (port=1)
          |_______|            |____|
              A                   B

          A : Mouse number assigned dynamically
          B : Port number the USB mouse is connected to

        [Thereafter until the mouse is disconnected]
	  usbmouse0: count=1 data=( 00 0f 0f )
          |_______|  |_____|        |______|
              C         D               E

          C : Mouse number dynamically assigned when a 
  	      USB mouse is connected
          D : The amount of data received from the mouse
          E : Raw data transferred from the mouse
              byte 0    : Button
                            bit 0    : Button 1
                            bit 1    : Button 2
                            bit 2    : Button 3
                            bits 3-7 : Device-specific
              byte 1    : X (signed char)
              byte 2    : Y (signed char)
              byte 3    : Wheel (Device-specific)
              bytes 4-n : Device-specific

<Controller operation>
	None

<Sending SET_INTERFACE request>
When the value of the InterfaceDescriptor bAlternateSetting is 0 in 
this sample program, SET_INTERFACE request will not be sent during 
device initialization.

The reason for this is because a malfunction may occur (e.g. hangup) 
when a SET_INTERFACE is received by some commercial USB devices.
	
Under the USB standard, a SET_INTERFACE request must be implemented in 
a USB device (at a minimum only an error should be returned). 
Nevertheless, there are devices which are not compatible with a 
SET_INTERFACE request.

For a mouse, it is difficult to think about multiple Alternative 
Settings, so there are probably few substitute actions using this 
method.

<Notes>

For further information on the USB mouse, refer to "Human Interface Devices" distributed at the following location:

"http://www.usb.org/developers/devclass_docs.html".
