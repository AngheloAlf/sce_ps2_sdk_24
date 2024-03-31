[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4.3
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing how a USB Device Driver can unload itself (usbmload.irx compatible)

<Description>
This program shows how a USB device driver (LDD) can unload itself. It is based on the USB mouse sample driver.
	
The most significant differences from the USB mouse sample driver are that the driver can unload itself, and that the RPC server has been removed.

With the current RPC specification, making an LDD with an RPC server unload itself requires a great deal of effort. To implement this in spite of the difficulty, an RPC server must also be implemented on the EE, that the LDD will unload itself must be reported, then the driver should unload itself only after receiving permission from the EE. However, it is doubtful whether the result will match the effort required.
	
Since this sample has no RPC server, it has no practical use as a USB mouse driver. However, it can be used as an example of an LDD that does not have an RPC server, such as a USB modem.
	
<Specifications>
Since this sample basically was created by modifying /usr/local/sce/iop/sample/usb/usbmouse/*, the specifications are similar.
	

1. Up to eight USB mice can be connected at the same time.
This value can be changed by using the MAX_MOUSE_NUM macro of "selfunld.h".
2. The USB module autoloader (usbmload.irx) is supported.
The "lmode" option is supported as an argument. If no argument is specified, the normal usage procedure occurs. For details, refer to the usbmload.irx document.
3. The driver supports the ability to unload itself. That is, the "selfunload" option of the argument is supported. For details, refer to the usbmload.irx document.
4. The driver can be unloaded from another module or from the EE.

<Files>
        selfunld.c  Main program
        selfunld.h  Header file
        run.bat     Execution batch file

<Run method>
First, compile /iop/sample/usb/activate/*.
	
        After dsidb is started, execute the following.
        % make
        % dsidb
        > reset 0 0
        > source run.bat

<Unloading method>
[Unload driver]
The sample is loaded when at one or more mice are connected to the PlayStation 2.
The sample is unloaded when all mice are detached from the PlayStation 2.
	
	[External unload]
	First, compile /usr/local/sce/iop/sample/kernel/module/*.
	
	To unload the driver, open a separate terminal and enter the following.
	% dsistart /usr/local/sce/iop/sample/kernel/module/removemd.irx SCE_USB_SelfUnload_sample
	
	[Verify unload]
	To verify that the driver can be unloaded, execute the following.
	% dsilist
	
		
Using mlist -m under dsidb is not recommended because dsidb does not fully support unloading.
However, if you do use mlist -m, you must terminate dsidb, then run dsidb -nr.
	
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
          D: Number of times data was received from the mouse.
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

The USB standard states that the SET_INTERFACE request must be implemented for a USB device (minimally, an implementation that just returns an error is required). However, note that there are some devices that do not support the SET_INTERFACE request.
	
Since a mouse does not necessarily have multiple Alternative Settings, there should be few if any side effects.

<Remarks>
For more detailed information about the USB mouse, refer to "Human Interface Devices," which is distributed through the URL "http://www.usb.org/developers/devclass_docs.html".

