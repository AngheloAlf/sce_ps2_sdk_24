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
 *                        main funcitons
 *
 *     Version    Date          Design     Log
 *  --------------------------------------------------------------------
 *     0.30       Jun.17.1999   morita     provisional version.
 *     0.50       Aug.19.1999   morita     rewrite for new siflib.
 */

#include <eekernel.h>
#include "sifcmd.h"
#include "sifrpc.h"
#include <sif.h>
#include <librspu2.h>
#include <rspu2cmd.h>
#include <stdarg.h>
#include "rs_i.h"

#define STACK_SIZE 0x10
#define DATA_SIZE_B (64)
#define UNCHASH 0x20000000

static u_long128 stack[STACK_SIZE];
static u_int sbuff[16] __attribute__((aligned (64)));
sceSifClientData gCd;

int sceSpu2Remote( int arg, ... );
extern void (*gStPrepareCB)( unsigned int, int  );
extern void (*gStTransCB)( unsigned int );
extern void (*gStFinishCB)( unsigned int, int );
extern int  (*gDMA0CB)( void );
extern int  (*gDMA1CB)( void );
extern int  (*gIRQCB)( void );


/* ------------------------------------------------------------------------
   SPU2リモート環境の初期化。ここでIOP側RPC関数のBindを行なう。
   ------------------------------------------------------------------------*/
int sceSpu2RemoteInit( void )
{
	int fd, i,cid;
	char *cp;
	static  SpuCommonAttr c_attr;

	sceSifInitRpc(0);

	while(1) {
		if (sceSifBindRpc( &gCd, sce_SPU_DEV, 0) < 0) {
			printf("error \n");
			while(1);
		}
		i = 10000;
		while( i-- ){
		}
		if(gCd.serve != 0) break;
	}

	FlushCache(0); //--- 念のため
	sceSpu2Remote( 1, _rSpuInit  );

	return 1;
}

sceSifEndFunc gEnd_func = NULL;


/* ------------------------------------------------------------------------
   SIFによる転送が終了した時に発生する、割り込みハンドラを設定。
   ------------------------------------------------------------------------*/
sceSifEndFunc sceSpu2RemoteCallBack( sceSifEndFunc end_func )
{
	gEnd_func = end_func;
	return ;
}


/* ------------------------------------------------------------------------
   IOP側のlibspu,libsndの関数をリモート実行する。
   ------------------------------------------------------------------------*/
int sceSpu2Remote( int arg, ... )
{
	int ret, i, isBlock, command;
	sceSifDmaData transData;
	va_list ap;
	sceSifEndFunc end_func = NULL;

	va_start( ap, arg );
	isBlock = arg;
	command = va_arg(ap, int);
	sbuff[0] = (int)sbuff;
	for( i = 1; i < 7; i++ ){
		sbuff[i] = va_arg(ap, int);
	}


	if( isBlock == 0 ){
		isBlock = SIF_RPCM_NOWAIT;
		end_func = gEnd_func;
	}
	else{
		isBlock = 0;
	}

	if( command >= rSpuSetTransferCallback )
	{
		if( command == rSpuStSetPreparationFinishedCallback ){
			gStPrepareCB = (void*)sbuff[1];
			return 0;
		}
		else if( command == rSpuStSetTransferFinishedCallback ){
			gStTransCB = (void*)sbuff[1];
			return 0;
		}
		else if( command == rSpuStSetStreamFinishedCallback ){
			gStFinishCB = (void*)sbuff[1];
			return 0;
		}
		else if( command == rSpuSetTransferCallback ){
			gDMA1CB = (void*)sbuff[1];
		}
		else if( command == rSpuAutoDMASetCallback ){
			gDMA0CB = (void*)sbuff[1];
		}
		else if( command == rSpuSetIRQCallback ){
			gIRQCB = (void*)sbuff[1];
		}
	}
	else if( command == rSpuGetVoiceAttr)
	{
		//--- 第2引数にボイスマスクを与える
		sbuff[2] = ((SpuVoiceAttr*)sbuff[1])->voice;
		PRINTF(("s_attr.voice = %x\n", sbuff[2] ));
	}

	PRINTF(("sceSifCallRpc start - "));
	PRINTF(("send value = %x %x, %x, %x\n", sbuff[0], sbuff[1], sbuff[2]));

	if( (command) == (rSpuSetMultiVoiceAttr) ){
		//--- rSpuSetMultiVoiceAttrの時
		//--- 転送する個数が第二引数にある
		printf("rSpuSetMultiVoiceAttr\n");
		sceSifCallRpc( &gCd,  command, isBlock, (void *)(sbuff[1]), 
			sbuff[2]*64, NULL, 0, 
			end_func, (void *)(sbuff[0]));
	}
	else if( (command&0xF000) == (rSpuGetCommonAttr&0xF000) ){
		//--- GET系で構造体に値が返ってくるもの
		sceSifCallRpc( &gCd,  command, isBlock, (void *)(&sbuff[0]), 
			DATA_SIZE_B, (void *)(sbuff[1]), 64, 
			end_func, (void *)(sbuff[1]));
	}
	else if( (command&0xF000) == (rSpuSetCommonAttr&0xF000) ){
		//--- SET系で構造体で値を渡すもの
		sceSifCallRpc( &gCd,  command, isBlock, (void *)(sbuff[1]), 
			DATA_SIZE_B, NULL, 0, 
			end_func, (void *)(sbuff[0]));
	}
	else{
		sceSifCallRpc( &gCd,  command, isBlock, (void *)(&sbuff[0]), 
			DATA_SIZE_B, (void *)(&sbuff[0]), 16,
			end_func, (void *)(&sbuff[0]));
		ret   = sbuff[0];
	}


	PRINTF(("sceSifCallRpc cmplete \n"));

	va_end(ap);
	return ret;
}

