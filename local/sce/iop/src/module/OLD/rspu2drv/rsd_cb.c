/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *                    I/O Proseccor sample Library
 *                          Version 0.50
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
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
#include "sif.h"
#include "sifcmd.h"
#include "sifrpc.h"
#include <libspu2.h>
#include <rspu2cmd.h>
#include "rs_i.h"

#define _SCE_IOP_STUB_

SpuStEnv *gStPtr;
SpuStEnv gStBuff;

//#define PRINTF(x) printf x
#define PRINTF(x)


unsigned int  spustFunc(void *data, int size);

volatile static SpuEECBData eeCBData;
volatile int gStThid;
int bid;
sceSifClientData cd;


void sceSifCmdLoop2()
{
  /*コマンドを待つ処理*/
  while(1) {
    if( eeCBData.mode != 0 )
    {
	sceSifCallRpc( &cd, 0, 0, (void*)&eeCBData, sizeof(SpuEECBData), NULL, 0, NULL,0);

	PRINTF(("sceSifCallRpc  IOP -> EE :command %d\n",eeCBData.mode));
	if( eeCBData.mode <= SPU_CB_ST_FINISH ){
		memcpy( (unsigned char*)gStPtr, 
			(unsigned char*)&gStBuff, sizeof(SpuStEnv) );
	}
	eeCBData.mode = 0;
    }
    /* 次のコマンドが来るまで眠る */
	PRINTF(("******* Sleep *********\n"));
	SleepThread();
	PRINTF(("******* Wake UP *********\n"));
  }
  return;
}

int DMA0CallBackProc( void* common )
{
	eeCBData.mode = SPU_CB_DMA0;
	iWakeupThread( gStThid );
	return 1;
}

int DMA1CallBackProc( void* common )
{
	eeCBData.mode = SPU_CB_DMA1;
	iWakeupThread( gStThid );
	return 1;
}

int IRQCallBackProc( void* common )
{
	eeCBData.mode = SPU_CB_IRQ;
	iWakeupThread( gStThid );
	return 1;
}

SpuStCallbackProc
spustCB_preparation_finished (unsigned long voice_bit, long p_status)
{
	PRINTF(("******** spustCB_preparation_finished\n"));
	eeCBData.mode = SPU_CB_ST_PREPARE;
	eeCBData.voice_bit = voice_bit;
	eeCBData.status = p_status;

	iWakeupThread( gStThid );
	return 0;
}


SpuStCallbackProc
spustCB_transfer_finished (unsigned long voice_bit, long t_status)
{
	PRINTF(("******** spustCB_transfer_finished\n"));
	eeCBData.mode = SPU_CB_ST_TRANS;
	eeCBData.voice_bit = voice_bit;
	eeCBData.status = t_status;

	iWakeupThread( gStThid );
	return 0;
}

SpuStCallbackProc
spustCB_stream_finished (unsigned long voice_bit, long s_status)
{
	PRINTF(("******** spustCB_stream_finished\n"));
	eeCBData.mode = SPU_CB_ST_FINISH;
	eeCBData.voice_bit = voice_bit;
	eeCBData.status = s_status;

	iWakeupThread( gStThid );
	return 0;
}



int sce_spust_loop()
{
	int i;
	eeCBData.mode = 0;

	//callback用RPCエントリのバインド
	PRINTF(("sceSifBindCmd(sce_SPUST_CB) start \n"));
	while(1) {
		if (sceSifBindRpc( &cd, sce_SPUST_CB, 0) < 0) {
			printf("error \n");
			while(1);
		}
		i = 10000;
		while( i-- ){
		}
		if(cd.serve != 0) break;
	}
	PRINTF(("sceSifBindRpc completed \n"));
	PRINTF(("goto spu stream cmd loop\n"));
	sceSifCmdLoop2();

	return 0;
}


/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */

