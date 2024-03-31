/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                  I/O Proseccor sample program
 *                          Version 1.20
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                      ezbgm.irx - bgm_com.c
 *                    command dispatch routine
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   1.20      Nov.23.1999     morita    first checked in.
 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include "sif.h"
#include "sifcmd.h"
#include "sifrpc.h"
#include <libsd.h>
#include "bgm_i.h"

int gRpcArg [16];	//--- EEから転送されるRPCの引数の受け口

static void* bgmFunc(unsigned int fno, void *data, int size);

extern int  BgmInit (int ch, int allocsize);
extern void BgmQuit (int channel, int status);
extern int  BgmOpen (int ch, char *filename);
extern void BgmClose (int ch, int status);
extern int  BgmPreLoad (int ch, int status);
extern void BgmStart (int ch, int status);
extern void BgmStop (int ch, int status);
extern void BgmSeek (int ch, unsigned int vol);
extern void BgmSetVolume (int ch, unsigned int vol);
extern int BgmRaw2Spu (int ch, int which, int mode);
extern void BgmSetVolumeDirect (int ch, unsigned int vol);
extern void BgmSetMasterVolume (int ch, unsigned int vol);
extern unsigned int BgmGetMode (int ch, int status);
extern void BgmSetMode (int ch, u_int mode);
extern void BgmSdInit (int ch, int status);

/* ------------------------------------------------------------------------
   ezbgmモジュールのメインスレッド。
   実行後、割り込み環境の初期化とコマンドの登録を行い、以後はEEからリクエス
   トがあるまでウエイトする。
   ------------------------------------------------------------------------*/
int
sce_bgm_loop (void)
{
    sceSifQueueData qd;
    sceSifServeData sd;

    //--- リクエストによってコールされる関数を登録
    sceSifInitRpc(0);

    sceSifSetRpcQueue (&qd, GetThreadId());
    sceSifRegisterRpc (&sd, EZBGM_DEV, bgmFunc, (void*)gRpcArg, NULL, NULL, &qd);
    PRINTF(("goto bgm cmd loop\n"));

    //--- コマンド待ちループ
    sceSifRpcLoop(&qd);

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

static void *
bgmFunc (unsigned int command, void *data, int size)
{ 
    int ch;

    PRINTF ((" bgmfunc %x, %x, %x, %x\n",
	     *((int *)data + 0), *((int *)data + 1),
	     *((int *)data + 2), *((int *)data + 3)));

    ch = command & 0x000f;
	
    switch (command & 0xfff0) {
    case EzBGM_INIT:				/* 初期化処理 */
	ret = BgmInit (ch, *((int *)data));
	break;
    case EzBGM_QUIT:				/* 終了処理 */
	BgmQuit (ch, *((int *)data));
	break;
    case EzBGM_OPEN:				/* 波形データファイルをオープン */
	ret = BgmOpen (ch, (char *)((int *)data));
	break;
    case EzBGM_CLOSE:				/* 波形データファイルをクローズ */
	BgmClose (ch, *((int *)data));
	break;
    case EzBGM_PRELOAD:				/* 波形データをバッファ分先読み */
	ret = BgmPreLoad (ch, *((int *)data));
	break;
    case EzBGM_START:				/* 演奏開始 */
	BgmStart (ch, *((int *)data));
	break;
    case EzBGM_STOP:				/* 演奏停止 */
	BgmStop (ch, *((int *)data));
	break;
    case EzBGM_SEEK:				/* ファイルの読み出し位置の移動 */
	BgmSeek (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_SETVOL:				/* ボリューム設定 */
	BgmSetVolume (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_SETVOLDIRECT:			/* ボリュームを即座に変更 */
	BgmSetVolumeDirect (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_SETMASTERVOL:			/* マスターボリュームの変更 */
	BgmSetMasterVolume (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_SETMODE:				/* ステレオ/モノラルの設定 */
	BgmSetMode (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_GETMODE:				/* ステレオ/モノラルの現在値の取得 */
	ret = BgmGetMode (ch, *((int *)data));
	break;
    case EzBGM_SDINIT:				/* 低レベルサウンドライブラリの初期化 */
	BgmSdInit (ch, *((int *)data));
	break;
    default:
	ERROR(("EzBGM driver error: unknown command %d \n", *((int *)data)));
	break;
    }
    PRINTF(("return value = %x \n", ret)); 

    return (void *)(&ret);
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
