[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library  Release 2.4
    Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing how to print hello

<Description>

This program displays "hello !\n", then displays its arguments one at a time.

<Files>

	hello.c

<Execution>

	% make	: compile
	% dsreset 0 2 ; dsistart hello.irx arguments : run

The program can also be executed in the following manner:

	% dsidb
	dsidb R> reset 0 2 ; mstart hello.irx arguments

	
Trace execution can be executed in the following manner:
	% dsidb
	dsidb R> reset 0 2
	dsidb R> mload hello.irx  any argument
	dsidb R> bp start
	dsidb R> mstart -d
	dsidb S> list
	dsidb S> lnext
	dsidb S> lnext

