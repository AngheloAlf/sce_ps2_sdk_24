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

/* IOP �̃X���b�h�x�[�X�̃v���O������ crt0 ��K�v�Ƃ��܂���B
 * main�֐��ł͂Ȃ� start�֐���p�ӂ��܂��B
 */

int start(int argc, char *argv[])
{
    struct ThreadParam param;
    int	thid;

    printf("\nCrate thread test ! \n\n");
    /* �X���b�h�Ƃ��ă������ɏ풓����v���O�����ł�
     * start �֐��͏o���邾�����₩�ɖ߂邱�Ƃ����҂���Ă���̂�
     * ���Ԃ̂�����Ȃ������������ς܂������Ƃ́A�����̂�����
     * �X���b�h�ɐ����n���ďI�����܂��B
     */
    param.attr         = TH_C;
    param.entry        = firstthread;
    param.initPriority = BASE_priority;
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread(&param);
    if( thid > 0 ) {
	StartThread(thid,0);
	return RESIDENT_END; /* 0: �v���O�������������ɏ풓�������܂܏I�� */
    } else {
	return NO_RESIDENT_END;	/* 1: �v���O����������������������ďI�� */
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
    /* �����֐������s����X���b�h�� 6�쐬 */
    for( i = 0; i < 6 ; i ++ ) {
	printf("CreateThread() #%d\n",i);
	thid[i] = CreateThread(&param);	
    }
    /* �쐬����̃X���b�h�Q�̏�Ԃ�\�� */
    printheader();
    for( i = 0; i < 6 ; i ++ ) {
	printthreadstatus(thid[i]);
    }
    /* �X���b�h���N�� */
    for( i = 0; i < 6 ; i ++ ) {
	printf("StartThread() #%d\n",i);
	StartThread(thid[i],i);
    }
    /* �N������̃X���b�h�Q�̏�Ԃ�\�� */
    printheader();
    for( i = 0; i < 6 ; i ++ ) {
	printthreadstatus(thid[i]);
    }
    /* �����̃v���C�I���e�B��������,���̃X���b�h�Ɏ��s����n���B*/
    ChangeThreadPriority( TH_SELF, BASE_priority+2 );
    printf(" all thread end #1\n");

    ChangeThreadPriority( TH_SELF, BASE_priority );
    /* �X���b�h���ēx�N�� */
    for( i = 0; i < 6 ; i += 2 ) {
	printf("StartThread() #%d\n",i);
	StartThread(thid[i],i);
    }
    for( i = 1; i < 6 ; i += 2 ) {
	printf("StartThread() #%d\n",i);
	StartThread(thid[i],i);
    }

    /* �N������̃X���b�h�Q�̏�Ԃ�\�� */
    printheader();
    for( i = 0; i < 6 ; i ++ ) {
	printthreadstatus(thid[i]);
    }
    ChangeThreadPriority( TH_SELF, BASE_priority+2 );
    printf(" all thread end #2\n");

    /* �I����̃X���b�h�Q�̏�Ԃ�\�� */
    printheader();
    for( i = 0; i < 6 ; i ++ ) {
	printthreadstatus(thid[i]);
    }
    /* �X���b�h���폜 */
    for( i = 0; i < 6 ; i ++ ) {
	DeleteThread(thid[i]);
    }
    printf("fin\n");
    /* ���܂́A�܂��I�������X���b�h�v���O���������������珜������@�\��
     *  ������
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
    /* �X���b�h�̃g�b�v�̊֐����甲���邱�Ƃ� ExitThread()���ĂԂ��Ƃ͓��� */
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
