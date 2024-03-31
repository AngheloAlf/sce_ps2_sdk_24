/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.3
 */
/*
 *	    iLINK socket Library Sample
 *
 *	                    Shift-JIS
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *	                 All Rights Reserved.
 *
 *                      ilsample.c
 *
 *	Version       Date            Design     Log
 *  --------------------------------------------------------------------
 *	0.91          02/16/2000      sim        Initial
 *	1.00          04/03/2000      sim        Test of kit0331
 *	1.01          04/04/2000      sim        add maxsize to sceILsockInit()
 *	1.02          04/11/2000      sim        Test of kit0407, cmc=1
 *	1.03          04/18/2000      sim        Delete QueryBootParam
 *	1.04          04/22/2000      sim        492 byte Recv bug fixed
 *	1.05          04/24/2000      sim        RPC DMA size optimized
 *	1.06          05/10/2000      sim        Rename sceILsock...
 *	1.07          05/11/2000      sim        Selectable USE_CYCLETIMEV
 *	1.08          05/11/2000      sim        Optimize stack size
 *	2.00          2001.11.19      oonuki     unload module
 *  --------------------------------------------------------------------
 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sifrpc.h>
#include <ilink.h>
#include <ilsock.h>
#include <ilsocksf.h>

//#define DEBUG_PRINT
#define DBGPRINTF   printf
//#define USE_CYCLETIMEV

ModuleInfo Module = { "iLINK_Server", 0x0202 };

#define BASE_priority       100
#define CYCLETIME_priority   32

/*******************************************************************************************/
// thread and service functions
/*******************************************************************************************/
static unsigned int buf_Init[ 4 ];

void* svr_sceILsockInit(unsigned int fno, void *data, int size)
{
  int maxsock = ((int*)data)[0];
  int maxsize = ((int*)data)[1];
  
  ((int*)data)[0] = sceILsockInit( maxsock, maxsize ); 

#ifdef DEBUG_PRINT
  DBGPRINTF( "sceILsockInit( maxsock=%d, maxsize=%d ) = %d\n", maxsock, maxsize, ((int*)data)[0] );
#endif

  return data;
}

void th_sceILsockInit()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockInit, svr_sceILsockInit, (void*)buf_Init, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_Reset[ 4 ];

void* svr_sceILsockReset(unsigned int fno, void *data, int size)
{
  sceILsockReset(); 

#ifdef DEBUG_PRINT
  DBGPRINTF( "sceILsockReset()\n");
#endif

  return data;
}

void th_sceILsockReset()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockReset, svr_sceILsockReset, (void*)buf_Reset, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_Open[ 4 ];

void* svr_sceILsockOpen(unsigned int fno, void *data, int size)
{
  int domain = ((int*)data)[0];
  int type = ((int*)data)[1];
  int protocol = ((int*)data)[2];

  ((int*)data)[0] = sceILsockOpen( domain, type, protocol ); 

#ifdef DEBUG_PRINT
  DBGPRINTF( "sceILsockOpen( %d, %d, %d ) = %d\n", domain, type, protocol, ((int*)data)[0] );
#endif

  return data;
}

void th_sceILsockOpen()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockOpen, svr_sceILsockOpen, (void*)buf_Open, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_Close[ 4 ];

void* svr_sceILsockClose(unsigned int fno, void *data, int size)
{
  int sock = ((int*)data)[0];

  ((int*)data)[0] = sceILsockClose( sock ); 

#ifdef DEBUG_PRINT
  DBGPRINTF( "sceILsockClose( %d ) = %d\n", sock, ((int*)data)[0] );
#endif

  return data;
}

void th_sceILsockClose()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockClose, svr_sceILsockClose, (void*)buf_Close, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_Bind[ 8 ];

void* svr_sceILsockBind(unsigned int fno, void *data, int size)
{
  int sock = ((int*)data)[0];
  int namelen = ((int*)data)[1];

  ((int*)data)[0] = sceILsockBind( sock, (struct sceILsock_addr*)&(((int*)data)[2]), namelen); 

#ifdef DEBUG_PRINT
  if ( ((int*)data)[0] < 0 )
  {
    DBGPRINTF( "sceILsockBind( sock=%d, namelen=%d, data=%08x ) = %d\n", 
      sock, namelen, (unsigned int)((int*)data), ((int*)data)[0] );
  }
#endif

  return data;
}

