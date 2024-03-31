/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *              Emotion Engine Library Sample Program
 *
 *                         - <file> -
 *
 *                         Version <0.10>
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                            <main.c>
 *                     <sif cmd test program>
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.10            Mar,26,1999     hakamatani  first version
 *      0.90            Aug,23,1999     hakamatani  first version
 */

/* ================================================================
 *
 *      Program
 *
 * ================================================================ */

#include <stdio.h>
#include <loadcore.h>
#include <intrman.h>
#include <sif.h>
#include <sifcmd.h>

#define _SIF_I_DEF int oldisEI
#define _SIF_DI  CpuSuspendIntr(&oldisEI)
#define _SIF_EI  CpuResumeIntr(oldisEI) 

sceSifCmdHdr	sch;
sceSifCmdSRData srd;

ModuleInfo Module = {"SIFCMD_TEST", 0x0101 };

/* SIF CMD�֐�����ѕϐ��̐錾 */
static void  test_f0(void *,void *);
static volatile int   test0;
static void  test_f1(void *,void *);
static volatile int   test1;

/* SIFCMD�֐��o�^�o�b�t�@ */
#define BSIZE 0x10
static sceSifCmdData cmdbuffer[BSIZE];

int start()
{
  unsigned int id;
  _SIF_I_DEF;

  /* init */
  sceSifInitCmd();

  /* set local buffer & functions */
  /* �o�^�̊Ԋ��荞�݋֎~*/
  _SIF_DI;
  /* SIFCMD �p�̊֐��o�b�t�@�̐ݒ�*/
  sceSifSetCmdBuffer( &cmdbuffer[0], BSIZE);

  /* �֐��o�b�t�@��0/1�Ԃ�,�֐��̓o�^ */
  sceSifAddCmdHandler( 0, (void *) test_f0, (void *) &test0);
  sceSifAddCmdHandler( 1, (void *) test_f1, (void *) &test1);
  _SIF_EI;

  /* �ϐ��̓��e���O�� */
  test0 = 0;
  test1 = 0;

  /* sync other cpu */
  /* �W���o�^��SIFCMD�֐���,���葤��CPU�Ƒ҂����킹 */
  srd.rno = 31;
  srd.value = 1;
  sceSifSendCmd(SIF_CMDC_SET_SREG, &srd, sizeof(srd), 0 , 0, 0);
  while(sceSifGetSreg(31) == 0);

  /* call test_f0 */
  /* ���葤�̂O�Ԗڂ̓o�^�֐����ĂԁB*/
  sch.opt = (unsigned int) 10;
  id = sceSifSendCmd(0, &sch, sizeof(sch), 0 , 0, 0);
  while(sceSifDmaStat(id) >= 0);
  
  /* call test_f1 */
  /* ���葤�̂P�Ԗڂ̓o�^�֐����Ă� */
  sch.opt = (unsigned int) 20;
  id = sceSifSendCmd(1, &sch, sizeof(sch), 0 , 0, 0);
  while(sceSifDmaStat(id) >= 0);

  /* loop */
  /* ���葤����o�^�֐�����΂�āA�ϐ��̓��e���ς��܂ő҂� */
  while((test0 == 0) || (test1 == 0));

  /* �ύX���ꂽ�ϐ��̓��e��\�� */  
  printf("test0 = %d test1 = %d\n", test0, test1);

  while(1);

}

/* �O�Ԗڂ̓o�^�֐� */
/* �֐��o�^���̃f�[�^�A�h����,int �̃A�h���X�Ƃ���,���̒l�� */
/* �R�}���h�p�P�b�g��opt �t�B�[���h�ɐݒ�*/
void test_f0(void *p,void *q)
{
  sceSifCmdHdr *h = (sceSifCmdHdr *) p;

  *(int *)q = (int)h->opt;
  
  return;
}


/* �P�Ԗڂ̓o�^�֐� */
/* �֐��o�^���̃f�[�^�A�h����,int �̃A�h���X�Ƃ���,���̒l�� */
/* �R�}���h�p�P�b�g��opt �t�B�[���h�ɐݒ�*/
void test_f1(void *p,void *q)
{
  sceSifCmdHdr *h = (sceSifCmdHdr *) p;

  *(int *)q = (int)h->opt;
  
  return;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
