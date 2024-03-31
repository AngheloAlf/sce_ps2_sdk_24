[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4
    Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

Sample Program Showing How to Call an IOP-Side Stream Function (standard I/O function)

<Description>

This is a libcdvd stream function (standard I/O function)test program. It sequentially carries out the following actions.

	1. CD/DVD-ROM-related initialization
      	2. Calling a stream function
      	3. Playing straight PCM

<Files>
	main.c
      	wav2int.c 	Linux sample program for converting 
			Wavfile to PS2_PCM_Raw_Format

<Execution>
	% dsreset 0 0
	% make :  Compile

Insert the media containing the sample data "M_STEREO.INT" (/usr/local/sce/data/sound/wave) in the CD/DVD-ROM Drive.
Open two windows.
In one window, start up dsidb, and in the other window, execute the program as follows:

	% make run    Executes the program.

When the following is displayed in the dsidb window

	sample end.

and the straight PCM performance has ended, the program normally terminates.


<Controller Operation>
	None

<Remarks>	

The wave2int.c file is included as a sample data converter to PlayStation 2 straight PCM data.
