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

    /* �V�X�e���N���b�N�̎��g�������߂� */
    USec2SysClock(1000000, &clock);
    onesecclock = clock.low;

    hltimer   = AllocHardTimer(TC_HLINE, 32, 1);
    sysctimer = AllocHardTimer(TC_SYSCLOCK, 32, 256);

    SetupHardTimer(sysctimer, TC_SYSCLOCK, TM_NO_GATE, 1);
    SetupHardTimer(hltimer, TC_HLINE, TM_NO_GATE, 0);
    StartHardTimer(sysctimer);

    /* ================================================================
     *J  ��b�Ԃ� H-line �����񂠂邩�̌v�������Ă݂�
     */
    /* NTSC�C���^�[���X �̏ꍇ 1�b�Ԃ� 15734.25(=59.94*262.5) �񂠂�͂��B*/
    /* PAL�C���^�[���X �̏ꍇ 1�b�Ԃ� 15625(=50*312.5) �񂠂�͂��B       */
    StartHardTimer(hltimer);
    /* ��b�҂B�i����:���̑҂����� IOP �� cpu �^�C���𖳑ʂɏ����̂ŁA
     *                   �ʏ�͂��̂悤�Ƀv���O��������͍̂D�܂����Ȃ��B�j
     */
    starttime = GetTimerCounter(sysctimer);
    while( GetTimerCounter(sysctimer)-starttime < onesecclock )
	{}
    hlcount = GetTimerCounter(hltimer);
    StopHardTimer(hltimer);
    printf("  One second H-lines is %ld\n", hlcount);

    /* ================================================================
     *J  ������x�A��b�Ԃ� H-line �����񂠂邩�̌v�������Ă݂�
     */
    StartHardTimer(hltimer);
    /* ��b�҂B�i����: DelayThread() �� IOP �� cpu �^�C���͖��ʂɂ��Ȃ���
     *                    ���m�����ۏ؂��ꂸ�A�w�肵�����Ԃ����኱�x�ꂪ
     *                    ������B�j
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
