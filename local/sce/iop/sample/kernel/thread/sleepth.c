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
 *                            sleepth.c
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

int firstthread(int arg);
int thread(int arg);

int start(int argc, char *argv[])
{
    struct ThreadParam param;
    int	thid;

    printf("\nSleep thread test ! \n");
    param.attr         = TH_C;
    param.entry        = firstthread;
    param.initPriority = BASE_priority;
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread(&param);
    if( thid > 0 ) {
	StartThread(thid,0);
	return RESIDENT_END;
    } else {
	return NO_RESIDENT_END;
    }
}

int firstthread(int arg)
{
    struct ThreadParam param;
    int	thid[6], i;
    char cmd[80];

    param.attr         = TH_C;
    param.entry        = thread;
    param.initPriority = BASE_priority+1;
    param.stackSize    = 0x800;
    param.option = 0;
    for( i = 0; i < 6 ; i ++ ) {
	thid[i] = CreateThread(&param);	
	StartThread(thid[i],i);
    }
    for(;;) {
	printf("0..5 or a..f or A..F > ");
	gets(cmd);	/* キー入力待ちでこのスレッドは WAIT状態になる */
	switch(cmd[0]) {
	case '0': 	case '1':
	case '2':	case '3':
	case '4':	case '5':
	    WakeupThread( thid[cmd[0]-'0'] );
	    break;
	case 'a': 	case 'b':
	case 'c':	case 'd':
	case 'e':	case 'f':
	    WakeupThread( thid[cmd[0]-'a'] );
	    WakeupThread( thid[cmd[0]-'a'] );
	    break;
	case 'A': 	case 'B':
	case 'C':	case 'D':
	case 'E':	case 'F':
	    WakeupThread( thid[cmd[0]-'A'] );
	    WakeupThread( thid[cmd[0]-'A'] );
	    WakeupThread( thid[cmd[0]-'A'] );
	    WakeupThread( thid[cmd[0]-'A'] );
	    break;
	}
    }
}

int thread(int arg)
{
    int i, tty;
    char ttyname[10];

    /* 各スレッド毎に tty を open する
     *  "tty0:" から "tty9:" まで使用可能。
     */
    strcpy(ttyname,"tty?:");
    ttyname[3] = arg+1+'0';
    tty = open(ttyname, O_RDWR);

    fdprintf(tty, "thread no. %d start\n",arg);
    for(i = 0;;i++) {
	fdprintf(tty, "\t thread no. %d   %d time SleepThread\n",arg, i);
	SleepThread();
	fdprintf(tty, "\t thread no. %d   wakeup count = %d\n",arg,
		 CancelWakeupThread( TH_SELF ));
    }
}
