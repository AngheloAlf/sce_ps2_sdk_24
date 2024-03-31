[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Sample program showing how to store and display log messages from the inet library

<Description>
This sample is a source program for the inetlog utility that displays log messages from inet or ppp on the TTY, and stores messages in a file.

<Precautions>
This sample program shows the invocation method for a USB Ethernet adapter.

Since the  use of an986.irx is prohibited within a title (for details, refer to the tech notes section of the developer support website), it has not been included in the release package (under sce/iop/modules).

Please download it from the developer support website.

<File>
inetlog.c   : Main program

<Run method>
	$ make      : Compile

Since the created module inetlog.irx handles logging for a program that communicates using inet, it is loaded after inet.irx.

In order for inetlog to capture the log, the debug=<debug> option of inet.irx should be specified appropriately, then inet.irx itself should be used to output the log.
(For details, refer to the section of "Run options for inet.irx" from  overview.txt).

Example:
	Using the following steps, the inetlog.txt file containing the log, is created in the current directory.

	$ dsidb
	> mstart /usr/local/sce/iop/modules/inet.irx debug=1c
	> mstart inetlog.irx host1:inetlog.txt
	> mstart /usr/local/sce/iop/modules/netcnf.irx
icon=host1:../../../../ee/sample/inet/setapp/SYS_NET.ICO iconsys=host1:../../../../ee/sample/inet/setapp/icon.sys
	> mstart /usr/local/sce/iop/modules/inetctl.irx -no_decode host1:/usr/local/sce/conf/net/net003.cnf
	> mstart /usr/local/sce/iop/modules/usbd.irx
	> mstart /usr/local/sce/iop/modules/an986.irx

<Usage>
	Arguments can be specified for inetlog in the following manner.

	inetlog [<fname>]

	<fname>: Name of the file where the log is stored. If fname is omitted, the log is displayed on TTY0 instead of being saved in a file.

<Remarks>

