[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing how to display information about each connection in the inet library (netstat program)

<Description>
This sample is the source program for the netstat utility that displays information about each connections in inet.

This program is the inet version of the netstat program used commonly in UNIX etc.

<File>
	netstat.c        : Main program

<Run method>

	$ make		: Compile

netstat.irx is used when some other program is operating over inet.

For example, the sample program /usr/local/sce/ee/sample/inet/echo_server is started in a separate window.

	$ cd /usr/local/sce/ee/sample/inet/echo_server
	$ make
	$ dsedb -r run main.elf 3               : Start echo_server 

While the echo_server is operating, netstat.irx is loaded in the original window.

	$ dsidb -nr                             : Start dsidb (no reset) 
	> mstart netstat.irx                    : Load netstat.irx
	...
	Proto Recv-Q Send-Q Local Address        Foreign Address      State
	tcp        0      0 0.0.0.0:7            0.0.0.0:0            LISTEN
	tcp        0      0 0.0.0.0:7            0.0.0.0:0            LISTEN
	tcp        0      0 0.0.0.0:7            0.0.0.0:0            LISTEN

When netstat.irx is loaded, then information for each connection over inet is displayed as shown above.

In this case, it can be seen that 3 connections have been created by the echo_server and that each of the connections is in the LISTEN state.

<From the display point of view>
When netstat.irx is loaded, information for each connection is displayed on separate lines.

The fields on each line have the following meaning.

	Proto           : Protocol
	Recv-Q          : Number of data bytes in the receive buffer
	Send-Q          : Number of data bytes in the transmit buffer
	Local Address   : Local IP address that has been binded to the connection
	Foreign Address : Remote IP address that has been binded to the connection
	State           : State of the connection

For details regarding each of the fields, refer to the sceInetInfo structure of inet.txt.

Information for each of the connections that has been obtained in netstat.irx is passed by sceInetControl() through the sceInetInfo structure.

<Controller operation>
	None

<Remarks>
By specifying options, the UNIX version of netstat can display diverse network information such as channel control display etc., however, options are not supported by this program. Only connection information can be displayed by this program.