void th_sceILsockBind()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockBind, svr_sceILsockBind, (void*)buf_Bind, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_Connect[ 8 ];

void* svr_sceILsockConnect(unsigned int fno, void *data, int size)
{
  int sock = ((int*)data)[0];
  int namelen = ((int*)data)[1];

  ((int*)data)[0] = sceILsockConnect( sock, (struct sceILsock_addr*)&(((int*)data)[2]), namelen); 

#ifdef DEBUG_PRINT
  DBGPRINTF( "sceILsockConnect( sock=%d, namelen=%d ) = %d\n", sock, namelen, ((int*)data)[0] );
#endif

  return data;
}

void th_sceILsockConnect()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockConnect, svr_sceILsockConnect, (void*)buf_Connect, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_Send[ 128 ];

void* svr_sceILsockSend(unsigned int fno, void *data, int size)
{
  int sock  = ((int*)data)[0];
  int len   = ((int*)data)[2];

  ((int*)data)[0] =sceILsockSend( sock, (char*)&((int*)data)[3], len, 0 ); 

#ifdef DEBUG_PRINT
  if(((int*)data)[0] < 0)
    DBGPRINTF( "ilsample line: %d, sceILsockSend( sock=%d, len=%d ) = %d\n",
      __LINE__, sock, len, ((int*)data)[0] );
#endif

  return data;
}

void th_sceILsockSend()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockSend, svr_sceILsockSend, (void*)buf_Send, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_SendTo[ 128 ];

void* svr_sceILsockSendTo(unsigned int fno, void *data, int size)
{
  int sock  = ((int*)data)[0];
  int len   = ((int*)data)[4];

#ifdef DEBUG_PRINT
  int i;
  DBGPRINTF( "sceILsockSendTo:\n" );
  for( i = 0; i < tolen / 4; i++ )
    DBGPRINTF( "to [%02x]= 0x%08x\n", i, ((int*)data)[1+i] );
  for( i = 0; i < len / 4; i++ )
    DBGPRINTF( "buf[%02x]= 0x%08x\n", i, ((int*)data)[5+i] );
#endif

  ((int*)data)[0] = sceILsockSendTo( sock, (char*)&(((int*)data)[5]), len, 0,
    (struct sceILsock_addr*)&(((int*)data)[1]), 16 ); 

#ifdef DEBUG_PRINT
  DBGPRINTF( "sceILsockSendTo( sock=%d, len=%d ) = %d\n",
    sock, len, ((int*)data)[0] );
#endif

  return data;
}

void th_sceILsockSendTo()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockSendTo, svr_sceILsockSendTo, (void*)buf_SendTo, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_Recv[ 128 ];

void* svr_sceILsockRecv(unsigned int fno, void *data, int size)
{
  int sock  = ((int*)data)[0];
  int len   = ((int*)data)[1];

  ((int*)data)[0] = sceILsockRecv( sock, (char*)&(((int*)data)[1]), len, 0 );

#ifdef DEBUG_PRINT
  DBGPRINTF( "sceILsockRecv( sock=%d, len=%d ) = %d\n", sock, len, ((int*)data)[0] );
#endif

  return data;
}

void th_sceILsockRecv()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockRecv, svr_sceILsockRecv, (void*)buf_Recv, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_RecvFrom[ 128 ];

void* svr_sceILsockRecvFrom(unsigned int fno, void *data, int size)
{
  int sock    = ((int*)data)[0];
  int len     = ((int*)data)[1];
  int fromlen = 16;

  ((int*)data)[4] = sceILsockRecvFrom( sock, (char*)&(((int*)data)[5]), len, 0,
    (struct sceILsock_addr*)&(((int*)data)[1]), &fromlen);

  ((int*)data)[0] = fromlen; 

#ifdef DEBUG_PRINT
  DBGPRINTF( "sceILsockRecvFrom( sock=%d, len=%d, fromlen=%d ) = %d\n",
    sock, len, fromlen, ((int*)data)[4] );
#endif

  return data;
}

