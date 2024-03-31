[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 1.6
    Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing MIDI sequence playback

<Description>
This program plays back a MIDI sequence via the IOP using
provisional sound libraries libspu2 and libsnd2.

The data file format is completely identical to that of the current
PlayStation, so existing tools can be used to create data.

<Files>
	main.c	
	fuga.seq   (/usr/local/sce/data/sound/seq)
	simple.vh  (/usr/local/sce/data/sound/wave)
	simple.vb  (/usr/local/sce/data/sound/wave)

<Execution>
	% make		: compile
	% make run	: run

	The program is running properly if fuga 
	is played with a piano sound.

<Controller operations>
	none

<Notes>
	none
