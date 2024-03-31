[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4.3
    Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

USB keyboard sample driver (usbmload.irx compatible)

<Description>

This program is a USB keyboard sample driver and shows how to use a USB driver (USBD).
	
This driver is used by an EE-side sample program located in the following directory:

	sce/ee/sample/usb/usbkeybd

Also, if only this IOP-side driver is executed, you can confirm the display using dsidb.

<Precautions>

usbkeybd.irx cannot be used at the same time as the USB keyboard library (usbkb.irx).

<Specifications>

1. Up to eight USB keyboards can be connected at the same time. This value can be changed by using the MAX_KEYBD_NUM macro of "usbkeybd.h".

2. Since a ring buffer is provided for each keyboard, destruction of EE-side data can be prevented. The ring buffer size is defined by using the RINGBUF_SIZE macro of "usbkeybd.h".

3. LED control is performed by this sample driver. The specifications for lighting the CAPS LED provide the following two methods.
a. The CAPS-LED lights up when the CAPS key is pressed as on a Macintosh.
b. The CAPS-LED lights up when the CAPS + Shift keys are pressed as on a Windows machine. This method is defined by using the CAPS_LED_TYPE macro of "usbkeybd.h".

4. Compatible with the USB module autoloader (usbmload.irx) and with the "lmode" option argument. If no argument is specified, the usage method will be normal. For details refer to the document usbmload.irx.

5. Does not support Unloading.
Refer to /usr/local/sce/iop/sample/usb/usbmouse/ and
/usr/local/sce/iop/sample/usb/selfunld for unload samples.

<Files>
        usbkeybd.c  Main program
        usbkeybd.h  Same header file

<Run method>

        % make :  Compile
        % dsidb
        > reset 0 0
        > mstart /usr/local/sce/iop/modules/usbd.irx
        > mstart usbkeybd.irx

The same operation as above is also possible in one operation by running make run.

<Display>

The display is explained based on the following examples.

[When a keyboard is connected]

          dev_id:7  <--- A 
          usbkeybd1: attached (port=2,1,1)
          |_______|            |________|
              B                     C

A :  Device ID (1-127)
B :  Keyboard number (0-7) assigned by the IOP.
C :  Port to which USB keyboard is connected.
For the example above, C indicates that the keyboard is connected to port 2 of the system unit --> port 1 of the first stage hub --> port 1 of the second stage hub.
        
[When keyboard data is received]

usbkeybd1: count=120 led=00 data=( 00 00 00 00 00 00 00 00 )
|_______|  |_______| |____|        |_____________________|
    D          E        F                     G

D :  Keyboard number assigned by this sample driver.
     This is not a device ID.
E :  Number of times data is fetched from this keyboard.
F :  LED status.
     	bit 0 :  NUM LOCK
     	bit 1 :  CAPS LOCK
     	bit 2 :  SCROLL LOCK
     	bit 3 :  COMPOSE
     	bit 4 :  KANA
        bits 5-7 :  Not used
G :  Raw data that was sent according to an interrupt transfer by the USB keyboard. Some keyboards can send longer data.
        byte 0 :  Modifier keys
                         bit 0:  Left-Ctrl
                         bit 1:  Left-Shift
                         bit 2:  Left-Alt (Win), 
				 Left-option (Mac)
                         bit 3:  Left-Win (Win),
				 Left-Apple (Mac)
                         bit 4:  Right-Ctrl
                         bit 5:  Right-Shift
                         bit 6:  Right-Alt (Win), 
				 Right-option (Mac)
                         bit 7:  Right-Win (Win), 
				 Right-Apple (Mac)
                byte 1 :  Reserved
                byte 2 :  Key code
                  :
                byte 7 :  Key code

Key Codes
See "HID Usage Tables Document," which is distributed from the web site http://www.usb.org/developers/devclass_docs.html.


<Controller operation>
	None

<Sending SET_INTERFACE request>

When the value of the InterfaceDescriptor bAlternateSetting is 0 in this sample program, SET_INTERFACE request will not be sent during device initialization.

The reason for this is because a malfunction may occur (e.g. hangup) when a SET_INTERFACE is received by some commercial USB devices.
	
Under the USB standard, a SET_INTERFACE request must be implemented in a USB device (at a minimum only an error should be returned). Nevertheless, there are devices which are not compatible with a SET_INTERFACE request.

For a keyboard, it is difficult to think about multiple Alternative Settings, so there are probably few substitute actions using this method.

<Sending a SET_IDLE request>

In this sample program, to prevent the keyboard from sending data periodically to the host, a SET_IDLE request is sent to the keyboard during initialization.
After this the keyboard will send data to the host only when there is a change in the keyboard state.

<Notes>

For more detailed USB keyboard information, see "Human Interface Devices," which is distributed from the web site http://www.usb.org/developers/devclass_docs.html.

