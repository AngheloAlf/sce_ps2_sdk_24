/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *                    I/O Proseccor sample Library
 *                          Version 0.61
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                      rs2drv.irx - rsd_com.c
 *                initialize & command dispatch routine
 *
 *   Version   Date            Design   Log
 *  --------------------------------------------------------------------
 *   0.30      Jun.17.1999     morita   first checked in
 *   0.50      Aug.10.1999     morita   SpuStSetCore etc. added.
 *   0.60      Spt.25.1999     morita   rSpuSetMultiVoiceAttr etc. added.
 *   0.61      Feb.27.2000     kaol     rSpuAutoDMAWrite changed.
 */


#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include "sif.h"
#include "sifcmd.h"
#include "sifrpc.h"
#include <libspu2.h>
#include <libsnd2.h>
#include "rs_i.h"
#include <rspu2cmd.h>

#define _SCE_IOP_STUB_
//#define PRINTF(x) printf x
#define PRINTF(x)

#define VOICE_LIMIT 2
#define SPU_MALLOC_MAX		128 //キメうち
#define BASE_priority  32

//int gRpcArg[16];	//--- EEから転送されるRPCの引数の受け口
int gRpcArg[16*24];	//--- EEから転送されるRPCの引数の受け口
char seq_table[SS_SEQ_TABSIZ * 4 * 5]; /* seq data table */
char spu_malloc_rec [SPU_MALLOC_RECSIZ * (SPU_MALLOC_MAX + 1)];
extern int bid;
extern SpuStEnv *gStPtr;
extern SpuStEnv gStBuff;
extern volatile int gStThid;
extern int sce_spust_loop();

static void* spuFunc(unsigned int fno, void *data, int size);

extern SpuStCallbackProc spustCB_preparation_finished (unsigned long voice_bit, long p_status);
extern SpuStCallbackProc spustCB_transfer_finished (unsigned long voice_bit, long t_status);
extern SpuStCallbackProc spustCB_stream_finished (unsigned long voice_bit, long s_status);
extern int  DMA0CallBackProc( void* common );
extern int  DMA1CallBackProc( void* common );
extern int  IRQCallBackProc( void* common );


/* ------------------------------------------------------------------------
   rspu2モジュールのメインスレッド。
   実行後、割り込み環境の初期化とコマンドの登録を行い、以後はEEからリクエス
   トがあるまでウエイトする。
   ------------------------------------------------------------------------*/
