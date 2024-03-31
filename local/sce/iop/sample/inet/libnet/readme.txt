[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4.2
   Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

libnet library sample (IOP-side program) that transparently handles the inet library from the EE 

<Description>
This is an IOP-side program of the libnet library that transparently handles inet from the EE. Using the SIF system, libnet can transparently call functions from the EE on the IOP. Consequently, it is not necessary to write a separate IOP program which handles inet for network-compatible applications that run on the EE. Generally, inet can be handled from the EE.

<Files>
        libnet.c	: Main program

<Run method>
        $ make		: Compile

The created module libnet.irx is loaded with sceSifLoadModule() during EE program initialization.


<Notes>
Internally libnet uses libmrpc (multithread compatible SIFRPC). Consequently, function calls can be issued from multiple threads on the EE at arbitrary times.
