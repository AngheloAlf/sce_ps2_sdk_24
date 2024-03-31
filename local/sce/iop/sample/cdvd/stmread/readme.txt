[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4
     Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

Sample program showing how to call an IOP stream function

<Description>

This sample program tests the libcdvd stream functions.

The following steps are performed:
	1: Initialize CD/DVD-ROM
	2: Call stream function

<Files>
	main.c

<Execution>
	% dsreset 0 0
	% make		: compile

Place a PlayStation CD in the CD/DVD-ROM drive. Open two windows.
Start dsidb in one window and run the program in the other window by entering:
	
	% make run      run program

If all goes well, the dsidb window should show

	sample end.

and the program file on the PlayStation CD should be copied to the current directory.

<Controller operations>
	None

<Notes>

