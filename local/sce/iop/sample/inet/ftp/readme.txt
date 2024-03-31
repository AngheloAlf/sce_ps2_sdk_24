[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing how to communicate with a remote host via the File Transfer Protocol (FTP) using the inet library.

<Description>
This program uses inet to connect with a remote host that is running an ftp server program and perform actions on remote files. For ease of use, these actions are similar to those that can be performed on local files, including saving files to the HDD.

<Notes>
This program is intended solely as a sample that shows how to use the inet library, and as such does not implement all ftp commands. Supported commands are listed at the end of this document. Also, because this is open-source, it is possible to implement new commands and functions. However, we are leaving it to users to add these as needed, although we will consider adding those functions and commands that are requested frequently.

<Files>
  ftp.c		ftp main program
  ftp.h		Header file for ftp.c
  hddpart.c	Program that partly implements HDD access
  httpart.h	Header file for hddpart.h
  inet.rc.hdd	Script file that reads dsidb, and loads settings and modules.
  Makefile	Makefile

<Run Method>
  $ make			: Compile program

  Next, in another window,
  $ dsicons			: start dsicons

  Return to first window
  $disdb			: start dsidb
  > source inet.rc.hdd		: execute script that loads required modules
  > mstart ftp.irx[option] remote_host_ip_address

  You can then type input into the window where dsicons was started.   

  Replace "remote_host_ip_address" with the address of the remote host you want to communicate with. Options are as follows:
        -h			: show this help message
        -f			: format HDD


<Notes>
The script file inet.rc.hdd will use /usr/local/sce/conf/net/net005.cnf as the default for the inetctl.irx argument. The files ifc005.cnf and dev002.cnf located in the same directory will also be accessed. Before executing inet.rc.hdd, you must first edit these files and put in the correct network settings such as the IP address.

<Controller Operation>
	None

<Remarks>
Currently, the following commands are supported.

	?		help		quit		ascii		a
	binary		bin		bi		b		get
	put		pwd		dir		ls		mkdir
	cd		rmdir		rm		lcd		lpwd
	lls		lmkdir		

This sample program checks operation for the ftp daemon using wu-2.6.0(1) and wu-2.6.1(1) only.
