[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

inet library sample that sends and receives packets between a client and a server using TCP. (client program)

<Description>
This sample program uses inet to establish a TCP connection with a server, then sends and receives a specified amount of data.

The data size and number of threads to start simultaneously can be specified in an argument.

The corresponding server program must be started first.

[Caution]
In this sample an986.irx is required when using a USB Ethernet adapter.

Because the use of an986.irx is not allowed in titles (refer to the technical note on the developer support web site), it is not included with the release package (under sce/iop/modules). Download an986.irx from the developer support web site.

<Files>
        load_test.c        : Main program


<Run method>
First start the server program on the server. There are two types of server programs that run on Linux and the EE. Refer to the following documents for details on starting each program.

/usr/local/sce/ee/sample/inet/load_test/linux_daemon/readme.txt
/usr/local/sce/ee/sample/inet/load_test/daemon/readme.txt

Once server preparations are complete, start the client program.

        $ make                                  : Compile
        $ dsidb					: Start dsidb
        > source inet.rc.{hdd,hdd.pppoe,usb,usb.pppoe}    : Initialize network
          Select the source file that corresponds to the environment you wish to use.
          ....
          inetctl: Waits until a display like "(ID=1) [ XXX.XXX.XXX.XXX ]" is output.
          (signals that the IP address has been set in the inet layer)

        > mstart load_test.irx <saddr> <th_num> <pktsize>
                                                : Execute load_test.irx
        <saddr>: Server address
	<th_num>: Number of simultaneous connections to establish
	<pktsize>: Data size to transmit

<Controller operation>
        None

<Notes>
IThe accompanying inet.rc.* files assume that the connection environment configuration is USB Ethernet, HDD Ethernet, PPPoE (USB Ethernet), or PPPoE (HDD Ethernet).

To change the IP address or the PPPoE user name, or to use PPP, change the configuration in the combination file (netXXX.cnf) that is specified for the inetctl.irx argument in each inet.rc.* file.

inet.rc.* references configuration files under /usr/local/sce/conf/net. This is because this program is for use as a development sample.  Please note that for an official title etc., individually encoded configuration files should be used. For information on how to use individually encoded configuration files, please refer to the sample configuration application.

In the current connection environment configuration files, only one interface is specified per file. Hence, in this sample program, it is assumed that multiple devices are not being used simultaneously. In this program, the target device works under the condition that only a single connection has been made. Please note that no considerations have been made regarding multiple connections for the same device.

<Process summary>
A summary of the process is given below.

        1. Start work thread.
        2. Start th_num communication threads.
	For each thread:
        3. Establish a TCP connection with the server.
        4. Send data size to send and receive.
	5. Send pktsize bytes of data to server.
        6. Receive data from server.
        7. Check that data is identical.
