[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4	     Copyright (C) 2001 by Sony Computer Entertainment Inc.					       All Rights Reserved


Icon Files for Common Network Configuration

<Contents>

- Japanese icon files:

	japanese/icon.sys	
	japanese/SYS_NET.ICO		

- English icon files:

  	english/icon.sys	
	english/SYS_NET.ICO


<Notes>

* When using the common network configuration library (netcnf), specify the icon files as follows:

- When starting up from dsidb console:

dsidb R> mstart netcnf.irx icon=SYS_NET.ICO iconsys=icon.sys

- When loading from an EE program:


	   {	       
		static char netcnf_arg[] =   "icon=host0:SYS_NET.ICO\0iconsys=host0:icon.sys";	         while(sceSifLoadModule("host0:netcnf.irx", sizeof(netcnf_arg), netcnf_arg) < 0)
	           {	               
		      printf("Can't load module netcnf.irx\n");	           		   }

	   }

* Be sure to use English icon files in English territories.