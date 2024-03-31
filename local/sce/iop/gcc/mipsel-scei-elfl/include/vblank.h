/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5
 */
/* $Id: vblank.h,v 1.2 1999/10/12 09:20:51 tei Exp $ */

/*
 *                     I/O Processor System Services
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                         vblank.h
 *                         vblank services
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.00           1999/10/12      tei
 */

#ifndef _VBLANK_H_
#define _VBLANK_H_

#define VB_START	0
#define VB_END		1

/* thread level function */
extern int WaitVblankStart();
extern int WaitVblankEnd();
extern int WaitVblank();
extern int WaitNonVblank();

/* interrupt handler layer */
extern int RegisterVblankHandler( int edge, int priority,
			   int (*handler)(void*), void *common );
extern int ReleaseVblankHandler( int edge, int (*handler)(void*) );

#endif /* _VBLANK_H_ */
