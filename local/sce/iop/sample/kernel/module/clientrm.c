/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *		I/O Processor Library Sample Program
 *
 *			-- Module --
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                         clientrm.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       2.3.0          May,19,2001     isii
 */

#include <stdio.h>
#include <string.h>
#include <kernel.h>

#define MYNAME "clientrm"
ModuleInfo Module = { MYNAME, 0x0101 };

#define BASE_priority  (USER_LOWEST_PRIORITY+USER_HIGHEST_PRIORITY)/2

int module_start(int argc, char *argv[]);
int module_stop(int argc, char *argv[]);

int MyCreateThread(int attr, void *entry, int iprio, int stack, void *opt);
int thread(void *arg);

extern void libentry1(char *name);
extern void libentry2(char *name);

int thid;

int start(int argc, char *argv[])
{
    if( argc >= 0 )	return module_start(argc,argv);
    else	 	return module_stop(-argc,argv);
}

int module_start(int argc, char *argv[])
{
    printf("'%s' Start\n", MYNAME);
    thid = MyCreateThread(TH_C, thread, BASE_priority, 0x800, 0);
    if( thid > 0 ) {
	StartThread(thid,0);
	printf("'%s' removable resident\n", MYNAME);
	return REMOVABLE_RESIDENT_END; /* 削除可能の常駐を通知する */
    } else {
	printf("'%s' no resident\n", MYNAME);
	return NO_RESIDENT_END;
    }
}

int module_stop(int argc, char *argv[])
{
    if( strcmp(argv[0],"other") == 0 ) {
	/* 自モジュールの生成したスレッドが呼び出した他のモジュールの
	 *  関数内から間接的に StopModule()を呼び出された場合
	 *  ここで、自モジュールの生成したスレッドを停止しようとすると
	 *  TerminateThread()に失敗します。このときのエラーリカバリは
	 *  煩雑になるので、あらかじめチェックをするほうが良いでしょう。
	 */
	if( thid == GetThreadId() ) {
	    printf("'%s' What Happen ?, Indirect StopModule() call\n", MYNAME);
	    return REMOVABLE_RESIDENT_END; /* 停止処理に失敗 */
	}
	printf("'%s' stop by other\n", MYNAME);
	if( TerminateThread(thid) == KE_OK
	    && DeleteThread(thid) == KE_OK ) {
	    return NO_RESIDENT_END; /* 停止処理に成功 */
	} else {
	    /* 停止処理に失敗した場合、可能ならば常駐続行のための
	     *  停止処理のキャンセル（必要なら再初期化）が行われるのが
	     *  無難だと思われます。
	     *  しかしこれはケースバイケースなので、充分検討の上
	     *  注意深くプログラミングしてください。
	     */
	    printf("'%s' stop fail\n", MYNAME);
	    return REMOVABLE_RESIDENT_END; /* 停止処理に失敗 */
	}
    } else {
	printf("'%s' self stop\n", MYNAME);
	return NO_RESIDENT_END; /* 停止処理に成功 */
    }
}

int thread(void *arg)
{
    int i, modid;
    for( i = 0; i < 10 ; i ++ ) {
	DelayThread(2000000); /* wait 2 sec */
	libentry1(MYNAME);
	libentry2(MYNAME);
    }
    modid = SelfStopModule(0,NULL,&i);
    printf("'%s' My module id was %d. End !\n", MYNAME, modid);
    SelfUnloadModule();
    return 0; /* no use */
}

int MyCreateThread(int attr, void *entry, int iprio, int stack, void *opt)
{
    struct ThreadParam param;
    param.attr = attr;
    param.entry = entry;
    param.initPriority = iprio;
    param.stackSize = stack;
    param.option = (u_int)opt;
    return CreateThread(&param);
}
