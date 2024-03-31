[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 1.6
   Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing streaming playback with voices

<Description>

This program uses the libspu2 provisional sound library, to perform streaming playback with SPU2 voices via the IOP.

<Files>
	main.c	
	tr1l_pad.vb (/usr/local/sce/data/sound/wave)
	tr1r_pad.vb (/usr/local/sce/data/sound/wave)

<Execution>
	% make		: compile
	% make run	: run

	The program is running properly if the 
	music is played back in stereo.

<Controller operations>
	none

<Notes>
	none
