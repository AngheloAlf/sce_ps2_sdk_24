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
 *                         timer4.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.0.0          2000/10/11      tei
 *       1.0.1          2001/01/17      tei	   add few comments
 */

#include <stdio.h>
#include <kernel.h>

ModuleInfo Module = {"hard_timer_sample_4", 0x0101 };

#define MAXCOUNT  10
struct com {
    int	  evfid;
    int   count;
    int	  interval[MAXCOUNT];
};

static int myCreateEventFlag(u_int attr, u_int initpattern, u_int option);

static u_int timeuphandler1(void *arg)
{
    struct com *common = (struct com *)arg;

    iSetEventFlag( common->evfid, 1);
    common->count ++;
    if( common->count >= MAXCOUNT ) {
	return 0;	/* �^�C�}�[�͒�~���� */
    } else {
	return common->interval[0];
    }
}

static u_int timeuphandler2(void *arg)
{
    struct com *common = (struct com *)arg;

    iSetEventFlag( common->evfid, 1);
    common->count ++;
    if( common->count >= MAXCOUNT ) {
	return 0;	/* �^�C�}�[�͒�~���� */
    } else {
	return common->interval[common->count];
    }
}

int start(int argc, char *argv[])
{
    struct SysClock	clock;
    u_long	onesecclock, hlines, starttime, nowtime;
    int		hltimer, systimer, sec, usec;
    struct com	common;
    int   i;

    printf("\nhard timer sample 4\n");
    common.evfid = myCreateEventFlag(EA_SINGLE,0,0);

    /* �V�X�e���N���b�N�̎��g�������߂� */
    USec2SysClock(1000000, &clock);
    onesecclock = clock.low;

    /* �n�[�h�E�G�A�^�C�}���l������ */
    hltimer = AllocHardTimer(TC_HLINE, 32, 1);
    systimer = AllocHardTimer(TC_SYSCLOCK, 32, 1);
    SetupHardTimer(systimer, TC_SYSCLOCK, TM_NO_GATE, 1);
    StartHardTimer(systimer);

    /* 0.5 �b�Ԋu�Ńn���h�����Ă΂��T���v�� */
    common.interval[0] = onesecclock/2;
    common.count = 0;
    SetTimerHandler(hltimer, common.interval[0], timeuphandler1, &common);
    SetupHardTimer(hltimer, TC_SYSCLOCK, TM_NO_GATE, 1);
    StartHardTimer(hltimer);
    starttime = GetTimerCounter(systimer);
    printf(" half second interval timer start\n");
    for( i = 0; i < MAXCOUNT; i++ ) {
	WaitEventFlag(common.evfid, 1, EW_AND|EW_CLEAR, NULL );
	nowtime = GetTimerCounter(systimer);
	clock.hi = 0;
	clock.low = nowtime - starttime;
	SysClock2USec(&clock, &sec, &usec);
	printf("  %d : %d.%06d sec\n", i, sec, usec);
	starttime = nowtime;
    }
    printf(" end\n");


    /* �Ԋu�� 0.�P�b���� 1.1 �b�܂ő����Ă����悤�ɂŃn���h�����Ă΂��T���v�� */
    for( i = 0; i < MAXCOUNT; i ++ )
	common.interval[i] = (onesecclock*(i+1))/10;
    common.count = 0;
    SetTimerHandler(hltimer, common.interval[0], timeuphandler2, &common);
    SetupHardTimer(hltimer, TC_SYSCLOCK, TM_NO_GATE, 1);
    StartHardTimer(hltimer);
    starttime = GetTimerCounter(systimer);
    printf(" variable interval timer start\n");
    for( i = 0; i < MAXCOUNT; i++ ) {
	WaitEventFlag(common.evfid, 1, EW_AND|EW_CLEAR, NULL );
	nowtime = GetTimerCounter(systimer);
	clock.hi = 0;
	clock.low = nowtime - starttime;
	SysClock2USec(&clock, &sec, &usec);
	printf("  %d : %d.%06d sec\n", i, sec, usec);
	starttime = nowtime;
    }
    printf(" end\n");

    /* 60 V �Ԋu�Ńn���h�����Ă΂��T���v�� */
    SetupHardTimer(hltimer, TC_HLINE, TM_NO_GATE, 1);
    WaitVblankStart();
    StartHardTimer(hltimer);
    WaitVblankStart();    
    hlines = GetTimerCounter(hltimer);     /* 1 V �� h-line �������߂� */
    StopHardTimer(hltimer);

    common.interval[0] = hlines*60;
    common.count = 0;
    SetTimerHandler(hltimer, common.interval[0], timeuphandler1, &common);
    SetupHardTimer(hltimer, TC_HLINE, TM_NO_GATE, 1);
    StartHardTimer(hltimer);
    starttime = GetTimerCounter(systimer);
    printf(" %d H-line interval timer start\n", common.interval[0]);
    for( i = 0; i < MAXCOUNT; i++ ) {
	WaitEventFlag(common.evfid, 1, EW_AND|EW_CLEAR, NULL );
	nowtime = GetTimerCounter(systimer);
	clock.hi = 0;
	clock.low = nowtime - starttime;
	SysClock2USec(&clock, &sec, &usec);
	printf("  %d : %d.%06d sec\n", i, sec, usec);
	starttime = nowtime;
    }
    printf(" end\n");

    StopHardTimer(hltimer);    /* hltimer �͊��Ɏ~�܂��Ă��邪�A�O�̂��ߎ~�߂� */
    FreeHardTimer(hltimer);
    StopHardTimer(systimer);
    FreeHardTimer(systimer);
    DeleteEventFlag(common.evfid);
    printf("hard timer sample 4 end\n");
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
