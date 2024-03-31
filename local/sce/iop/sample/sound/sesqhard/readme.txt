[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

SE sequence playback using hardware synthesizer 

<Description>
This sample program shows how to use the SE sequencer (modsesq) and the hardware synthesizer (modhsyn).

The program reads one sq file (which includes multiple SE sequences) and one group of hd/bd bank binary files (timbre) then plays back the SE sequences in order.

On the IOP side libsd.irx, modhsyn.irx, modsesq.irx must be running. The load state will be displayed in dsicons. Verify loading if playback is not smooth.

<Files>
	main.c	
	sesample2.sq (/usr/local/sce/data/sound/seq)
	sesample2.hd (/usr/local/sce/data/sound/wave)
	sesample2.bd (/usr/local/sce/data/sound/wave)

<Run method>
	% make		:Compile
	% make run	:Execute

Operation is correct if six SE sequences are played back in order.

<Controller operation>
	None

<Notes>
	None

