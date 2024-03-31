[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program of SPU2 waveform data matching processing module (spucodec) on the IOP

<Description>
This is a sample program for the spucodec module.

16-bit PCM data is matched on the IOP and is transferred to SPU2 local memory and voice generation is performed on the SPU2.

First, header information is removed from the VAG file that was matched beforehand, and the resultant waveform data is transferred to the SPU2 where sound is generated.

After this, the 16-bit PCM data that will become the original data in the IOP is matched and the result of matching is transferred to the SPU2 where sound is generated.

<Files>
	main.c	
	knot.vag (There is no header information as regards the /usr/local/sce/data/sound/wave VAG)
	knot_l.raw (/usr/local/sce/data/sound/wave)

<Run method>
	% make		:Compile
	% make run	:Execute

Operation is normal if the same sound is heard twice in accordance with a character message.

<Controller operation>
	none

<Remarks>
	None
