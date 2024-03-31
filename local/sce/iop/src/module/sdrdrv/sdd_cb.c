/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/* 
 *                    I/O Proseccor sample Library
 *                          Version 0.50
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2002 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                       rs2drv.irx - rsd_cb.c
 *                         call back thread
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   0.30      Jun.17.1999     morita    first checked in
 *   0.50      Aug.18.1999     morita    SpuStSetCore etc. added.
 *                                       rewrite for new siflib.
 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>

#include "sdr_i.h"

#ifdef SCE_OBSOLETE
#define SDR_CB_DMA0    (1 << 0)
#define SDR_CB_DMA1    (1 << 1)
#define SDR_CB_IRQ     (1 << 2)
#endif
#define SDR_CB_DMA0INT (1 << 8)
#define SDR_CB_DMA1INT (1 << 9)
#define SDR_CB_IRQINT  (1 << 10)

unsigned int sdrcbFunc (void *data, int size);

volatile static SdrEECBData eeCBData;
volatile static SdrEECBData eeCBDataSend;
volatile int gStThid;
int bid;
sceSifClientData cd;

void
sceSifCmdLoop2 (void)
{
    int oldei;
    
    /* コマンドを待つ処理 */
    while (1) {
	if (eeCBData.mode != 0) {
	resend:
	    /* copy interrupt information */
	    CpuSuspendIntr (&oldei);
	    memcpy ((void *)&eeCBDataSend, (void *)&eeCBData, sizeof(SdrEECBData));
	    CpuResumeIntr (oldei);

	    /* send interrupt information to EE */
	    sceSifCallRpc (&cd, 0, 0, (void *)&eeCBDataSend, sizeof(SdrEECBData), NULL, 0, NULL,0);

	    /* check additional interrupt(s) */
	    CpuSuspendIntr (&oldei);
	    PRINTF(("sceSifCallRpc  IOP -> EE :command %d\n",eeCBDataSend.mode));
	    if (eeCBData.mode != eeCBDataSend.mode) {
		eeCBData.mode &= ~(eeCBDataSend.mode); /* clear old interrupt info */
		CpuResumeIntr (oldei);
		goto resend;
	    }
	    /* clear interrupt information */
	    eeCBData.mode = 0;
	    iCancelWakeupThread (TH_SELF);	/* 溜っていた wakeup 要求をクリア */
	    CpuResumeIntr (oldei);
	}
	/* 次のコマンドが来るまで眠る */
	PRINTF(("******* Sleep *********\n"));
	SleepThread();
	PRINTF(("******* Wake UP *********\n"));
    }
    return;
}

#ifdef SCE_OBSOLETE
int
_sce_sdrDMA0CallBackProc (void * common)
{
    eeCBData.mode |= SDR_CB_DMA0;
    iWakeupThread (gStThid);
    return 1;
}

int
_sce_sdrDMA1CallBackProc (void * common)
{
    eeCBData.mode |= SDR_CB_DMA1;
    iWakeupThread (gStThid);
    return 1;
}

int
_sce_sdrIRQCallBackProc (void * common)
{
    eeCBData.mode |= SDR_CB_IRQ;
    iWakeupThread (gStThid);
    return 1;
}
#endif

int
_sce_sdrDMA0IntrHandler (int core, void *common)
{
    eeCBData.mode |= SDR_CB_DMA0INT;
    iWakeupThread (gStThid);
    return NEXT_ENABLE;
}

int
_sce_sdrDMA1IntrHandler (int core, void *common)
{
    eeCBData.mode |= SDR_CB_DMA1INT;
    iWakeupThread (gStThid);
    return NEXT_ENABLE;
}

int
_sce_sdrSpu2IntrHandler (int core_bit, void *common)
{
    eeCBData.mode |= SDR_CB_IRQINT;
    eeCBData.voice_bit = core_bit;
    iWakeupThread (gStThid);
    return NEXT_ENABLE;
}

int
sce_sdrcb_loop (void)
{
    int i;

    eeCBData.mode = 0;

    /* callback用RPCエントリのバインド */
    PRINTF (("sceSifBindCmd(sce_SDRST_CB) start \n"));
    while(1) {
	if (sceSifBindRpc (&cd, sce_SDRST_CB, 0) < 0) {
	    printf("error \n");
	    while(1);
	}
	i = 10000;
	while (i--) {
	}
	if (cd.serve != 0) break;
    }
    PRINTF (("sceSifBindRpc completed \n"));
    PRINTF (("goto spu stream cmd loop\n"));
    sceSifCmdLoop2 ();
    
    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */

