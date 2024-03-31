[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
    Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

SIF CMD Protocol Sample Program

<Description>

This program shows how to use the SIF CMD protocol.

The program performs the following actions in the order shown.

            1.  Initialize SIF CMD
            2.  Register the buffer and command functions
            3.  Call destination-side command functions
            4.  Display command function results

<Files>
	iopmain.c

<Execution>
	% dsreset 0 0
	% make:  Compile

After compilation, start up dsidb.
	% dsidb

Next, open a separate window and start up dsedb from an EE-side sample (ee/sample/sif/sifcmd).

In the IOP sample-side window, execute the following:

	> mstart iopmain.irx

In the EE sample-side window, execute the following:

	> run main.elf

The program has completed normally if the following is displayed:

            test0 = 10 test1 = 20


<Controller operation>
	None

<Notes>

This program was designed to be almost identical to the EE-side sample program.