/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *      Sample for usbmload.irx
 *
 *                          Version 0.10
 *                          Shift-JIS
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         activate - activate.c
 *
 *        Version       Date             Design     Log
 *  --------------------------------------------------------------------
 *        0.10          Sep,29,2000      fukunaga   Initial
 */

#include <stdio.h>
#include <kernel.h>

#include <usb.h>
#include <usbd.h>
#include <usbmload.h>


/*********************************************************
                           Type 
**********************************************************/


/*********************************************************
                     Common Variables
**********************************************************/


/*********************************************************
                         Macros
**********************************************************/


/*********************************************************
                         Prototype
**********************************************************/


/*********************************************************
                      Program Start
**********************************************************/

/* ---------------------------------------------
  Function Name	: start
  function     	: ÉÅÉCÉìä÷êî
  Input Data	: ac ,av
  Output Data   : none
  Return Value	: NO_RESIDENT_END (îÒèÌíìèIóπ)
----------------------------------------------*/
int start(int ac, char *av[])
{
  for(--ac, ++av; 0 < ac; --ac, ++av) {
    sceUsbmlActivateCategory(av[0]);
    printf("Activate:%s\n",av[0]);
  }
  
  sceUsbmlEnable();
  
  return(NO_RESIDENT_END);
}

