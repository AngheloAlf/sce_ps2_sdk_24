/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *                      Emotion Engine Library
 *                          Version 0.50
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                        librspu2 - rs_st.c
 *                   interrupt callback functions
 *
 *     Version    Date          Design     Log
 *  --------------------------------------------------------------------
 *     0.30       Jun.17.1999   morita     provisional version.
 *     0.50       Aug.18.1999   morita     rewrite for new siflib.
 */

#include <eekernel.h>
#include "sifcmd.h"
#include <sif.h>
#include <librspu2.h>
#include <rspu2cmd.h>
#include <stdarg.h>
#include "rs_i.h"
#include "sifrpc.h"

#define STACK_SIZE 0x10

static u_long128 stack[STACK_SIZE];
static SpuEECBData gEECBData __attribute__((aligned (64)));

static void _spuCBThread(void *data );
static unsigned int _spuCB(unsigned int fnd, void *data, int size);

void (*gStPrepareCB)( unsigned int, int  );
void (*gStTransCB)( unsigned int );
void (*gStFinishCB)( unsigned int, int );
int (*gDMA0CB)( void );
int (*gDMA1CB)( void );
int (*gIRQCB)( void );




/* ------------------------------------------------------------------------
   IOP側では、割り込みが入った時にEE側にRPCして_spuCB() を起こそうとする。
   そのためのThreadをここで作成する。
   ------------------------------------------------------------------------*/
int sceSpu2CallbackInit( int priority )
{
	int i, ret;
	struct ThreadParam tp;
	static char stack[256*16] __attribute__ ((aligned(16)));

	sceSpu2Remote( 1, 0xe621 ); //IOP側でBindする

	tp.entry = (void*)_spuCBThread;
	tp.stack = stack;
	tp.stackSize = sizeof(stack);
	tp.initPriority = priority;
	tp.gpReg = &_gp;
	tp.option = CreateThread( &tp );

	i = StartThread( tp.option, (void*)NULL );
	if( i < 0 ){ PRINTF(("Can't start thread for streaming.\n"));return -1; }

	return tp.option;
}


/* ------------------------------------------------------------------------
   librspu2のコールバック用スレッド。
   起動されるとコマンドの登録を行い、以後はIOPからリクエストがあるまでウエ
   イトする。
   ------------------------------------------------------------------------*/
static void _spuCBThread(void *data )
{
	sceSifQueueData	qd;
	sceSifServeData	sd;

	PRINTF(("_spuCBThread \n"));

	sceSifInitRpc(0);
	sceSifSetRpcQueue( &qd,  GetThreadId() );

	sceSifRegisterRpc(&sd, sce_SPUST_CB, (sceSifRpcFunc)_spuCB, (void*)&gEECBData, NULL, NULL, &qd );

	sceSifRpcLoop(&qd);

	return;
}


/* ------------------------------------------------------------------------
   IOPからのRPCによって起こされる関数。
   *dataにはSpuEECBDataの値。
   ------------------------------------------------------------------------*/
static unsigned int _spuCB(unsigned int fnd, void *data, int size)
{

	PRINTF(("**** spuCB %d \n", *((int*)data) ));

	switch( *((int*)data) )
	{
	case SPU_CB_ST_PREPARE:
		(*gStPrepareCB)( *((int*)data+1), *((int*)data+2));
		PRINTF(("======CB  %d, %d \n",  *((int*)data+1),  *((int*)data+2) ));
		break;
	case SPU_CB_ST_TRANS:
		(*gStTransCB)(*((int*)data+1));
		break;
	case SPU_CB_ST_FINISH:
		(*gStFinishCB)(*((int*)data+1),  *((int*)data+2));
		break;
	case SPU_CB_DMA0:
		(*gDMA0CB)();
		break;
	case SPU_CB_DMA1:
		(*gDMA1CB)();
		break;
	case SPU_CB_IRQ:
		(*gIRQCB)();
		break;
	}

	return 0;
}

