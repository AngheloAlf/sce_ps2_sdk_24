[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4.3
     Copyright (C) 2000 by Sony Computer Entertainment Inc.
                                        All Rights Reserved

Sample program showing communication using i.LINK(IEEE 1394)

<Description>

This sample program enables two PlayStation 2 consoles to send and receive automatically generated data via a protocol of SCE's own, i.LINK socket, and the monitor screen displays the results.

In this program, communication is via synchronous transmission and asynchronous reception.

This program is equivalent to the EE-side sample, and both function together. For details, refer to the EE-side sample readme file (ee/sample/ilink/readme.txt).
	
<Files>
	ilsample.c	RPC server

        IOP modules loaded automatically:
	ilink.irx (/usr/local/sce/iop/modules)
		 	i.LINK basic driver
	ilsock.irx (/usr/local/sce/iop/modules)
			i.LINK socket driver

<Execution>
	Refer to the EE-side sample readme file.

<Controller Operation>
	None

<Remarks>
	None
