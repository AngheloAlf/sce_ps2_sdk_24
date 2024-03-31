[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4
     Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

Calling an IOP-Side Stream Function - Sample Program

<Description>

This is a libcdvd stream function test program. It sequentially carries out the following actions.

	1. CD/DVD-ROM-related initialization
      	2. Calling a stream function
      	3. Playing ADPCM

<File>
	main.c

<Execution>
	% dsreset 0 0
	% make :  Compile

Insert the media containing the sample data "PS_MONO.VB" (/usr/local/sce/data/sound/wave) in the CD/DVD-ROM Drive. 
Open two windows. 
In one window, start up dsidb, and in the other window, execute the program as follows:
	
	% make run     Executes the program.

When the following is displayed in the dsidb window

	sample end.

and the ADPCM performance has ended, the program normally terminates.


<Controller Operation>
	None

<Remarks>	

