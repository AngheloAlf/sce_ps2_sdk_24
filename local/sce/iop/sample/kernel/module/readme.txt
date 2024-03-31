[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                     All Rights Reserved

Sample of a resident module

<Description >

This is an example of how to create a resident library module.

<File>

    mylib.c	   Resident library sample source code
    mylib.tbl	   Resident library entry table sample
    client.c	   Sample program that uses the resident library
    mylibrm.c	   Sample source code that uses the resident library 
		   which can be deleted
    clientrm.c	   Sample source code for the resident module 
		   which perform self-deletion
    modlist.c	   Sample that uses GetModuleIdList() and 
		   ReferModuleStatus()
    searchmd.c	   Sample that uses SearchModuleByName() and
		   SearchModuleByAddress()
    removemd.c	   Sample source code that deletes other modules

<Activating the program>

    For mylib.irx and client.irx

    % make				      :compiles the program
    % dsreset 0 2
    % dsistart mylib.irx; dsistart client.irx :executes the program

This program can also be executed by using the following method.

    % dsidb
    > reset 0 2 ; mstart mylib.irx; mstart client.irx

    For mylibrm.irx, client.irx, clientrm.irx, and removemd.irx

    First, in another window,
    % dsicons 
    is to be executed.
	
    % dsreset 0 2
    % dsistart mylibrm.irx
    % dsistart client.irx
    % dsistart clientrm.irx
    % dsilist                             : checks modules in memory
    Wait until clientrm.irx is completed,
    % dsistart removemd.irx mylibrm
    % dsilist                             : checks modules in memory

    For modlist.irx

    % dsistart modlist.irx     : can display a list of modules
		 		 in memory
    % dsistart modlist.irx 2   : can display the status of a
				 specified module ID
    % dsistart modlist.irx modulename  : can display a list of  
modules with the specified name.


    For searchmd.irx

	% dsreset 0 2
	% dsistart searchmd.irx
