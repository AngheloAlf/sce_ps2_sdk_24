/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *		I/O Processor Library Sample Program
 *
 *			-- Hardware timer --
 * 
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                         timer1.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.0.0          2000/10/11      tei
 */

#include <stdio.h>
#include <kernel.h>

ModuleInfo Module = {"hard_timer_sample_1", 0x0101 };

int start(int argc, char *argv[])
{
    struct SysClock	clock;
    int		hltimer, sysctimer;
    u_long	onesecclock, hlcount, starttime;

    printf("\nhard timer sample 1\n");

    /* システムクロックの周波数を求める */
    USec2SysClock(1000000, &clock);
    onesecclock = clock.low;

    hltimer   = AllocHardTimer(TC_HLINE, 32, 1);
    sysctimer = AllocHardTimer(TC_SYSCLOCK, 32, 256);

    SetupHardTimer(sysctimer, TC_SYSCLOCK, TM_NO_GATE, 1);
    SetupHardTimer(hltimer, TC_HLINE, TM_NO_GATE, 0);
    StartHardTimer(sysctimer);

    /* ================================================================
     *J  一秒間に H-line が何回あるかの計測をしてみる
     */
    /* NTSCインターレス の場合 1秒間に 15734.25(=59.94*262.5) 回あるはず。*/
    /* PALインターレス の場合 1秒間に 15625(=50*312.5) 回あるはず。       */
    StartHardTimer(hltimer);
    /* 一秒待つ。（注意:この待ち方は IOP の cpu タイムを無駄に消費するので、
     *                   通常はこのようにプログラムするのは好ましくない。）
     */
    starttime = GetTimerCounter(sysctimer);
    while( GetTimerCounter(sysctimer)-starttime < onesecclock )
	{}
    hlcount = GetTimerCounter(hltimer);
    StopHardTimer(hltimer);
    printf("  One second H-lines is %ld\n", hlcount);

    /* ================================================================
     *J  もう一度、一秒間に H-line が何回あるかの計測をしてみる
     */
    StartHardTimer(hltimer);
    /* 一秒待つ。（注意: DelayThread() は IOP の cpu タイムは無駄にしないが
     *                    正確さが保証されず、指定した時間よりも若干遅れが
     *                    生じる。）
     */
    DelayThread(1000000);
    hlcount = GetTimerCounter(hltimer);
    StopHardTimer(hltimer);
    StopHardTimer(sysctimer);
    printf("  One second H-lines is %ld\n", hlcount);

    FreeHardTimer(hltimer);
    FreeHardTimer(sysctimer);

    printf("hard timer sample 1 end\n");

    return NO_RESIDENT_END;
}
