/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *              I/O Processor Library Sample Program
 * 
 *                         - thread -
 * 
 * 
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                            createth.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.4.0   
 */

#include <stdio.h>
#include <kernel.h>

#define BASE_priority  32

void firstthread(int arg);
void thread(int arg);
void printheader();
void printthreadstatus(int thid);

/* IOP のスレッドベースのプログラムは crt0 を必要としません。
 * main関数ではなく start関数を用意します。
 */

int start(int argc, char *argv[])
{
    struct ThreadParam param;
    int	thid;

    printf("\nCrate thread test ! \n\n");
    /* スレッドとしてメモリに常駐するプログラムでは
     * start 関数は出来るだけ速やかに戻ることが期待されているので
     * 時間のかからない初期化等を済ませたあとは、自分のつくった
     * スレッドに制御を渡して終了します。
     */
    param.attr         = TH_C;
    param.entry        = firstthread;
    param.initPriority = BASE_priority;
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread(&param);
    if( thid > 0 ) {
	StartThread(thid,0);
	return RESIDENT_END; /* 0: プログラムをメモリに常駐させたまま終了 */
    } else {
	return NO_RESIDENT_END;	/* 1: プログラムをメモリから消去して終了 */
    }
}

void firstthread(int arg)
{
    struct ThreadParam param;
    int	thid[6], i;

    param.attr         = TH_C;
    param.entry        = thread;
    param.initPriority = BASE_priority+1;
    param.stackSize    = 0x800;
    param.option = 0;
    /* 同じ関数を実行するスレッドを 6個作成 */
    for( i = 0; i < 6 ; i ++ ) {
	printf("CreateThread() #%d\n",i);
	thid[i] = CreateThread(&param);	
    }
    /* 作成直後のスレッド群の状態を表示 */
    printheader();
    for( i = 0; i < 6 ; i ++ ) {
	printthreadstatus(thid[i]);
    }
    /* スレッドを起動 */
    for( i = 0; i < 6 ; i ++ ) {
	printf("StartThread() #%d\n",i);
	StartThread(thid[i],i);
    }
    /* 起動直後のスレッド群の状態を表示 */
    printheader();
    for( i = 0; i < 6 ; i ++ ) {
	printthreadstatus(thid[i]);
    }
    /* 自分のプライオリティを下げて,他のスレッドに実行権を渡す。*/
    ChangeThreadPriority( TH_SELF, BASE_priority+2 );
    printf(" all thread end #1\n");

    ChangeThreadPriority( TH_SELF, BASE_priority );
    /* スレッドを再度起動 */
    for( i = 0; i < 6 ; i += 2 ) {
	printf("StartThread() #%d\n",i);
	StartThread(thid[i],i);
    }
    for( i = 1; i < 6 ; i += 2 ) {
	printf("StartThread() #%d\n",i);
	StartThread(thid[i],i);
    }

    /* 起動直後のスレッド群の状態を表示 */
    printheader();
    for( i = 0; i < 6 ; i ++ ) {
	printthreadstatus(thid[i]);
    }
    ChangeThreadPriority( TH_SELF, BASE_priority+2 );
    printf(" all thread end #2\n");

    /* 終了後のスレッド群の状態を表示 */
    printheader();
    for( i = 0; i < 6 ; i ++ ) {
	printthreadstatus(thid[i]);
    }
    /* スレッドを削除 */
    for( i = 0; i < 6 ; i ++ ) {
	DeleteThread(thid[i]);
    }
    printf("fin\n");
    /* いまは、まだ終了したスレッドプログラムをメモリから除去する機能は
     *  未実装
     */
}

void thread(int arg)
{
    int i;
    printf("thread no. %d start\n",arg);
    for( i = 0; i < 3 ; i ++ ) {
	printf("\t thread no. %d   %d time Rotate Ready Queue\n",arg, i);
	RotateThreadReadyQueue(TPRI_RUN);
    }
    printf("thread no. %d end\n",arg);
    /* スレッドのトップの関数から抜けることと ExitThread()を呼ぶことは同等 */
}

char statuschar[] = "000RunRdy333Wat555666777Sus999aaabbbWSudddeeefffDom";
char waitchar[]   = "  SlDlSeEvMbVpFp  ";


void printheader()
{
    printf( "THID   ATTR     OPTION   STS ENTRY  STACK  SSIZE GP     CP  IP WT WID    WUC\n");
}

void printthreadstatus(int thid)
{
    struct ThreadInfo 	info;
    if( ReferThreadStatus(thid, &info) >= KE_OK ) {
	printf( "%06x %08x %08x %.3s ",
		thid, info.attr, info.option,
		  statuschar+info.status*3);
	printf( "%06x %06x %04x %06x %3d %3d %c%c %06x %3x\n",
		(int)info.entry, (int)info.stack, info.stackSize,
		(int)info.gpReg, info.currentPriority, info.initPriority,
		waitchar[info.waitType<<1], waitchar[1+(info.waitType<<1)],
		info.waitId, info.wakeupCount);
    } else {
	printf(" thid=%x not found\n", thid);
    }
}