void th_sceILsockRecvFrom()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sceILsockRecvFrom, svr_sceILsockRecvFrom, (void*)buf_RecvFrom, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
static unsigned int buf_Eui64[ 4 ];

void* svr_sce1394SbEui64(unsigned int fno, void *data, int size)
{
  unsigned int *buf = (unsigned int*)data;

  buf[ 0 ] = sce1394SbEui64( &buf[ 1 ] ); 

#ifdef DEBUG_PRINT
  DBGPRINTF( "sce1394SbEui64() = %d (0x%08x 0x%08x)\n", buf[ 0 ], buf[ 1 ], buf[ 2 ] );
#endif

  return data;
}

void th_sce1394SbEui64()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sce1394SbEui64, svr_sce1394SbEui64, (void*)buf_Eui64, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
#ifdef USE_CYCLETIMEV
static unsigned int CycleTimeV;
static unsigned int buf_CycleTimeV[4];

void* svr_sce1394CycleTimeV(unsigned int fno, void *data, int size)
{
  *(unsigned int*)data = CycleTimeV; 

#ifdef DEBUG_PRINT
  {
    unsigned int timer = *(unsigned int*)data;
    int CycleTimerSecond, CycleTimerCycle, CycleTimerOffset;

    CycleTimerSecond = (int)(timer >> 25);
    CycleTimerCycle = (int)((timer >> 12) & 0x1fff);
    CycleTimerOffset = (int)(timer & 0xfff);
    DBGPRINTF( "sce1394CycleTimeV() = %03d:%04d:%04d\n", CycleTimerSecond, CycleTimerCycle, CycleTimerOffset );
  }
#endif

  return data;
}

void th_sce1394CycleTimeV()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sce1394CycleTimeV, svr_sce1394CycleTimeV, (void*)buf_CycleTimeV, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}
#endif
/*******************************************************************************************/
static unsigned int buf_NodeId[4];

void* svr_sce1394SbNodeId(unsigned int fno, void *data, int size)
{
  *(unsigned int*)data = sce1394SbNodeId(); 

#ifdef DEBUG_PRINT
	DBGPRINTF( "sce1394SbNodeID() = 0x%04x\n", *(unsigned int*)data );
#endif

  return data;
}

void th_sce1394SbNodeId()
{
  sceSifQueueData qd;
  sceSifServeData sd;

  sceSifSetRpcQueue( &qd, GetThreadId() );
  sceSifRegisterRpc( &sd, SIFNUM_sce1394SbNodeId, svr_sce1394SbNodeId, (void*)buf_NodeId, 0, 0, &qd );
  sceSifRpcLoop( &qd );
}

/*******************************************************************************************/
// 
/*******************************************************************************************/
/* アンロードモジュール対応 oonuki.2001.11.19 */
int create_start_thread( struct ThreadParam *p, const char *name, void* f, int size, int pri )
{
  int th;

  p->initPriority = pri;
  p->stackSize    = size;
  p->entry        = f;
  th = CreateThread( p );
  if( th > 0 ) 
  {
    StartThread( th, 0 );
//    #ifdef DEBUG_PRINT
    DBGPRINTF( "0x%08x : %s\n", th , name);
//    #endif
    return(th); // Normal end oonuki.2001.11.19
  } else {
    DBGPRINTF( "Error at Create thread: %s\n" , name );
    return(th); // Abnormal end oonuki.2001.11.19
  }
}

/* アンロードモジュール対応 oonuki.2001.11.19 */
int terminate_delete_thread( int id )
{
  int ret;

  ret = TerminateThread( id );
  if( ret != KE_OK ){
//    #ifdef DEBUG_PRINT
    DBGPRINTF( "0x%08x : terminate error.\n", id );
//    #endif
    return(-1);
  }

  ret = DeleteThread( id );
  if( ret != KE_OK ){
//    #ifdef DEBUG_PRINT
    DBGPRINTF( "0x%08x : delete error.\n", id );
//    #endif
    return(-1);
  }

  return(0);
}


/*******************************************************************************************/
//
/*******************************************************************************************/
typedef struct{
  int id;
  const char *name;
  void ( *func )( void );
  int size;
  int pri;
} THREAD_ID;

#ifndef USE_CYCLETIMEV
  #define ID_NUM 12
#else
  #define ID_NUM 13
