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
 *                      rs2drv.irx - rsd_com.c
 *                initialize & command dispatch routine
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   0.30      Jun.17.1999     morita    first checked in
 *   0.50      Oug.10.1999     morita    SpuStSetCore etc. added.
 *             May.14.2000     kaol      Thread priority changed.
 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <libsd.h>
#include <sdrcmd.h>

#include "sdr_i.h"

int gRpcArg [16];	/* EEから転送されるRPCの引数の受け口 */
extern int bid;						/* sdd_cb.c */
extern volatile int gStThid;				/* sdd_cb.c */
extern int sce_sdrcb_loop (void);			/* sdd_cb.c */
extern int sceSdrChangeThreadPriority (int, int);	/* sdd_main.c */

static void *sdrFunc (unsigned int fno, void *data, int size);

/* ユーザ関数 */
static volatile sceSdrUserCommandFunction _sceSdr_vUserCommandFunction [rSdUserCommandMaxNum];

/* ------------------------------------------------------------------------
   sdrモジュールのメインスレッド。
   実行後、割り込み環境の初期化とコマンドの登録を行い、以後はEEからリクエス
   トがあるまでウエイトする。
   ------------------------------------------------------------------------*/
int
sce_sdr_loop (void)
{
    int i;
    sceSifQueueData qd;
    sceSifServeData sd;

    /* SIF RPC の初期化 */
    sceSifInitRpc (0);

    /* リクエストによってコールされる関数を登録 */
    sceSifSetRpcQueue (&qd, GetThreadId ());
    sceSifRegisterRpc (&sd, sce_SDR_DEV, sdrFunc, (void *)gRpcArg, NULL, NULL, &qd);
    PRINTF (("goto sdr cmd loop\n"));

    /* ユーザ登録関数テーブルの初期化 */
    for (i = 0; i < rSdUserCommandMaxNum; i ++) {
	_sceSdr_vUserCommandFunction [i] = (sceSdrUserCommandFunction) NULL;
    }

    /* コマンド待ちループ */
    sceSifRpcLoop (&qd);

    return 0;
}

/* ------------------------------------------------------------------------
   EEからのリクエストによって起こされる関数。
   引数は*dataに格納されている。先頭4バイトは予備用で使われていない。
   この関数の返値が、EE側のRPCの返値になる。
   引数が構造体の場合は、gRpcDataに送られているのでそれを使用する。
   構造体をEEに返す場合は、第１引数のアドレス（EE側）に値を送る。
   ------------------------------------------------------------------------*/
int ret = 0;
sceSdEffectAttr e_attr;
u_int procbat_returns [16 * 24]; /* 返り値の保存場所 */

