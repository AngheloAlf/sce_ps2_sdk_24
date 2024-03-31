[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                     All Rights Reserved

SE playback using hardware synthesizer

<Description>
This sample program demonstrates SE stream playback (modsein) and the hardware synthesizer (modhsyn).

The program reads one group of hd/bd bank binary files (timbre) and generates sound for the SE streams, which are played back in order.
 
On the IOP side libsd.irx, modhsyn.irx, modsein.irx must be running. The load state will be displayed in dsicons. Verify loading if playback is not smooth.

<Files>
	main.c	
	eff.hd (/usr/local/sce/data/sound/wave)
	eff.bd (/usr/local/sce/data/sound/wave)

<Run method>
	% make		:Compile 
	% make run	:Execute  

Operation is correct if the "Comedy", "Glass" and "Telephone" sound effects are generated in order.

<Controller operation>
	None

<Notes>
	None