#endif
THREAD_ID th_id[ ID_NUM ] = {
	{ 0, "sceILsockInit"    , th_sceILsockInit    , 0x400, BASE_priority }, 
	{ 0, "sceILsockReset"   , th_sceILsockReset   , 0x300, BASE_priority },
	{ 0, "sceILsockOpen"    , th_sceILsockOpen    , 0x300, BASE_priority },
	{ 0, "sceILsockClose"   , th_sceILsockClose   , 0x300, BASE_priority },
	{ 0, "sceILsockBind"    , th_sceILsockBind    , 0x300, BASE_priority },
	{ 0, "sceILsockConnect" , th_sceILsockConnect , 0x600, BASE_priority },
	{ 0, "sceILsockSend"    , th_sceILsockSend    , 0x700, BASE_priority },
	{ 0, "sceILsockSendTo"  , th_sceILsockSendTo  , 0x700, BASE_priority },
	{ 0, "sceILsockRecv"    , th_sceILsockRecv    , 0x400, BASE_priority - 1 },
	{ 0, "sceILsockRecvFrom", th_sceILsockRecvFrom, 0x400, BASE_priority - 1 },
	{ 0, "sce1394SbEui64"   , th_sce1394SbEui64   , 0x300, BASE_priority },
	{ 0, "sce1394SbNodeId"  , th_sce1394SbNodeId  , 0x300, BASE_priority },
#ifdef USE_CYCLETIMEV
	{ 0, "sce1394CycleTimeV", th_sce1394CycleTimeV, 0x300, BASE_priority - 1 },
#endif
};

start ( int argc, char *argv[] )
{
  struct ThreadParam p;
  int cnt1, cnt2;
  int ret = 0;
  int EUI64[ 2 ] = { 0, 0 };

  if( argc >= 0 ){

    DBGPRINTF( "ilsample.c %s %s\n", __DATE__, __TIME__ );

    FlushDcache();
  
    CpuEnableIntr();

    sce1394Initialize( NULL );

    ret = sce1394SbEui64( EUI64 );
    if( ret == SCE1394ERR_OK ) DBGPRINTF( "My GUID = 0x%08x 0x%08x\n", EUI64[ 0 ], EUI64[ 1 ] );
    else DBGPRINTF( "Error at sce1394SbEui64: %d\n", ret );

    ret = sce1394SbNodeId();
    if( ret != SCE1394ERR_NOT_INITIALIZED )
      DBGPRINTF( "My NodeId = 0x%04x (Bus=0x%04x, Offset=0x%03x)\n", ret, ret >> 6, ret & 0x3f );
    else DBGPRINTF( "Error at sce1394SbNodeId: %d\n", ret );

    p.attr = TH_C;
    p.option = 0;

    /* スレッドの生成とスタート */
    for( cnt1 = 0; cnt1 < ID_NUM; cnt1++ ){
      if(( th_id[cnt1].id = create_start_thread( &p, th_id[cnt1].name, th_id[cnt1].func,
							th_id[cnt1].size, th_id[cnt1].pri )) <= 0){
        /* スレッドが立ち上がらなかったら、全て削除 */
        for( cnt2 = 0; cnt2 < cnt1+1; cnt2++ ){
          terminate_delete_thread( th_id[cnt2].id );
        }
        return NO_RESIDENT_END;
      }
    }

    #ifdef USE_CYCLETIMEV
    if( RegisterVblankHandler( VB_START, CYCLETIME_priority, (void*)sceGetCycleTimeV, (void*)&CycleTimeV ) != 0 )
      return NO_RESIDENT_END;
    #endif

    return REMOVABLE_RESIDENT_END;
  }
  else{

    argc = -argc;

    #ifdef USE_CYCLETIMEV
    if( ReleaseVblankHandler( VB_START, (void*)sceGetCycleTimeV ) != 0 ) return REMOVABLE_RESIDENT_END;
    #endif

    /* スレッドの停止と削除 */
    for( cnt1 = 0; cnt1 < ID_NUM; cnt1++ ){
      if( terminate_delete_thread( th_id[cnt1].id ) < 0 ) return REMOVABLE_RESIDENT_END;
    }

    return NO_RESIDENT_END;
  }
}
