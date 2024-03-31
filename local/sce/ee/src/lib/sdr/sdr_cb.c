/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/* 
 *                      Emotion Engine Library
 *                          Version 0.50
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2002 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                        libsdr - sdr_cb.c
 *                   interrupt callback functions
 *
 *     Version    Date          Design     Log
 *  --------------------------------------------------------------------
 *     0.60       Oct.12.1999   morita     first checked in.
 */

#include <eekernel.h>
#include "sifcmd.h"
#include <sif.h>
#include <stdarg.h>
#include <sdmacro.h>
#include "sifrpc.h"
#include "sdr_i.h"

#define STACK_SIZE 0x10

static u_long128 stack [STACK_SIZE];
static SdrEECBData gEECBData __attribute__((aligned (64)));

static void _sdrCBThread (void *data);
static unsigned int _sdrCB (unsigned int fnd, void *data, int size);

int (*_sce_sdr_gDMA0CB)(void) = (int (*)(void)) NULL;
int (*_sce_sdr_gDMA1CB)(void) = (int (*)(void)) NULL;
int (*_sce_sdr_gIRQCB)(void)  = (int (*)(void)) NULL;

sceSdTransIntrHandler _sce_sdr_transIntr0Hdr = (sceSdTransIntrHandler) NULL;
sceSdTransIntrHandler _sce_sdr_transIntr1Hdr = (sceSdTransIntrHandler) NULL;
sceSdSpu2IntrHandler  _sce_sdr_spu2IntrHdr   = (sceSdSpu2IntrHandler)  NULL;

void *_sce_sdr_transIntr0Arg = (void *) NULL;
void *_sce_sdr_transIntr1Arg = (void *) NULL;
void *_sce_sdr_spu2IntrArg   = (void *) NULL;

/* ------------------------------------------------------------------------
   IOP側では、割り込みが入った時にEE側にRPCして_sdrCB() を起こそうとする。
   そのためのThreadをここで作成する。
   ------------------------------------------------------------------------*/
int
sceSdRemoteCallbackInit (int priority)
{
    int i, ret;
    struct ThreadParam tp;
    static char stack [256 * 16] __attribute__ ((aligned(64)));

    _sce_sdr_gDMA0CB = (int (*)(void)) NULL;
    _sce_sdr_gDMA1CB = (int (*)(void)) NULL;
    _sce_sdr_gIRQCB  = (int (*)(void)) NULL;
    _sce_sdr_transIntr0Hdr = (sceSdTransIntrHandler) NULL;
    _sce_sdr_transIntr1Hdr = (sceSdTransIntrHandler) NULL;
    _sce_sdr_spu2IntrHdr   = (sceSdSpu2IntrHandler)  NULL;

    _sce_sdr_transIntr0Arg = (void *) NULL;
    _sce_sdr_transIntr1Arg = (void *) NULL;
    _sce_sdr_spu2IntrArg   = (void *) NULL;

    sceSdRemote (1, 0xe620); //IOP側でBindする

    tp.entry = (void*)_sdrCBThread;
    tp.stack = stack;
    tp.stackSize = sizeof(stack);
    tp.initPriority = priority;
    tp.gpReg = &_gp;
    tp.option = CreateThread (&tp);

    i = StartThread (tp.option, (void*)NULL);
    if (i < 0){
	scePrintf("Can't start thread for streaming.\n");
	return -1;
    }

    return tp.option;
}

/* ------------------------------------------------------------------------
   libsdrのコールバック用スレッド。
   起動されるとコマンドの登録を行い、以後はIOPからリクエストがあるまでウエ
   イトする。
   ------------------------------------------------------------------------*/
static void
_sdrCBThread (void *data)
{
    sceSifQueueData qd;
    sceSifServeData sd;

    PRINTF (("_sdrCBThread \n"));

    sceSifInitRpc (0);
    sceSifSetRpcQueue (&qd,  GetThreadId ());

    sceSifRegisterRpc (&sd, sce_SDRST_CB, (sceSifRpcFunc)_sdrCB,
		       (void *)&gEECBData, NULL, NULL, &qd);
    sceSifRpcLoop(&qd);

    return;
}

// please sync with `sdrdrv'
#ifdef SCE_OBSOLETE
#define SDR_CB_DMA0    (1 << 0)
#define SDR_CB_DMA1    (1 << 1)
#define SDR_CB_IRQ     (1 << 2)
#endif
#define SDR_CB_DMA0INT (1 << 8)
#define SDR_CB_DMA1INT (1 << 9)
#define SDR_CB_IRQINT  (1 << 10)

/* ------------------------------------------------------------------------
   IOPからのRPCによって起こされる関数。
   *dataにはSdrEECBDataの値。
   ------------------------------------------------------------------------*/
static unsigned int
_sdrCB (unsigned int fnd, void *data, int size)
{
    PRINTF(("**** sdrCB %d (%d)\n",
	    *((int *)data), ((SdrEECBData *)data)->mode));

#ifdef SCE_OBSOLETE
    if ((*((int *)data)) & SDR_CB_DMA0)
	if (_sce_sdr_gDMA0CB)
	    (*_sce_sdr_gDMA0CB)();
    if ((*((int *)data)) & SDR_CB_DMA1)
	if (_sce_sdr_gDMA1CB)
	    (*_sce_sdr_gDMA1CB)();
    if ((*((int *)data)) & SDR_CB_IRQ)
	if (_sce_sdr_gIRQCB)
	    (*_sce_sdr_gIRQCB)();
#endif
    if ((*((int *)data)) & SDR_CB_DMA0INT)
	if (_sce_sdr_transIntr0Hdr)
	    (*_sce_sdr_transIntr0Hdr)(0, _sce_sdr_transIntr0Arg);
    if ((*((int *)data)) & SDR_CB_DMA1INT)
	if (_sce_sdr_transIntr1Hdr)
	    (*_sce_sdr_transIntr1Hdr)(1, _sce_sdr_transIntr1Arg);
    if ((*((int *)data)) & SDR_CB_IRQINT)
	if (_sce_sdr_spu2IntrHdr)
	    (*_sce_sdr_spu2IntrHdr) ((((SdrEECBData *) data)->voice_bit),
				     _sce_sdr_spu2IntrArg);

    return 0;
}

