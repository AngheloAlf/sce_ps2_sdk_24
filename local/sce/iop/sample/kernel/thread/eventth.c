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
 *                            eventth.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.4.0           
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <kernel.h>

#define BASE_priority  32

struct tharg {
	int   thid;
	int   thnum;
	int   evfid;
};

int firstthread(int arg);
int thread(struct tharg *arg);
int MyCreateThread(int attr, void *entry, int iprio, int stack, void *opt);

int start(int argc, char *argv[])
{
    int	thid;

    printf("\nevent thread test ! \n");
    thid = MyCreateThread(TH_C, firstthread, BASE_priority, 0x800, 0);
    if( thid > 0 ) {
	StartThread(thid,0);
	return RESIDENT_END;
    } else {
	return NO_RESIDENT_END;
    }
}

int firstthread(int arg)
{
    struct EventFlagParam eparam;
    int	 i, evfid;
    struct tharg thinf[6];
    char cmd[80];

    eparam.attr = EA_MULTI;
    eparam.initPattern = 0;
    eparam.option = 0;
    evfid = CreateEventFlag(&eparam);

    for( i = 0; i < 6 ; i ++ ) {
	thinf[i].thnum = i;
	thinf[i].thid =MyCreateThread(TH_C,thread,BASE_priority+1,0x800,0);
	thinf[i].evfid = evfid;
	StartThread(thinf[i].thid, (u_int)&thinf[i]);
    }
    for(;;) {
	printf("0..5,a > ");
	gets(cmd);	/* キー入力待ちでこのスレッドは WAIT状態になる */
	switch(cmd[0]) {
	case '0': 	case '1':
	case '2':	case '3':
	case '4':	case '5':
	    SetEventFlag(evfid, 1<<(cmd[0]-'0')); break;
	case 'a':
	    /* 複数ビットを立てることで一気に複数のスレッドを起こせる。
	     * (このプログラムではやっていないが 単一ビットに対して
	     *  複数のスレッドが待ちにはいることも可能)
	     */
	    SetEventFlag(evfid, 0xffffffff); break;
	}
    }
}

int thread(struct tharg *arg)
{
    int i, tty;
    long result;
    char ttyname[10];

    /* 各スレッド毎に tty を open する
     *  "tty0:" から "tty9:" まで使用可能。
     */
    strcpy(ttyname,"tty?:");
    ttyname[3] = arg->thnum+1+'0';
    tty = open(ttyname, O_RDWR);
    fdprintf(tty, "thread no. %d start\n", arg->thnum);
    for(i = 0;;i++) {
	fdprintf(tty, "\t thread no. %d   %d time WaitEventFlag\n",arg->thnum, i);
	if( arg->thnum < 3 ) {
	    /* 0,1,2 のスレッドは 戻ってきてから 自分で担当の bit だけ
	       クリアする。*/
	    WaitEventFlag(arg->evfid, 1<<arg->thnum,
			  EW_OR, (void *)&result);
	    ClearEventFlag( arg->evfid, ~(1<<arg->thnum) );
	} else {
	    /* 3,4,5 のスレッドは EW_CLEAR で条件成立時に
	       イベントフラグの自動クリアを指定する。*/
	    WaitEventFlag(arg->evfid, 1<<arg->thnum,
			  EW_OR|EW_CLEAR, (void *)&result);
	}
	fdprintf(tty, "\t thread no. %d     result pattern = 0x%lx\n",
	       arg->thnum, result);
    }
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
