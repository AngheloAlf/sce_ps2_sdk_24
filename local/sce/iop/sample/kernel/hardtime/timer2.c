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
 *                         timer2.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.0.0          2000/10/11      tei
 */

#include <stdio.h>
#include <kernel.h>

ModuleInfo Module = {"hard_timer_sample_2", 0x0101 };

int start(int argc, char *argv[])
{
    int		hltimer1, hltimer2;
    u_long      c0, c1, c2, c3, vlines;

    printf("\nhard timer sample 2\n");

    hltimer1 = AllocHardTimer(TC_HLINE, 16, 1);
    hltimer2 = AllocHardTimer(TC_HLINE, 32, 1);
    SetupHardTimer(hltimer2, TC_HLINE, TM_NO_GATE, 0);

    printf(" Count H-line use gate signal(V-blank)\n"
	   "                  +- StartHardTimer()\n"
	   "                  V\n"
	   "\n"
	   "     V-blank  ----+    +----------------------------+    +------\n"
	   "                  |    |                            |    |\n"
	   "                  |    |                            |    |\n"
	   "                  +----+                            +----+\n");

    /* ================================================================
     *J  1�t���[�����Ԃ� H-line �� TM_NO_GATE �ŃJ�E���g
     */
    SetupHardTimer(hltimer1, TC_HLINE, TM_NO_GATE, 0);
    WaitVblankStart();    /* Vblank �ɂȂ�܂ő҂� */
    StartHardTimer(hltimer1);
    WaitVblankStart();    /* ���� Vblank �ɂȂ�܂ő҂� */
    vlines = GetTimerCounter(hltimer1);
    StopHardTimer(hltimer1);
    printf("    TM_NO_GATE:\n"
	   "                  0================================>============\n"
	   "                                    (%3ld)\n", vlines);


    /* ================================================================
     *J  1�t���[�����Ԃ� H-line �� TM_GATE_ON_Count �ŃJ�E���g
     */
    SetupHardTimer(hltimer1, TC_HLINE, TM_GATE_ON_Count, 0);
    WaitVblankStart();
    StartHardTimer(hltimer1);
    StartHardTimer(hltimer2);
    /* �J�E���g���J�n����܂ł̎��Ԃ��v�� */
    while( GetTimerCounter(hltimer1)==0 )
	c1 = GetTimerCounter(hltimer2);
    /* StartHardTimer()���s���� 1 �t���[���o�߂���܂ł܂� */
    while( vlines > GetTimerCounter(hltimer2) )
	{}
    c2 = GetTimerCounter(hltimer1);
    StopHardTimer(hltimer1);
    StopHardTimer(hltimer2);
    printf("    TM_GATE_ON_Count:\n"
	   "                  <---->0==========================><---->0=====\n"
	   "                   (%2ld)             (%3ld)\n", c1, c2);

    /* ================================================================
     *J  H-line �� TM_GATE_ON_ClearStart �ŃJ�E���g
     */
    SetupHardTimer(hltimer1, TC_HLINE, TM_GATE_ON_ClearStart, 0);
    WaitVblankStart();
    StartHardTimer(hltimer1);
    /*  �J�E���^���N���A�����܂ł̎��Ԃ��v�� */
    c0 = c1 = GetTimerCounter(hltimer1);
    while( c0 <= c1 ) {
	c0 = c1;
	c1 = GetTimerCounter(hltimer1);
    }
    /*  ������x�J�E���^���N���A�����܂ł̎��Ԃ��v�� */
    c2 = c3 = GetTimerCounter(hltimer1);
    while( c2 <= c3 ) {
	c2 = c3;
	c3 = GetTimerCounter(hltimer1);
    }
    StopHardTimer(hltimer1);
    printf("    TM_GATE_ON_ClearStart:\n"
	   "                  0====>0================================>0=====\n"
	   "                   (%2ld)             (%3ld)\n", c0, c2 );

    /* ================================================================
     *J  1�t���[�����Ԃ� H-line �� TM_GATE_ON_Clear_OFF_Start �ŃJ�E���g
     */
    SetupHardTimer(hltimer1, TC_HLINE, TM_GATE_ON_Clear_OFF_Start, 0);
    WaitVblankStart();
    StartHardTimer(hltimer1);
    /*  �J�E���^���N���A�����܂ł̎��Ԃ��v�� */
    c0 = c1 = GetTimerCounter(hltimer1);
    while( c0 <= c1 ) {
	c0 = c1;
	c1 = GetTimerCounter(hltimer1);
    }
    /*  �J�E���^���J�E���g���J�n����܂ł̎��Ԃ��v�� */
    StartHardTimer(hltimer2);
    while( GetTimerCounter(hltimer1) == 0 ) {
	c2 = GetTimerCounter(hltimer2);
    }
    StopHardTimer(hltimer1);
    StopHardTimer(hltimer2);
    printf("    TM_GATE_ON_Clear_OFF_Start:\n"
	   "                  0====><-------------------------->0====><-----\n"
	   "                   (%2ld)             (%3ld)\n", c0, c2);

    /* ================================================================
     *J  1�t���[�����Ԃ� H-line �� TM_GATE_ON_Start �ŃJ�E���g
     */
    SetupHardTimer(hltimer1, TC_HLINE, TM_GATE_ON_Start, 0);
    WaitVblankStart();
    StartHardTimer(hltimer1);
    StartHardTimer(hltimer2);
    /* �J�E���g���J�n�����܂ł̎��Ԃ��v�� */
    while( GetTimerCounter(hltimer1)==0 )
	c1 = GetTimerCounter(hltimer2);
    /* StartHardTimer()���s���� 1 �t���[���o�߂���܂ł܂� */
    while( vlines > GetTimerCounter(hltimer2) )
	{}
    c2 = GetTimerCounter(hltimer1);
    StopHardTimer(hltimer1);
    StopHardTimer(hltimer2);
    printf("    TM_GATE_ON_Start:\n"
	   "                  <---->0==========================>============\n"
	   "                   (%2ld)             (%3ld)\n", c1, c2);

    FreeHardTimer(hltimer1);
    FreeHardTimer(hltimer2);

    printf("hard timer sample 2 end\n");

    return NO_RESIDENT_END;
}
