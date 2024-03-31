[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
    Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

SIF RPC Protocol Sample Program

<Description>

This program shows how to use the SIF RPC protocol.

Client/server operation is performed using the EE and IOP. #define SERVER should be defined on one side.  That side then becomes the server.

The client creates data and sends it to the server. The server processes the received data and sends the results back to the client.

The server is called using a number of different modes.

<Files>
	iopmain.c

<Execution>
	% dsreset 0 0
	% make:  Compile

After compilation, start up dsidb.
	% dsidb

Next, open a separate window and start up dsedb from an EE-side sample (ee/sample/sif/sifrpc).

In the IOP sample-side window, execute the following:

	> mstart iopmain.irx

In the EE sample-side window, execute the following:

	> run main.elf

The server uses 0xff000000 to mask the transmitted data. The program completes normally when the server sends the result back to the client.

<Controller operation>
	None

<Notes>
This program was designed to be almost identical to the EE-side sample program.
