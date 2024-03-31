[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4
    Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

Sample program showing how to call CD/DVD-ROM drive command functions from the IOP

<Description>

This is a test program for CD/DVD-ROM command functions.

	1. Initialize CD/DVD-ROM related items
	2. Call command function
	3. Display results from command function
	4. Media exchange

	Processes 1 to 4 are repeated in the above order.

<Files>
	main.c

<Execution>

Mount a PlayStation CD in the CD/DVD-ROM drive. 
Open two windows.
Start dsidb in one window and run the following in the other window:
 
	% dsreset 0 0
	% make		ÅFcompiles program
	% make run	ÅFexecutes program

Exchange media when a message requesting an exchange of media appears.

<Controller operations>
	None

<Notes>

