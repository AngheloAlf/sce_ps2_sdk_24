[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4
    Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                       All Rights Reserved

Sample program showing how to play voices

<Description>

This program uses libsd to play SPU2 voices on the IOP. Options are provided to experiment with batch processing, DMA callback, and IRQ callback.

The VAG file format is the same as that of the current PlayStation, so files can be created with existing tools.

With SPU2, care must be taken with core selection. Please note that the output from core0 passes through the main volume control of core1 (i.e., if core1's main volume setting is 0, core0 sounds will not be played).

When BATCH_MODE is set to 1, operations that had been performed through the independent API will be performed as batch operations. An EE memory write test is performed so dsedb can be used to check the memory dump.

When IRQ_CB_TEST is set to 1, IRQ callbacks are enabled. When the 0x1000th address (IRQ_ADDR_OFST) from the start of the VAG file is accessed, an "interrupt detected" message is output to the console.

When DMA_CB_TEST is set to 1, DMA end-of-transfer callbacks are enabled. After a VAG file transfer completes, an "interrupt detected" message will be output to the console.

<Files>
	init_bat.h
	main.c	
	piano.vag (/usr/local/sce/data/sound/wave)

<Execution>
	% make		: compile
	% make run	: run

The program is working properly if it plays an eight-tone octave with a Piano sound.

<Controller operations>
	None

<Notes>
	None