static void *
sdrFunc (unsigned int command, void *data, int size)
{ 
    struct ThreadParam param;
    int fid;

    /* asm volatile ("break 1"); */

    PRINTF ((" sdrfunc %x, %x, %x, %x\n",
	     *((int *)data + 0), *((int *)data + 1),
	     *((int *)data + 2), *((int *)data + 3)));

    ret = 0;
    switch (command & 0xFFF0) {
    case rSdInit:
	ret = sceSdInit (*((int *)data + 1));
	break;
    case rSdSetParam:
	sceSdSetParam (*((int *)data + 1), *((int *)data + 2));
	break;
    case rSdSetSwitch:
	sceSdSetSwitch (*((int *)data + 1), *((int *)data + 2));
	break;
    case rSdSetAddr:
	sceSdSetAddr (*((int *)data + 1), *((int *)data + 2));
	break;
    case rSdSetCoreAttr:
	sceSdSetCoreAttr (*((int *)data + 1), *((int *)data + 2));
	break;
    case rSdGetParam:
	ret = sceSdGetParam (*((int *)data + 1));
	break;
    case rSdGetSwitch:
	ret = sceSdGetSwitch (*((int *)data + 1));
	break;
    case rSdGetAddr:
	ret = sceSdGetAddr (*((int *)data + 1));
	break;
    case rSdGetCoreAttr:
	ret = sceSdGetCoreAttr (*((int *)data + 1));
	break;
	
    case rSdNote2Pitch:
	ret = sceSdNote2Pitch (*((int *)data + 1), *((int *)data + 2),
			       *((int *)data + 3), *((int *)data + 4));
	break;
    case rSdPitch2Note:
	ret = sceSdPitch2Note (*((int *)data + 1), *((int *)data + 2),
			       *((int *)data + 3));
	break;

    case rSdProcBatch:
	ret = sceSdProcBatch ((sceSdBatch *)*((int *)data + 1),
			      (u_int *)*((int *)data + 2),
			      *((int *)data + 3));
	break;
    case rSdProcBatchEx:
	ret = sceSdProcBatchEx ((sceSdBatch *)*((int *)data + 1),
				(u_int *)*((int *)data + 2),
				*((int *)data + 3), *((int *)data + 4));
	break;
    case rSdProcBatch2:
	ret = sceSdProcBatch (((sceSdBatch *)data) + 1, procbat_returns + 1,
			      ((sceSdBatch *)data)->entry);
	procbat_returns [0] = ret;
	break;
    case rSdProcBatchEx2:
	ret = sceSdProcBatchEx (((sceSdBatch *)data) + 1, procbat_returns + 1,
				((sceSdBatch *)data)->entry,
				((sceSdBatch *)data)->value);
	procbat_returns [0] = ret;
	break;
    case rSdVoiceTrans:
	ret = sceSdVoiceTrans (*((int *)data + 1), *((int *)data + 2), 
			       (u_char *)*((int *)data + 3),
			       (u_int)*((int *)data + 4),
			       *((int *)data + 5));
	break;
    case rSdBlockTrans:
	ret = sceSdBlockTrans (*((int *)data + 1), *((int *)data + 2),
			       (u_char *)*((int *)data + 3),
			       *((int *)data + 4),
			       (u_char *)*((int *)data + 5));
	break;
    case rSdVoiceTransStatus:
	ret = sceSdVoiceTransStatus (*((int *)data + 1), *((int *)data + 2));
	break;
    case rSdBlockTransStatus:
	ret = sceSdBlockTransStatus (*((int *)data + 1), *((int *)data + 2));
	break;
    case rSdStopTrans:
	ret = sceSdStopTrans (*((int *)data + 1));
	break;
#ifdef SCE_OBSOLETE
    case rSdSetTransCallback:
	if (*((int *)data + 1) == 0){
	    /* DMA channel 0 */
	    if (*((int *)data + 2) == (int)NULL) { /* remove interrupt handler */
		ret = (int)sceSdSetTransCallback (0, (SD_TRANS_CBProc)NULL);
	    } else {
		ret = (int)sceSdSetTransCallback (0, (SD_TRANS_CBProc)_sce_sdrDMA0CallBackProc);
	    }
	} else {
	    /* DMA channel 1 */
	    if (*((int *)data + 2) == (int)NULL) { /* remove interrupt handler */
		ret = (int)sceSdSetTransCallback (1, (SD_TRANS_CBProc)NULL);
	    } else {
		ret = (int)sceSdSetTransCallback (1, (SD_TRANS_CBProc)_sce_sdrDMA1CallBackProc);
	    }
	}
	break;
    case rSdSetIRQCallback:
	if (*((int *)data + 1) == (int)NULL) { /* remove interrupt handler */
	    ret = (int)sceSdSetIRQCallback ((SD_IRQ_CBProc)NULL);
	} else {
	    ret = (int)sceSdSetIRQCallback ((SD_IRQ_CBProc)_sce_sdrIRQCallBackProc);
	}
	break;
#endif
    case rSdSetTransIntrHandler:
	if (*((int *)data + 1) == 0) {
	    /* DMA channel 0 */
	    if (*((int *)data + 2) == (int)NULL) { /* remove interrupt handler */
		ret = (int) sceSdSetTransIntrHandler (0, (sceSdTransIntrHandler)NULL, (void *)NULL);
	    } else {
		ret = (int) sceSdSetTransIntrHandler (0, _sce_sdrDMA0IntrHandler, (void *)NULL);
	    }
	} else {
	    /* DMA channel 1 */
	    if (*((int *)data + 2) == (int)NULL) { /* remove interrupt handler */
		ret = (int) sceSdSetTransIntrHandler (1, (sceSdTransIntrHandler)NULL, (void *)NULL);
	    } else {
		ret = (int) sceSdSetTransIntrHandler (1, _sce_sdrDMA1IntrHandler, (void *)NULL);
	    }
	}
	break;
    case rSdSetSpu2IntrHandler:
	if (*((int *)data + 1) == (int)NULL) { /* remove interrupt handler */
	    ret = (int) sceSdSetSpu2IntrHandler ((sceSdSpu2IntrHandler)NULL, (void *)NULL);
	} else {
	    ret = (int) sceSdSetSpu2IntrHandler (_sce_sdrSpu2IntrHandler, (void *)NULL);
	}
	break;

    case rSdSetEffectAttr:
	ret = sceSdSetEffectAttr  (command & 0xf, (sceSdEffectAttr *)((int *)data));
	break;
    case rSdGetEffectAttr:
	(void)sceSdGetEffectAttr (command & 0xf, &e_attr);
	break;
    case rSdSetEffectMode:
	ret = sceSdSetEffectMode (command & 0xf, (sceSdEffectAttr *)((int *)data));
	break;
    case rSdSetEffectModeParams:
	ret = sceSdSetEffectModeParams (command & 0xf, (sceSdEffectAttr *)((int *)data));
	break;
    case rSdClearEffectWorkArea:
	ret = sceSdClearEffectWorkArea (*((int *)data + 1), *((int *)data + 2),
					*((int *)data + 3));
	break;
    case rSdCleanEffectWorkArea:
	ret = sceSdCleanEffectWorkArea (*((int *)data + 1), *((int *)data + 2),
					*((int *)data + 3));
	break;

    case rSdChangeThreadPriority:
	ret = sceSdrChangeThreadPriority (*((int *)data + 1), *((int *)data + 2));
	break;

    case rSdUserCommand0:
    case rSdUserCommand1:
    case rSdUserCommand2:
    case rSdUserCommand3:
    case rSdUserCommand4:
    case rSdUserCommand5:
    case rSdUserCommand6:
    case rSdUserCommand7:
    case rSdUserCommand8:
    case rSdUserCommand9:
    case rSdUserCommandA:
    case rSdUserCommandB:
    case rSdUserCommandC:
    case rSdUserCommandD:
    case rSdUserCommandE:
    case rSdUserCommandF:
	fid = sceSdrGetUserCommandNumber (command);
	if (_sceSdr_vUserCommandFunction [fid] != (sceSdrUserCommandFunction) NULL) {
	    ret = (_sceSdr_vUserCommandFunction [fid]) (command, data, size);
	}
	break;
 
    case 0xe620:
	/* sceSdRemoteCallbackInit()から呼ばれて、callback用のスレッド
	 * （sce_sdrcb_loop) をcreate/startする。
	 * sce_sdrcb_loopでは、EE側コールバック関数のbindを行なって
	 * からコマンド待ちループに入る。 */
	param.attr         = TH_C;
	param.entry        = sce_sdrcb_loop;
	param.initPriority = initial_priority_cb;
	param.stackSize    = 0x800;
	param.option       = 0;
	gStThid = CreateThread (&param);
	StartThread (gStThid,0);
	printf ("SDR callback thread created\n");
	break;

    default:
	printf ("SDR driver ERROR: unknown command %x \n", command & 0xFFF0);
	break;
    }

    PRINTF (("return value = %x \n", ret)); 
    switch (command & 0xFFF0) {
    case rSdGetEffectAttr:
	return ((void *)(&e_attr));
    case rSdProcBatch2:
    case rSdProcBatchEx2:
	return ((void *)(procbat_returns));
    default:
	return ((void *)(&ret));
    }
}

sceSdrUserCommandFunction
sceSdrSetUserCommandFunction (int command, sceSdrUserCommandFunction func)
{
    int fid;
    sceSdrUserCommandFunction f;

    if (command < rSdUserCommandMin || command > rSdUserCommandMax) {
       return ((sceSdrUserCommandFunction)(-1));
    }

    fid = sceSdrGetUserCommandNumber (command);
    f = _sceSdr_vUserCommandFunction [fid];
    _sceSdr_vUserCommandFunction [fid] = func;

    PRINTF (("SDR command set (%d) %08x %08x\n",
            fid, func, _sceSdr_vUserCommandFunction [fid]));

    return f;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
