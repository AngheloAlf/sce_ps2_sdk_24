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
 *                         timer5.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.0.0          2000/10/11      tei
 *       1.0.1          2001/01/17      tei	   add few comments
 */

#include <stdio.h>
#include <kernel.h>

ModuleInfo Module = {"hard_timer_sample_5", 0x0101 };

#define MAXCOUNT  0x10
struct com {
    int	  evfid;
    int	  ltimer;
    int	  stimer;
    int   ovfcount;
    int   timeupcount;
    int   endtime;
    int	  interval;
};

static int myCreateEventFlag(u_int attr, u_int initpattern, u_int option);

static u_int overflowhandler(void *arg)
{
    struct com *common = (struct com *)arg;
    common->ovfcount ++;
    if( common->ovfcount >= MAXCOUNT ) {
	common->endtime = GetTimerCounter(common->ltimer);
	iSetEventFlag( common->evfid, 1);
	return 0;	/* タイマーは停止する */
    } else {
	return 1;
    }
}

static u_int timeuphandler(void *arg)
{
    struct com *common = (struct com *)arg;
    common->timeupcount ++;
    if( common->timeupcount >= MAXCOUNT ) {
	common->endtime = GetTimerCounter(common->ltimer);
	iSetEventFlag( common->evfid, 1);
	return 0;	/* タイマーは停止する */
    } else {
	return common->interval;
    }
}

int start(int argc, char *argv[])
{
    u_long	starttime;
    struct com	common;

    printf("\nhard timer sample 5\n");
    common.evfid = myCreateEventFlag(EA_SINGLE,0,0);

    /* ハードウエアタイマを獲得する */
    common.stimer = AllocHardTimer(TC_SYSCLOCK, 16, 1);
    common.ltimer = AllocHardTimer(TC_SYSCLOCK, 32, 1);


    SetupHardTimer(common.ltimer, TC_SYSCLOCK, TM_NO_GATE, 1);
    StartHardTimer(common.ltimer);

    common.ovfcount = 0;
    SetOverflowHandler(common.stimer, overflowhandler, &common);
    SetupHardTimer(common.stimer, TC_SYSCLOCK, TM_NO_GATE, 1);

    StartHardTimer(common.stimer);
    starttime = GetTimerCounter(common.ltimer)-GetTimerCounter(common.stimer);
    /* 100 回オーバーフローが起こるまで待つ */
    WaitEventFlag(common.evfid, 1, EW_AND|EW_CLEAR, NULL );
    StopHardTimer(common.stimer);
    printf("  Set Overflow handler only\n");
    printf("   16 bit system clock counter overflow  0x%x times  --> 0x%06lx clocks\n",
	   MAXCOUNT, common.endtime-starttime);

    common.ovfcount = -MAXCOUNT;
    common.timeupcount = 0;
    common.interval = 0x1000;
    SetTimerHandler(common.stimer, common.interval, timeuphandler, &common);
    StartHardTimer(common.stimer);
    starttime = GetTimerCounter(common.ltimer)-GetTimerCounter(common.stimer);
    /* 100 回タイムアップするまでまで待つ */
    WaitEventFlag(common.evfid, 1, EW_AND|EW_CLEAR, NULL );
    StopHardTimer(common.stimer);
    printf("  Set Overflow handler and Timeup handler(timeup=0x1000)\n");
    printf("   16 bit system clock counter timeup    0x%x times  --> 0x%06lx clocks\n",
	   MAXCOUNT, common.endtime-starttime);
    if( common.ovfcount == -MAXCOUNT )
	printf("   no overflow\n");

    FreeHardTimer(common.stimer);
    StopHardTimer(common.ltimer);
    FreeHardTimer(common.ltimer);
    DeleteEventFlag(common.evfid);
    printf("hard timer sample 5 end\n");

    return NO_RESIDENT_END;
}

static int myCreateEventFlag(u_int attr, u_int initpattern, u_int option)
{
    struct EventFlagParam eparam;
    eparam.attr = attr;
    eparam.initPattern = initpattern;
    eparam.option = option;
    return CreateEventFlag(&eparam);
}