int sce_spu2_loop()
{
	sceSifQueueData qd;
	sceSifServeData sd;

	//-- 割り込み環境の初期化を行っておく。
	CpuEnableIntr();
	EnableIntr( INUM_DMA_4 );
	EnableIntr( INUM_DMA_7 );
	EnableIntr( 9 );

	if( ! sceSifCheckInit() )
		sceSifInit();
	sceSifInitRpc(0);
	
	//--- リクエストによってコールされる関数を登録

	sceSifSetRpcQueue( &qd, GetThreadId() );
	sceSifRegisterRpc( &sd, sce_SPU_DEV, spuFunc, (void*)gRpcArg, NULL, NULL, &qd );
	PRINTF(("goto spu cmd loop\n"));

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
SpuCommonAttr c_attr;
SpuVoiceAttr s_attr;
SpuReverbAttr r_attr;
char status[24];
int gMultiVoiceNum = 0;

static void* spuFunc(unsigned int command, void *data, int size)
{ 
	struct ThreadParam param;
	int i;

//	asm volatile( "break 1");

	PRINTF(( " spufunc %x, %x, %x, %x\n", *((int*)data + 0), 
		*((int*)data + 1), *((int*)data + 2),*((int*)data + 3) ));

	switch( command )
	{
	case _rSpuInit:
		SpuInit(); 
		break;
	case rSpuSetCore:
		ret = SpuSetCore( *((int*)data + 1) ); break;
	case rSpuSetCommonAttr:
		SpuSetCommonAttr ( (SpuCommonAttr*)((int*)data) );
		break;
	case rSpuSetVoiceAttr:
		SpuSetVoiceAttr ((SpuVoiceAttr*)((int*)data));
		break;
	case rSpuSetMultiVoiceNum:
		gMultiVoiceNum =  *((int*)data + 1);
		break;
	case rSpuSetMultiVoiceAttr:
		for( i = 0; i < gMultiVoiceNum; i++ ){
			SpuSetVoiceAttr ((SpuVoiceAttr*)((int*)data) + i );
//			printf("rSpuSetMultiVoiceAttr %d addr %x\n", i,  
//				(SpuVoiceAttr*)((int*)data) + i );
		}
//		asm volatile( "break 1");
		break;
	case rSpuSetReverbModeParam:
		SpuSetReverbModeParam ((SpuReverbAttr*)((int*)data));
 		break;
	case rSpuSetKeyOnWithAttr:
		SpuSetKeyOnWithAttr ((SpuVoiceAttr*)((int*)data));
		break;
	case rSpuSetEnv:
		SpuSetEnv ((SpuEnv*)((int*)data));
 		break;
	case rSpuSetReverbEndAddr:
		SpuSetReverbEndAddr( *((int*)data + 1) ); break;
	case rSpuGetReverbEndAddr:
		ret = SpuGetReverbEndAddr(); break;
	case rSpuSetTransferMode:
		ret = SpuSetTransferMode( *((int*)data + 1) ); break;
	case rSpuSetTransferStartAddr:
		ret = SpuSetTransferStartAddr( *((int*)data + 1) ); break;
	case rSpuWrite:
		ret = SpuWrite( (unsigned char*)(*((int*)data + 1)), 
			 *((int*)data + 2) );
		break;
	case rSpuWritePartly:
		ret = SpuWritePartly( (unsigned char*)(*((int*)data + 1)), 
			 *((int*)data + 2) );
		break;
	case rSpuWrite0:
		ret = SpuWrite0( (*((int*)data + 1)) );
		break;
	case rSpuIsTransferCompleted:
		ret = SpuIsTransferCompleted( *((int*)data + 1) ); 
		break;
	case rSpuSetReverb:
		ret = SpuSetReverb( *((int*)data + 1) ); break;
	case rSpuSetReverbModeDepth:
		SpuSetReverbModeDepth( *((int*)data + 1), *((int*)data + 2) ); 
		break;
	case rSpuSetReverbVoice:
		ret = SpuSetReverbVoice( *((int*)data + 1), *((int*)data + 2) ); 		break;
	case rSpuSetKey:
		SpuSetKey( *((int*)data + 1), *((int*)data + 2) ); break;
	case rSpuInitMalloc:
		ret = SpuInitMalloc( *((int*)data + 1), spu_malloc_rec ); break;	case rSpuMalloc:
		ret = SpuMalloc( *((int*)data + 1) ); break;
	case rSpuStSetCore:
		ret = SpuStSetCore( *((int*)data + 1) ); break;
	case rSpuStInit:
		gStPtr = SpuStInit( *((int*)data + 1) ); 
		ret = (int)(&gStBuff);
		(void) SpuStSetPreparationFinishedCallback ((SpuStCallbackProc)
				spustCB_preparation_finished);
		(void) SpuStSetTransferFinishedCallback ((SpuStCallbackProc)
				spustCB_transfer_finished);
		(void) SpuStSetStreamFinishedCallback ((SpuStCallbackProc)
				spustCB_stream_finished);
		break;
	case rSpuStQuit:
		ret = SpuStQuit(); break;
	case rSpuStTransfer:
		memcpy( (unsigned char*)gStPtr, 
			(unsigned char*)&gStBuff, sizeof(SpuStEnv) );
		ret = SpuStTransfer( *((int*)data + 1), *((int*)data + 2) ); 
		break;
	case rSpuGetCommonAttr:
		SpuGetCommonAttr (&c_attr);
		return (void*)(&c_attr);
		break;
	case rSpuGetReverbModeParam:
		SpuGetReverbModeParam (&r_attr);
		return (void*)(&r_attr);
		break;
	case rSpuGetVoiceAttr:
		//-- 第2引数にvoiceマスクを入れるようにしている
		s_attr.voice = *((int*)data + 2);
//		FlushDcache();
		SpuGetVoiceAttr (&s_attr);
		return (void*)(&s_attr);
		break;
	case rSpuGetAllKeysStatus:
		SpuGetAllKeysStatus (&status[0]);
		return (void*)(&(status[0]));
		break;
	case rSpuClearReverbWorkArea:
		ret = SpuClearReverbWorkArea( *((int*)data + 1) ); 
		break;
	case rSpuFlush:
		ret = SpuFlush( *((int*)data + 1) ); 
		break;
	case rSpuFree:
		SpuFree( *((int*)data + 1) ); 
		break;
	case rSpuGetKeyStatus:
		ret = SpuGetKeyStatus (*((int*)data + 1));
		break;
	case rSpuGetIRQ:
		ret = SpuGetIRQ(); 
		break;
	case rSpuGetIRQAddr:
		ret = SpuGetIRQAddr(); 
		break;
	case rSpuGetMute:
		ret = SpuGetMute(); 
		break;
	case rSpuGetNoiseClock:
		ret = SpuGetNoiseClock(); 
		break;
	case rSpuGetNoiseVoice:
		ret = SpuGetNoiseVoice(); 
		break;
	case rSpuGetPitchLFOVoice:
		ret = SpuGetPitchLFOVoice(); 
		break;
	case rSpuGetReverb:
		ret = SpuGetReverb(); 
		break;
	case rSpuGetReverbVoiceb:
		ret = SpuGetReverbVoice(); 
		break;
	case rSpuGetTransferMode:
		ret = SpuGetTransferMode(); 
		break;
	case rSpuGetTransferStartAddr:
		ret = SpuGetTransferStartAddr(); 
		break;
	case rSpuInitHot:
		SpuInitHot(); 
		break;
	case rSpuIsReverbWorkAreaReserved:
		ret = SpuIsReverbWorkAreaReserved( *((int*)data + 1) ); 
		break;
	case rSpuMallocWithStartAddr:
		ret = SpuMallocWithStartAddr( *((int*)data + 1), *((int*)data + 2) ); 
		break;
	case rSpuRead:
		ret = SpuRead( (unsigned char*)(*((int*)data + 1)), 
			 *((int*)data + 2) );
		break;
	case rSpuReadDecodedData:
		ret = SpuReadDecodedData( (SpuDecodedData*)(*((int*)data + 1)), 
			 *((int*)data + 2) );
		break;
	case rSpuReserveReverbWorkArea:
		ret = SpuReserveReverbWorkArea( *((int*)data + 1) ); 
		break;
	case rSpuSetIRQ:
		ret = SpuSetIRQ( *((int*)data + 1) ); 
		break;
	case rSpuSetIRQAddr:
		ret = SpuSetIRQAddr( *((int*)data + 1) ); 
		break;
	case rSpuSetIRQCallback:
		SpuSetIRQCallback( (SpuIRQCallbackProc )IRQCallBackProc );
		break;
	case rSpuSetMute:
		ret = SpuSetMute( *((int*)data + 1) ); 
		break;
	case rSpuSetNoiseClock:
		ret = SpuSetNoiseClock( *((int*)data + 1) ); 
		break;
	case rSpuSetNoiseVoice:
		ret = SpuSetNoiseVoice( *((int*)data + 1), *((int*)data + 2) ); 
		break;
	case rSpuSetPitchLFOVoice:
		ret = SpuSetPitchLFOVoice( *((int*)data + 1), *((int*)data + 2) ); 
		break;
	case rSpuSetTransferCallback:
		SpuSetTransferCallback( (SpuTransferCallbackProc)DMA1CallBackProc );
		break;
	case rSpuAutoDMASetCallback:
		SpuAutoDMASetCallback( (SpuTransferCallbackProc )DMA0CallBackProc );
		break;
	case rSpuStGetStatus:
		ret = SpuStGetStatus(); 
		break;
	case rSpuStGetVoiceStatus:
		ret = SpuStGetVoiceStatus(); 
		break;
	case rSpuAutoDMAWrite:
		ret = SpuAutoDMAWrite((u_char*)(*((int*)data + 1)), *((int*)data + 2), *((int*)data + 3), (u_char*)(*((int*)data + 4))); 
		break;
	case rSpuAutoDMAStop:
		ret = SpuAutoDMAStop(); 
		break;
	case rSpuAutoDMAGetStatus:
		ret = SpuAutoDMAGetStatus(); 
		break;
	case rSpuSetAutoDMAAttr:
		SpuSetAutoDMAAttr( *((int*)data + 1), *((int*)data + 2),*((int*)data + 3), *((int*)data + 4) );
		break;
	case rSpuSetSerialInAttr:
		SpuSetSerialInAttr( *((int*)data + 1), *((int*)data + 2) );
 		break;
	case rSpuSetDegialOut:
		SpuSetDegitalOut( *((int*)data + 1) );
 		break;
	case rSsInit:
		SsInit();
		break;
	case rSsPitchCorrect:
		SsPitchCorrect( *((int*)data + 1) ); break;
	case rSsSetTableSize:
		SsSetTableSize( seq_table, *((int*)data + 2), 
				*((int*)data + 3) ); break;
	case rSsSetTickMode:
		SsSetTickMode( *((int*)data + 1) ); break;
	case rSsVabOpenHead:
		ret = SsVabOpenHead( (u_char *)(*((int*)data + 1)), *((int*)data + 2) ); 
		break;
	case rSsVabOpenHeadSticky:
		ret = SsVabOpenHeadSticky( (u_char *)(*((int*)data + 1)), *((int*)data + 2), *((int*)data + 3) ); 
		break;
	case rSsVabTransCompleted:
		ret = SsVabTransCompleted( *((int*)data + 1) ); break;
	case rSsVabTransBody:
		SsVabTransBody( (u_char *)(*((int*)data + 1)), 
			*((int*)data + 2) ); break;
	case rSsSetMVol:
		SsSetMVol( *((int*)data + 1), *((int*)data + 2) ); break;
	case rSsSeqOpen:
		ret = SsSeqOpen( (u_long *)(*((int*)data + 1)), 
			*((int*)data + 2) ); break;
	case rSsStart:
		SsStart(); break;
	case rSsSeqSetVol:
		SsSeqSetVol( *((int*)data + 1) , *((int*)data + 2), *((int*)data + 3) ); break;
	case rSsSeqPlay:
		SsSeqPlay( *((int*)data + 1), *((int*)data + 2), 
		*((int*)data + 3) ); break;
	case rSsSeqCalledTbyT:
		SsSeqCalledTbyT(); break;
	case rSsSeqClose:
		SsSeqClose( *((int*)data + 1) ); break;
	case rSsVabClose:
		SsVabClose( *((int*)data + 1) ); break;
	case rSsEnd:
		SsEnd(); break;
	case rSsQuit:
		SsQuit(); break;
	case rSsAllocateVoices :
		SsAllocateVoices ( (u_char)(*((int*)data + 1)), (u_char)(*((int*)data + 2)) ); break;
	case rSsBlockVoiceAllocation :
		ret = SsBlockVoiceAllocation (); 
		break;
	case rSsChannelMute:
		SsChannelMute( *((int*)data + 1) , *((int*)data + 2), *((int*)data + 3) ); 
		break;
	case rSsGetActualProgFromProg :
		ret = SsGetActualProgFromProg ( *((int*)data + 1) , *((int*)data + 2) ); 
		break;
	case rSsGetChannelMute :
		ret = SsGetChannelMute ( *((int*)data + 1) , *((int*)data + 2) ); 
		break;
	case rSsGetCurrentPoint :
		ret = (int)SsGetCurrentPoint ( *((int*)data + 1) , *((int*)data + 2) ); 
		break;
	case rSsGetVoiceMask  :
		ret = SsGetVoiceMask  (); 
		break;
	case rSsIsEos  :
		ret = SsIsEos  ( *((int*)data + 1) , *((int*)data + 2) ); 
		break;
	case rSsPitchFromNote   :
		ret = SsPitchFromNote   ( *((int*)data + 1) , *((int*)data + 2) , 
			(u_char)(*((int*)data + 3)) , (u_char)(*((int*)data + 4)) ); 
		break;
	case rSsPlayBack :
		SsPlayBack ( *((int*)data + 1) , *((int*)data + 2), *((int*)data + 3) ); 
		break;
	case rSsQueueKeyOn :
		SsQueueKeyOn  ( *((int*)data + 1) ); 
		break;
	case rSsQueueReverb :
		SsQueueReverb ( *((int*)data + 1) , *((int*)data + 2) ); 
		break;
	case rSsSepClose  :
		SsSepClose  ( *((int*)data + 1)  ); 
		break;
	case rSsSepOpen   :
		ret = SsSepOpen   ( (unsigned long*)((int*)data + 1) , *((int*)data + 2), *((int*)data + 3)  ); 
		break;
	case rSsSepPause    :
		SsSepPause    ( *((int*)data + 1) , *((int*)data + 2)  ); 
		break;
	case rSsSepPlay    :
		SsSepPlay    ( *((int*)data + 1) , *((int*)data + 2) , 
			(u_char)(*((int*)data + 3)) , *((int*)data + 4) ); 
		break;
	case rSsSepReplay  :
		SsSepReplay ( *((int*)data + 1) , *((int*)data + 2)  ); 
		break;
	case rSsSepSetAccelerando    :
		SsSepSetAccelerando    ( *((int*)data + 1) , *((int*)data + 2) , 
			*((int*)data + 3) , *((int*)data + 4) ); 
		break;
	case rSsSepSetCrescendo     :
		SsSepSetCrescendo ( *((int*)data + 1) , *((int*)data + 2) , 
			*((int*)data + 3) , *((int*)data + 4) ); 
		break;
	case rSsSepSetDecrescendo      :
		SsSepSetDecrescendo ( *((int*)data + 1) , *((int*)data + 2) , 
			*((int*)data + 3) , *((int*)data + 4) ); 
		break;
	case rSsSepSetRitardando       :
		SsSepSetRitardando ( *((int*)data + 1) , *((int*)data + 2) , 
			*((int*)data + 3) , *((int*)data + 4) ); 
		break;
	case rSsSepSetVol :
		SsSepSetVol ( *((int*)data + 1) , *((int*)data + 2) , 
			*((int*)data + 3) , *((int*)data + 4) ); 
		break; 
	case rSsSepStop  :
		SsSepStop  ( *((int*)data + 1) , *((int*)data + 2) ); 
		break;
	case rSsSeqGetVol :
		SsSeqGetVol( *((int*)data + 1) , *((int*)data + 2) , 
			(short*)((int*)data + 3) , (short*)((int*)data + 4) ); 
		break;
	case rSsSeqPause  :
		SsSeqPause ( *((int*)data + 1)  ); 
		break;
	case rSsSeqPlayPtoP  :
		SsSeqPlayPtoP ( *((int*)data + 1) , *((int*)data + 2) , (u_char*)(*((int*)data + 3)) , (u_char*)(*((int*)data + 4)), *((int*)data + 5) , *((int*)data + 6) ); 
		break;
	case rSsSeqReplay:
		SsSeqReplay  ( *((int*)data + 1)  ); 
		break;
	case rSsSeqSetAccelerando:
		SsSeqSetAccelerando( *((int*)data + 1) , *((int*)data + 2) ,
			*((int*)data + 3) ); 
		break;
	case rSsSeqSetCrescendo:
		SsSeqSetCrescendo ( *((int*)data + 1) , *((int*)data + 2) , *((int*)data + 3)  ); 
		break;
	case rSsSeqSetDecrescendo:
		SsSeqSetDecrescendo      ( *((int*)data + 1) , *((int*)data + 2) , *((int*)data + 3)  ); 
		break;
	case rSsSeqSetRitardando:
		SsSeqSetRitardando       ( *((int*)data + 1) , *((int*)data + 2) , *((int*)data + 3)  ); 
		break;
	case rSsSeqSetNext:
		SsSeqSetNext  ( *((int*)data + 1) , *((int*)data + 2)  ); 
		break;
	case rSsSeqSkip :
		SsSeqSkip ( *((int*)data + 1) , *((int*)data + 2) , 
			*((int*)data + 3), *((int*)data + 4)  ); 
		break;
	case rSsSeqStop :
		SsSeqStop   ( *((int*)data + 1)  ); 
		break;
	case rSsSetAutoKeyOffMode  :
		SsSetAutoKeyOffMode    ( *((int*)data + 1)  ); 
		break;
	case rSsSetCurrentPoint:
		SsSetCurrentPoint( *((int*)data + 1) , *((int*)data + 2), (u_char*)(*((int*)data + 3))  ); 
		break;
	case rSsSetLoop :
		SsSetLoop( *((int*)data + 1) , *((int*)data + 2) , *((int*)data + 3)  ); 
		break;
	case rSsSetMono  :
		SsSetMono  (); 
		break;
	case rSsSetNext  :
		SsSetNext  ( *((int*)data + 1) , *((int*)data + 2) , *((int*)data + 3), *((int*)data + 4)  ); 
		break;
	case rSsSetReservedVoice   :
		ret = (char)SsSetReservedVoice     ( *((int*)data + 1)  ); 
		break;
	case rSsSetStereo   :
		SsSetStereo   (); 
		break;
	case rSsSetTempo   :
		SsSetTempo  ( *((int*)data + 1) , *((int*)data + 2) , 
			*((int*)data + 3)  ); 
		break;
	case rSsSetVoiceMask    :
		SsSetVoiceMask   ( *((int*)data + 1) ); 
		break;
	case rSsStart2 :
		SsStart2(); 
		break;
	case rSsUnBlockVoiceAllocation :
		ret = (char)SsUnBlockVoiceAllocation(); 
		break;
	case rSsUtFlush:
		SsUtFlush(); 
		break;
	case rSsUtGetVagAddr:
		ret = SsUtGetVagAddr ( *((int*)data + 1) , *((int*)data + 2)   ); 
		break;
	case rSsUtGetVagAddrFromTone     :
		ret = SsUtGetVagAddrFromTone( *((int*)data + 1) , *((int*)data + 2) , *((int*)data + 3)  ); 
		break;
	case rSsUtGetVBaddrInSB :
		ret = SsUtGetVBaddrInSB ( *((int*)data + 1) ); 
		break;
	case rSsVabTransBodyPartly :
		ret = SsVabTransBodyPartly ( (u_char*)(*((int*)data + 1)) , *((int*)data + 2) , *((int*)data + 3)  ); 
		break;
	case rSsVoiceCheck :
		ret = SsVoiceCheck ( *((int*)data + 1) , *((int*)data + 2) ,
		 *((int*)data + 3)  ); 
		break;
	case rSsVoKeyOff :
		ret = SsVoKeyOff  ( *((int*)data + 1) , *((int*)data + 2)  ); 
		break;
	case rSsVoKeyOn :
		ret = SsVoKeyOn  ( *((int*)data + 1) , *((int*)data + 2) ,
		 *((int*)data + 3), *((int*)data + 4) ); 
		break;
	case 0xe621:
	  	//--- sceSpu2CallbackInit()から呼ばれて、callback用のスレッド
		//--- （sce_spust_loop) をcreate/startする。
		//--- sce_spust_loopでは、EE側コールバック関数のbindを行なって
		//--- からコマンド待ちループに入る。
 		param.attr         = TH_C;
  		param.entry        = sce_spust_loop;
  		param.initPriority = BASE_priority - 8; // 24
		param.stackSize    = 0x800;
  		param.option       = 0;
  		gStThid = CreateThread(&param);
  		StartThread(gStThid,0);
  		printf("SPU2 callback thread created\n");
		break;
	default:
		printf("SPU driver error: unknown command %d \n", *((int*)data) );
		break;
	}
	PRINTF(( "return value = %x \n", ret )); 
 return (void*)(&ret);
}


/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */

