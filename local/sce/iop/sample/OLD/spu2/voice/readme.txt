[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 1.6
   Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing how to generate voices

<Description>

This program uses the libspu2 provisional sound library to generate SPU2 voices via the IOP. Options are provided to experiment with DMA callback and IRQ callback.

The VAG file format is completely identical to that of the current PlayStation, so existing tools can be used to create files.

API usage is roughly identical to that of the current PlayStation, except that core selection is needed for SPU2. Note that the output from core0 passes through the main volume control (i.e., if the main volume control for core1 is set to zero, core0 sounds will not be played).

If IRQ_CB_TEST is set to 1, IRQ callbacks are enabled. When address 0x1000 (IRQ_ADDR_OFST) from the start of the VAG file is accessed, an "IRQ interrupt detected" message will be sent to the console.

If DMA_CB_TEST is set to 1, DMA transfer termination callbacks are enabled. When transfer of a VAG file has completed, a "DMA interrupt detected" message will be sent to the console.

<Files>
	main.c	
	piano.vag (/usr/local/sce/data/sound/wave)

<Execution>
	% make		: compile
	% make run	: run

	When the program is working, an 8-note 
	scale with a piano sound will be generated.

<Controller operations>
	none

<Notes>
	none
