[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4.2
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved


Network configuration GUI library sample program
inet settings initialization module

<Description>
This program is an IOP module for initializing the inet settings used by sample programs. These programs show examples of how to use the network configuration GUI library (ntguicnf) to connect and disconnect from the network.

<Files>
	Makefile	:  Make file
	setinit.c	:  Main program
	setinit.h	:  Main program header 
			   (also referenced from the EE)
	write_env.c	:  Program for converting to inet 
			   settings
	write_env.h	:  Program header for converting to 
			   inet settings

<Usage>
	make clean
	make

	(*) Specifies setinit.irx, which is created in this directory by /usr/local/sce/ee/sample/inet/ntguicnf/sample.c.

