[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing how to exchange data directly with the modem layer

<Description>
This sample program shows how to exchange data directly with the modem layer without using PPP. Key data input from I1TTY is sent to the modem layer as data. Data received from the modem layer is output to I0TTY.

As this sample program is functionally similar to general terminal software, it can also be used to confirm the command or result code at the time of a PPP connection.

<File>

	main.c		: Main program

<Run method>

	$ make		: Compile

	$ dsicons 10
	The dsicons that sends key input with I1TTY is invoked in a separate window. 

	$ dsidb
	> mstart main.irx

	main.irx is invoked first.

	> mstart /usr/local/sce/iop/modules/usbd.irx

	For a USB Modem, USBD should be started prior to loading the modem driver.

	> mstart /usr/local/sce/iop/modules/inet.irx

	inet.irx must also be started if the modem driver uses memory allocate and free functions in INET.

	> mstart <modem_driver>

	Starts the modem driver.


<Operation>

	Key input other than '~'

		Sent as is to the modem driver.

	'~' + '~'

		The character '~' is sent to the modem driver.

	'~' + '?'

		Shows the current status of the modem such as number of bytes sent to and received from the modem.

	'~' + '+'

		Calls the function for starting use of the modem driver when the modem driver is stopped.

	'~' + '-'

		Calls the function for stopping use of the modem driver when the modem driver is being used.

	'~' + character other than those given above

		The input character is sent to the modem driver after the '~' character.


<Notes>
This program keeps function entries for sceModemRegisterDevice and sceModemUnregisterDevice, which are also kept by ppp.irx, so this program cannot run at the same time as ppp.irx.

