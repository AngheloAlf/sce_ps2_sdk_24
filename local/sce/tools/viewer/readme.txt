[SCE CONFIDENTIAL DOCUMENT]
"PlayStation 2" Programmer Tool Runtime Library Release 2.5
                  Copyright (C) 2001 Sony Computer Entertainment Inc.
                                                     All Rights Reserved
                                                               Sep. 2001
=========================================================================
[3D Icon Viewer Ver. 2.0]
A program to preview the 3D icon data for the MEMORY CARD

-------------------------------------------------------------------------
Contents of this package
-------------------------------------------------------------------------

viewer/3DIconViewer/icons/             sample icon data
                    icon.txt           list of icon data
                    viewer.elf         preview program

-------------------------------------------------------------------------
How to execute and Notes
-------------------------------------------------------------------------

Copy the executable file to an any directory, and execute it as follows.

	dsedb -r run viewer.elf icon.txt

- This program is based on the release 2.5 package.
- Names of icon data should be listed up in the icon.txt.
- At maximum, 20 icon data are displayable.
- Lighting and initial location of the camera are set to be equivalent to 
 those in the browser, MEMORY CARD management screen, but not perfectly 
 the same.
 Objects and effects other than the icon files are not drawn. Please make 
 a final check by saving the data to the MEMORY CARD.

- The specification has been modified to read the ANTI field of the icon
  file as 1 for displaying the icon from the version 2.0.


-----------------------------------------------------------------------
Permission and Limitation on Use
-----------------------------------------------------------------------
The permission, limitation, etc. on using this software conform to the 
contract concluded between your company and our company (Sony Computer 
Entertainment Inc.)

-----------------------------------------------------------------------
Notes on Trademarks
-----------------------------------------------------------------------
PlayStation and PlayStation logos are registered trademarks of Sony
Computer Entertainment Inc.
All product names included in this software are trademarks and 
registered trademarks of their respective owners. 

