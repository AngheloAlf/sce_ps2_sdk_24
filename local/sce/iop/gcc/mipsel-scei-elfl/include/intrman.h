/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5
 */
/* $Id: intrman.h,v 1.10 2002/02/15 07:45:51 tei Exp $ */

/*
 *                     I/O Processor System Services
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                         intrman.h
 *                         interrupt manager
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.00           1999/10/12      tei
 *       1.01           1999/11/13      tei
 */

#ifndef _INTRMAN_H
#define _INTRMAN_H

#ifndef NULL
#define NULL ((void*)0)
#endif

#define NEXT_DISABLE	0
#define NEXT_ENABLE	1

#define HTYPE_ASM	0
#define HTYPE_C		1
#define HTYPE_FULL	2

#if defined(__LANGUAGE_C) || defined(__GNUC__)

enum INUM {
    /* 0,1,2,3 */
    INUM_VBLANK=0, INUM_GM,     INUM_CDROM,   INUM_DMA,
    /* 4,5,6,7 */
    INUM_RTC0,	   INUM_RTC1,   INUM_RTC2,    INUM_SIO0,
    /* 8,9,10,11 */
    INUM_SIO1,     INUM_SPU,    INUM_PIO,     INUM_EVBLANK,
    /* 12,13,14,15 */
    INUM_DVD,      INUM_PCMCIA, INUM_RTC3,    INUM_RTC4,
    /* 16,17,18,19 */
    INUM_RTC5,     INUM_SIO2,   INUM_HTR0,    INUM_HTR1,
    /* 20,21,22,23 */
    INUM_HTR2,     INUM_HTR3,   INUM_USB,     INUM_EXTR,
    /* 24,25,26,27 */
    INUM_FWRE,     INUM_FDMA,
    /* 28,29,30,31 */
    /* 32,33,34,35 */
    INUM_DMA_0=32, INUM_DMA_1,    INUM_DMA_2,    INUM_DMA_3,
    /* 36,37,38,39 */
    INUM_DMA_4,    INUM_DMA_5,    INUM_DMA_6,	 INUM_DMA_BERR,
    /* 40,41,42,43 */
    INUM_DMA_7=40, INUM_DMA_8,    INUM_DMA_9,    INUM_DMA_10,
    /* 44,45,(46),(47) */
    INUM_DMA_11,   INUM_DMA_12,
    /* (48),(49),(50),(51) */
    /* (52),(53),(54),(55) */
    /* (56),(57),(58),(59) */
    /* (60),(61),(62),(63) */
    INUM_MAX = 64
};

#define IMODE_DMA_IRM 0x100
#define IMODE_DMA_IQE 0x200

typedef int (* intr_handler)();

extern int RegisterIntrHandler(int intrcode, int type, intr_handler func,
				void *common);
extern int ReleaseIntrHandler(int intrcode);
extern int EnableIntr( int intrcode );
extern int DisableIntr( int intrcode, int *oldstat );

extern int CpuSuspendIntr(int *oldstat);
extern int CpuResumeIntr(int oldstat);
extern int QueryIntrContext();
extern void iCatchMultiIntr();

#endif /* if defined(__LANGUAGE_C) || defined(__GNUC__) */

#endif /* _INTRMAN_H */
