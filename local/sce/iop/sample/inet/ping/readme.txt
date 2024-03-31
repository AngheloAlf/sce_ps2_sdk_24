[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program for confirming that the host is reachable using the inet library (the ping program).

<Description>
In this sample program, an ICMP echo request packet is sent to the target host using inet and an ICMP echo reply is returned to confirm that the host is reachable.

This program is the inet version of the ping program used commonly in UNIX etc.

[Precautions]
In this sample, the an986.irx is required if the USB Ethernet adapter used.

Since the  use of an986.irx is prohibited within a title (for details, refer to the tech notes section of the developer support website), it has not been included in the release package (under sce/iop/modules).

Please download it from the developer support website.

<File>
	ping.c   : Main program

<Run method>
	$ make      : Compile

To use ping.irx for input and output, dsicons should be opened for TTY0 in a separate window.

	$ dsicons                               : Opens TTY0 I/O

The program is executed in the original window.
	$ dsidb                               : Start dsidb
	> source inet.rc.{hdd,hdd.pppoe,usb,usb.pppoe}  : Initialize network
Select the source file that corresponds to the environment you wish to use.
	....
        Wait for a display like
              inetctl: (ID=1) [ XXX.XXX.XXX.XXX ]
          (This is a sign that the IP address has been set by the inet layer)

        > mstart ping.irx [hostname]        : Start ping.irx
        
	<hostname>: Destination host

The input and output of ping is displayed in the window in which dsicons was executed.

To quit ping.irx, enter Control-C in dsicons.
This allows the statistics gathered by ping.irx till that point to be displayed.

<Usage>

	The ping.irx has the following options.

    ping [<option>...] <hostname>
    <option>:
      -c <count> : Specifies the number of packets to be sent. When a transmission of only the specified number of packets is performed, the program will end by displaying the statistics (default infinite)
      -s <size>  : Specifies the size of 1 packet (default is 56 bytes)
      -tty <N>  : Specifies the TTY for use in input and output by ping.irx (default is TTY0)

<Controller operation>
	None

<Notes>
The accompanying inet.rc.* files assume that the connection environment configuration is USB Ethernet, HDD Ethernet, PPPoE (USB Ethernet), or PPPoE (HDD Ethernet).

To change the IP address or the PPPoE user name, or to use PPP, change the configuration in the combination file (netXXX.cnf) that is specified for the inetctl.irx argument in each inet.rc.* file.

inet.rc.* references configuration files under /usr/local/sce/conf/net. This is because this program is for use as a development sample.  Please note that for an official title etc., individually encoded configuration files should be used. For information on how to use individually encoded configuration files, please refer to the sample configuration application.

Please note that for an official title etc., individually encoded configuration files should be used. For information on how to use individually encoded configuration files, please refer to the sample configuration application.

In the current connection environment configuration files, only one interface is specified per file. Hence, in this sample program, it is assumed that multiple devices are not being used simultaneously. In this program, the target device works under the condition that only a single connection has been made. Please note that no considerations have been made regarding multiple connections for the same device.

<Processing summary>
	A summary of processing is given below.

1. Start the worker thread.
2. Start threads for receiving, sending, TTY input and for waiting on an event.

Sending thread
	Sends an ICMP echo request packet every second.
If the number of packets to be sent has been specified using the -c option then notifies the thread that is waiting on an event when it has reached the specified number of transmissions.

Receiving thread
	Repeatedly receives IP packets. If there is a reply corresponding to an ICMP echo request sent by the sending thread, then displays on the TTY.

Input thread
	Waits for receipt of input from the TTY. If a Control-C is input, then notifies the thread that is waiting on an event.

Thread waiting on an event
	Waits to receive a completion notification from each of the threads and displays statistics after it receives the completion notification.
