[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Streaming of waveform data using SPU2 bypass processing

<Description>
This sample program shows how to enable SPU2 bypass processing in the SPU2 CORE0 MEMIN sound data input area to perform streaming of waveform data.

The program reads one .inb file (a waveform data file that has been interleaved by 512 bytes L/R and in which data has been sequenced for bypass processing), enables SPU2 bypass processing, and performs streaming.

	[Note]
	Since audio is output only to the optical digital output, provide an environment in which the PlayStation 2 optical digital output and an optical digital output such as an AV amp can be played.

<Files>
	main.c	
	knot2.inb (/usr/local/sce/data/sound/wave)
	knot2.int (/usr/local/sce/data/sound/wave)
	... knot2.int, is based on knot.int and has zeros added so that the file size is a multiple of 1024 bytes.
	    knot2.inb is data that was converted based on knot2.int.

<Run Method>
	% make:  	Compile
	% make run:  	Execute

The program is operating normally when sound is output via the optical digital output.

<Controller Operation>
	None

<Remarks>
1. The accompanying sample int2inb.c shows how to convert data to straight PCM for PlayStation 2 bypass processing.

2. This sample program can also be used to output bitstream data that is used in public 3D sound, to the optical digital output.
To play bitstream data, specify SD_SPDIF_OUT_BITSTREAM instead of SD_SPDIF_OUT_BYPASS for the specified option in sceSdSetCoreAttr().
The waveform data that is used by this sample will not be played correctly if SD_SPDIF_OUT_BITSTREAM is specified.

