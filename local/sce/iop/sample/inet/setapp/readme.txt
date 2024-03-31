[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4.2
   Copyright (C) 2001 by Sony Computer Entertainment Inc.
                                      All Rights Reserved

Network configuration application sample program

<Description>
This sample program adds, edits and deletes network configuration files which are commonly used in title applications which utilize the network. It includes functions which use added / edited network settings to connect and disconnect the network.

<Files>

	- IOP side (/usr/local/sce/iop/sample/inet/setapp/) 

	Makefile	: Makefile
	ifnetcnf.h	: Header file
	ifnetcnf.c	: NetCnf RPC server
	read_env.c	: Network configuration file read process
	update_env.c	: Network configuration file edit process
	write_env.c	: Network configuration file write process
	term.txt	: Network configuration application item names and terminology
	flow.txt	: Network configuration application sample flow


	- EE side (/usr/local/sce/ee/sample/inet/setapp/)

	Makefile	: Makefile
	setapp.h	: Header file
	main.c		: Main program
	connect.c	: Initial connection screen
	menu.c		: Menu display
	list.c		: List display
	dev.c		: Setting process of connection device
	dev_more.c	: Detailed setting process of 
			  connection device
	ifc.c		: Setting process of connection provider
	ifc_more.c	: Detailed setting process of 
		          connection provider
	Rpc.c		: NetCnf RPC client
	util.c		: Character input process
	Gs_Initialize.c	: GS initialization process
	term.txt	: Network configuration application 
			  item names and terminology
	flow.txt	: Network configuration application
			  sample flow


	- Common (/usr/local/sce/ee/sample/inet/setapp/)

	common.h	: IOP, EE Common header file


<Run method>

	- Operations on directory on IOP side

	Start dsidb
	dsidb > (!gmake clean);(!gmake)

	- Operations on directory on EE side

	Start dsedb 
	dsedb > (!gmake clean);(!gmake)
	dsedb > reset;run main.elf

	(*)  Since setapp.h on the EE contains the IOP module path description, determine whether it is appropriate.

	setapp.h also contains a description of the USB autoload configuration file path. This configuration file contains settings related to the USB-Ethernet interface driver an986.irx, which SCEI currently provides for development purposes. When using a USB modem, the PATH of the USB modem driver and DIAL_CNF must be specified in this configuration file.

	The contents of /usr/local/sce/conf/usb/usbdrvho.cnf are for development use so they cannot be used in titles as is.

<Controller operation>

	- Operations on Network Connection screen

	Direction key up	:Item select
	Direction key down	:Item select
	Circle button	:Configuration file select (toggles each time the Circle button is pressed)
	SELECT button	:Move to network setting screen (Network Setting)
	START button	:Connection start/connection end

	(When combination selected (Combination))
	Triangle button	:Register / change combination
	Square button	:Delete combination


	- Operations on Network Setting screen (menu)

	Direction key up	:Item select
	Direction key down	:Item select
	Circle button	:Select(move to next screen)
	Cross button	:Cancel (move one screen back)


	- Operations on Network Setting screen (list)

	Direction key up	:Item select
	Direction key down	:Item select
	Circle button	:Select(move to next screen)
	Cross button	:Cancel (move one screen back)


	- Operations on Network Setting screen (dev)

	Direction key up	:Item select
	Direction key down	:Item select
	Circle button	:Select(move to next screen)
	Cross button	:Cancel (move one screen back)
	Square button	:Move to detailed screen (only when displayed)


	- Operations on Network Setting screen (dev_more)

	Direction key up	:Item select
	Direction key down	:Item select
	Circle button	:Select(move to next screen)
	Cross button	:Cancel (move one screen back)
	Triangle button	:Change to character edit mode (only items which require character input)


	- Operations on Network Setting screen (ifc)

	Direction key up	:Item select
	Direction key down	:Item select
	Circle button	:Select(move to next screen)
	Cross button	:Cancel (move one screen back)
	Square button	:Move to detailed setting screen / return to default setting values (only when displayed)
	Triangle button	:Change to character edit mode (only items which require character input)


	- Operations on Network Setting screen (ifc_more)

	L1 button	:Switch item
	L2 button	:Switch item
	R1 button	:Switch item
	R2 button	:Switch item
	Direction key up	:Item select
	Direction key down	:Item select
	Circle button	:Select(move to next screen)
	Cross button	:Cancel (move one screen back)
	Square button	:Return to default setting values (only when displayed)
	Triangle button	:Change to character edit mode (only items which require character input)


	- Operations in character edit mode

	Direction key up	:Character select
	Direction key down	:Character select
	Direction key right	:Choose character / move cursor
	Direction key left	:Choose character / move cursor
	Triangle button	:Insert
	Circle button	:Choose character (move right one character)
	Cross button	:End edit
	Square button	:Delete

	* The operation of moving to character edit mode on the previous screen  is displayed on this screen.

<Flow from settings to connection>

	A procedure is described here that creates new settings for a USB Modem connecting to a provider A, then makes the connection.

	* When using a USB Modem, define the USB Modem driver and DIAL_CNF PATH in the USB auto load configuration file (/usr/local/sce/conf/usb/usbdrvho.cnf).

	(1) Connect the USB Modem to the PlayStation 2.

	(2) Start the program and press the SELECT Button from the initial screen.
	    The following will be displayed. Select "New Setting" at the top.

	    * New Setting(Hardware Setting Network Service Provider Setti...
	      Hardware Setting
	      Network Service Provider Setting

	(3) If the Modem driver is loaded correctly, the name of the Modem will be displayed. Select the Modem.

	    * Modem-A
	      Modem-B

	(4) Select the type of telephone circuit.

	    * tone
	      pulse

	(5) Set the user ID and password of the provider (select each item and use the Triangle button to enter edit mode).

	    User ID  : Your user ID
	    Password : Password

	(6) Set the telephone number of the access point.

	    Tel.number1: 00-0000-0000

	(7)  Set the method of obtaining the name server address.

	    * Auto
	      Manual

	(8) Enter an appropriate provider setting name (use the Triangle button to enter edit mode).

	    NSP Setting Name : Your user ID's setting

	(9) Save the provider settings (Circle button).

	(10) When the save completes, press the Circle button.

	(11)  The screen will return to the first menu of the setting screen. Press the Cross button to return to the initial screen (start).

	(12) Select "Combination" and press the Circle Button. Then select a combination of hardware settings and provider settings and press the Triangle button.

	     * Combination              : Combination1
	       Hardware                 : Modem-A
	       Network Service Provider : Your user ID's setting

	(13) When "Complete!" displays, press any button.

	(14) Pressing the START Button will connect to the network in accordance with the settings.

<Limitations and precautions>

	- When the network configuration file is saved on the host, according to the dsnet specifications, part of the edit operation executed by setapp will not be directly reflected in the file on the host. Details of this are given below.

	(1) When the contents of a configuration management file is changed and saved, the changes are added to a temporary file called <configuration management filename>.tmp and not the configuration management file itself.
	
	(2) If there are configuration files in a configuration management file that are unregistered, they are not automatically deleted.

	(3) If an operation to delete a configuration file is executed, only the entries of the configuration management file are deleted. The configuration file on the host will not be deleted.

	These disparities will not occur if the network configuration file is saved on a memory card.

	- The following operating modes can be set by defining a macro in common.h.

	  #define DEBUG:		Output debugging printout
	  #define NO_HDD:		Do not recognize the hard disk
	  #define NO_HOST:		Do not recognize the host
	  #define MC_NO_DECODE:		Do not perform individual encoding/decoding of configuration files on the memory card
	  #define HDD_NO_DECODE:  	Do not perform individual encoding/decoding of configuration files on the hard disk
	  #define HOST_NO_DECODE:  	Do not perform individual encoding/decoding of configuration files on the host
	  #define HOST_READ_ONLY:  	Treat configuration files on the host as read-only files
	  #define CD_ROM:		Use CD-ROM
	  #define REPLACE:		Replace module
	  #define MEM2MB_MODE:		Use T10000 in 2MB mode

	- If setapp is installed in a directory different from usual, correspondence can be achieved by revising the locations of the following header files.

	  </usr/local/sce/ee/sample/inet/setapp/setapp.h>

	  #define BASE_NAME "/usr/local/sce" /* absolute PATH */
	  #define BASE_NAME "../../../.."    /* relative PATH */

	  </usr/local/sce/iop/sample/inet/setapp/common.h>

	  #include "/usr/local/sce/ee/sample/inet/setapp/common.h"
	                                     /* absolute PATH */
	  #include "../../../../ee/sample/inet/setapp/common.h"
	                                     /* relative PATH */

	- setapp, which was created as a reference for control flow according to the network configuration application specification, easily implements processing related to the input interface.

<Notes on Creating Applications>

	- In a title, the items, item names, and terms that are used in this sample program should be replaced with the corresponding items, item names, and terms.
	  See term.txt for a correspondence table.

	- For information about standards for creating network configuration applications used in titles, see the file setapp_r.pdf in the directory /sce/doc/general/.


