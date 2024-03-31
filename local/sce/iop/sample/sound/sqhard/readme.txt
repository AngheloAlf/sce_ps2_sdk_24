[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4
Copyright (C) 1999, 2000 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

MIDI playback with the hardware synthesizer

<Description>

This sample program demonstrates the operation of the MIDI sequencer (modmidi) and the hardware synthesizer (modhsyn). The program also provides a very simple example of how to use a csl module.

Two sq (MIDI sequence) files and bank binary files (tone quality) are read and played. These files can be created using the SMF2SQ and JAM tools from SCE.

libsd.irx must be running on the IOP. Load status is displayed with dsicons so if the program doesn't work correctly, check to be sure that data is being loaded properly.

<Files>
	main.c	
	sakana.sqbd (/usr/local/sce/data/sound/seq)
	sakana.hd   (/usr/local/sce/data/sound/wave)
	sakana.bd   (/usr/local/sce/data/sound/wave)
	overload.sq (/usr/local/sce/data/sound/seq)
	overload.hd (/usr/local/sce/data/sound/wave)
	overload.bd (/usr/local/sce/data/sound/wave)


<Execution>
	% make		: compile
	% make run	: run

The program plays two songs simultaneously if it is working correctly.

<Controller operations>
	None

<Notes>
	None
