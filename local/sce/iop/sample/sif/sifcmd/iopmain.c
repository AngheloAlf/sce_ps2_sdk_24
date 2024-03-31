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

/* SIF CMDŠÖ”‚¨‚æ‚Ñ•Ï”‚ÌéŒ¾ */
static void  test_f0(void *,void *);
static volatile int   test0;
static void  test_f1(void *,void *);
static volatile int   test1;

/* SIFCMDŠÖ”“o˜^ƒoƒbƒtƒ@ */
#define BSIZE 0x10
static sceSifCmdData cmdbuffer[BSIZE];

int start()
{
  unsigned int id;
  _SIF_I_DEF;

  /* init */
  sceSifInitCmd();

  /* set local buffer & functions */
  /* “o˜^‚ÌŠÔŠ„‚èž‚Ý‹ÖŽ~*/
  _SIF_DI;
  /* SIFCMD —p‚ÌŠÖ”ƒoƒbƒtƒ@‚ÌÝ’è*/
  sceSifSetCmdBuffer( &cmdbuffer[0], BSIZE);

  /* ŠÖ”ƒoƒbƒtƒ@‚Ì0/1”Ô‚Ö,ŠÖ”‚Ì“o˜^ */
  sceSifAddCmdHandler( 0, (void *) test_f0, (void *) &test0);
  sceSifAddCmdHandler( 1, (void *) test_f1, (void *) &test1);
  _SIF_EI;

  /* •Ï”‚Ì“à—e‚ð‚O‚Ö */
  test0 = 0;
  test1 = 0;

  /* sync other cpu */
  /* •W€“o˜^‚ÌSIFCMDŠÖ”‚Å,‘ŠŽè‘¤‚ÌCPU‚Æ‘Ò‚¿‡‚í‚¹ */
  srd.rno = 31;
  srd.value = 1;
  sceSifSendCmd(SIF_CMDC_SET_SREG, &srd, sizeof(srd), 0 , 0, 0);
  while(sceSifGetSreg(31) == 0);

  /* call test_f0 */
  /* ‘ŠŽè‘¤‚Ì‚O”Ô–Ú‚Ì“o˜^ŠÖ”‚ðŒÄ‚ÔB*/
  sch.opt = (unsigned int) 10;
  id = sceSifSendCmd(0, &sch, sizeof(sch), 0 , 0, 0);
  while(sceSifDmaStat(id) >= 0);
  
  /* call test_f1 */
  /* ‘ŠŽè‘¤‚Ì‚P”Ô–Ú‚Ì“o˜^ŠÖ”‚ðŒÄ‚Ô */
  sch.opt = (unsigned int) 20;
  id = sceSifSendCmd(1, &sch, sizeof(sch), 0 , 0, 0);
  while(sceSifDmaStat(id) >= 0);

  /* loop */
  /* ‘ŠŽè‘¤‚©‚ç“o˜^ŠÖ”‚ª‚æ‚Î‚ê‚ÄA•Ï”‚Ì“à—e‚ª•Ï‚í‚é‚Ü‚Å‘Ò‚Â */
  while((test0 == 0) || (test1 == 0));

  /* •ÏX‚³‚ê‚½•Ï”‚Ì“à—e‚ð•\Ž¦ */  
  printf("test0 = %d test1 = %d\n", test0, test1);

  while(1);

}

/* ‚O”Ô–Ú‚Ì“o˜^ŠÖ” */
/* ŠÖ”“o˜^Žž‚Ìƒf[ƒ^ƒAƒhƒŒ‚ð,int ‚ÌƒAƒhƒŒƒX‚Æ‚µ‚Ä,‚»‚Ì’l‚ð */
/* ƒRƒ}ƒ“ƒhƒpƒPƒbƒg‚Ìopt ƒtƒB[ƒ‹ƒh‚ÉÝ’è*/
void test_f0(void *p,void *q)
{
  sceSifCmdHdr *h = (sceSifCmdHdr *) p;

  *(int *)q = (int)h->opt;
  
  return;
}


/* ‚P”Ô–Ú‚Ì“o˜^ŠÖ” */
/* ŠÖ”“o˜^Žž‚Ìƒf[ƒ^ƒAƒhƒŒ‚ð,int ‚ÌƒAƒhƒŒƒX‚Æ‚µ‚Ä,‚»‚Ì’l‚ð */
/* ƒRƒ}ƒ“ƒhƒpƒPƒbƒg‚Ìopt ƒtƒB[ƒ‹ƒh‚ÉÝ’è*/
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
