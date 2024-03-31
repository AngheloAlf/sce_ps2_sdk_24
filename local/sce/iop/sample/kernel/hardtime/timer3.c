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
 *                         timer3.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.0.0          2000/10/11      tei
 */

#include <stdio.h>
#include <kernel.h>

ModuleInfo Module = {"hard_timer_sample_3", 0x0101 };

int start(int argc, char *argv[])
{
    struct SysClock	clock;
    int		sysctimer1, sysctimer2;
    u_long	onesecclock, c1, c2;

    printf("\nhard timer sample 3\n");

    /* システムクロックの周波数を求める */
    USec2SysClock(1000000, &clock);
    onesecclock = clock.low;

    sysctimer1 = AllocHardTimer(TC_HLINE, 32, 1);
    sysctimer2 = AllocHardTimer(TC_SYSCLOCK, 32, 256);

    /* 基準のタイマーカウンタを設定する */    
    SetupHardTimer(sysctimer1, TC_SYSCLOCK, TM_NO_GATE, 1);

    
    /* タイマーカウンタを prescale = 8 に設定する */    
    SetupHardTimer(sysctimer2, TC_SYSCLOCK, TM_NO_GATE, 8);

    StartHardTimer(sysctimer1);
    StartHardTimer(sysctimer2);
    /* 一秒待つ  */
    while( GetTimerCounter(sysctimer1) < onesecclock )
	{}
    c1 = GetTimerCounter(sysctimer1);
    c2 = GetTimerCounter(sysctimer2);

    StopHardTimer(sysctimer1);
    StopHardTimer(sysctimer2);

    printf("  System clock prescale=1   --> %9ld counts\n", c1);
    printf("  System clock prescale=8   --> %9ld counts *   8 => %9ld\n",
	   c2, c2*8);

    
    /* タイマーカウンタを prescale = 16 に設定する */    
    SetupHardTimer(sysctimer2, TC_SYSCLOCK, TM_NO_GATE, 16);
    StartHardTimer(sysctimer1);
    StartHardTimer(sysctimer2);
    /* 一秒待つ  */
    while( GetTimerCounter(sysctimer1) < onesecclock )
	{}
    c1 = GetTimerCounter(sysctimer1);
    c2 = GetTimerCounter(sysctimer2);

    StopHardTimer(sysctimer1);
    StopHardTimer(sysctimer2);

    printf("  System clock prescale=1   --> %9ld counts\n", c1);
    printf("  System clock prescale=16  --> %9ld counts *  16 => %9ld\n",
	   c2, c2*16);

    
    /* タイマーカウンタを prescale = 256 に設定する */    
    SetupHardTimer(sysctimer2, TC_SYSCLOCK, TM_NO_GATE, 256);
    StartHardTimer(sysctimer1);
    StartHardTimer(sysctimer2);
    /* 一秒待つ  */
    while( GetTimerCounter(sysctimer1) < onesecclock )
	{}
    c1 = GetTimerCounter(sysctimer1);
    c2 = GetTimerCounter(sysctimer2);

    StopHardTimer(sysctimer1);
    StopHardTimer(sysctimer2);

    printf("  System clock prescale=1   --> %9ld counts\n", c1);
    printf("  System clock prescale=256 --> %9ld counts * 256 => %9ld\n",
	   c2, c2*256);

    
    /* タイマーカウンタを TM_GATE_ON_Count に設定する */    
    SetupHardTimer(sysctimer1, TC_SYSCLOCK, TM_GATE_ON_Count, 1);
    /* 基準のタイマーカウンタを設定する */    
    SetupHardTimer(sysctimer2, TC_SYSCLOCK, TM_NO_GATE , 1);
    StartHardTimer(sysctimer1);
    StartHardTimer(sysctimer2);
    /* 一秒待つ  */
    while( GetTimerCounter(sysctimer2) < onesecclock )
	{}
    c1 = GetTimerCounter(sysctimer1);
    c2 = GetTimerCounter(sysctimer2);

    StopHardTimer(sysctimer1);
    StopHardTimer(sysctimer2);

    printf("  System clock with TM_NO_GATE                --> %9ld counts\n", c2);
    printf("  System clock with TM_GAGE_ON_Count(v-blank) --> %9ld counts\n", c1);


    FreeHardTimer(sysctimer1);
    FreeHardTimer(sysctimer2);
    printf("hard timer sample 3 end\n");

    return NO_RESIDENT_END;
}
