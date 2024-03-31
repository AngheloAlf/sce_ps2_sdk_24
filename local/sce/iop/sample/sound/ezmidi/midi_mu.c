/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                I/O Processor Library Sample Program
 *                          Version 1.30
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         ezmidi - midi_mu.c
 *                         MIDI Player Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.30      Dec.12.1999   morita   first checked in.
 */

#include <stdio.h>
#include <kernel.h>
#include <sif.h>
#include <csl.h>
#include <cslmidi.h>
#include <modmidi.h>
#include <modhsyn.h>
#include <libsd.h>
#include <timerman.h>
#include "ezmidi_i.h"
#include <modmsin.h>
#include <sys/file.h>

#define MIDI_IN_PORT    0
#define True 1

#define STREAMBUF_SIZE 1024

// 4167 micro sec. = 1000 * 1000 / 240 = 1/240 sec
#define ONE_240TH 4167

static sceCslCtx midiCtx;
static sceCslBuffGrp midiGrp [2];
static sceCslBuffCtx mInBfCtx [2], mOutBfCtx [1];

static char streamBf [STREAMBUF_SIZE + sizeof (sceCslMidiStream)];
static sceMidiEnv midiEnv;

static sceCslCtx synthCtx;
static sceCslBuffGrp synthGrp;
static sceCslBuffCtx sInBfCtx [4];
static sceHSynEnv synthEnv [2];

static char msinBf_T [MSIN_BUF_SIZE];
static char msinBf_R [MSIN_BUF_SIZE];

volatile int gWaitFlag;
volatile int gTimerID;
sceHSyn_VoiceStat gVstat;
int gThid;

typedef struct TimerCtx {
    int thread_id;
    int timer_id;
    int count;
} TimerCtx;

TimerCtx timer;

int gSpuTransPacketSize;
int gSpuTransPacketAddr;
int gIsInPlay;

/*--------------------------
  タイマー用割り込みハンドラ
  --------------------------*/
unsigned int
timer_handler (void *common)
{
    TimerCtx *tc = (TimerCtx *) common;

    iWakeupThread (tc->thread_id); // wakeup atick()

    return (tc->count); // 新たな比較値を設定しカウントを続行
}

/*-------------------
  1Tick毎の処理
  -------------------*/
static int
atick (void)
{
    while (1) {
	SleepThread ();

	//-- EEからmsinバッファが送られてきたならば、
	//   msinBf_Rにコピーする。hsynモジュールはmsinBf_Rを使う。
	if (((sceCslMidiStream*)msinBf_T)->validsize != 0 ){
	    memcpy (msinBf_R, msinBf_T, MSIN_BUF_SIZE);
	    ((sceCslMidiStream*)msinBf_T)->validsize = 0;
	}

	gIsInPlay = sceMidi_isInPlay (&midiCtx, MIDI_IN_PORT);

	sceMidi_ATick (&midiCtx);
	sceHSyn_ATick (&synthCtx);
    }

    return 0;
}

/* スレッドの作成 */
static int
make_thread (void)
{
    struct ThreadParam param;
    int	thid;

    param.attr         = TH_C;
    param.entry        = atick;
    param.initPriority = BASE_priority-3;
    param.stackSize    = 0x800;
    param.option = 0;

    /* スレッド作成 */
    thid = CreateThread (&param);	

    return thid;
}

/*-------------------
  タイマー設定
  -------------------*/
int
set_timer (TimerCtx *timer)
{
    struct SysClock clock;
    int timer_id;

    // 1/240 sec = ??? sysclock
    USec2SysClock (ONE_240TH, & clock);
    timer->count = clock.low;	/* within 32-bit */

    // Use sysclock timer
    if ((timer_id = AllocHardTimer (TC_SYSCLOCK, 32, 1)) <= 0) {
	printf ("Can NOT allocate hard timer ...\n");
	return (-1);
    }
    timer->timer_id = timer_id;

    if (SetTimerHandler (timer_id, timer->count,
			 timer_handler, (void *) timer) != KE_OK) {
	printf ("Can NOT set timeup timer handler ...\n");
	return (-1);
    }

    if (SetupHardTimer (timer_id, TC_SYSCLOCK, TM_NO_GATE, 1) != KE_OK) {
	printf ("Can NOT setup hard timer ...\n");
	return (-1);
    }

    return 0;
}

int
start_timer (TimerCtx* timer)
{
    if (StartHardTimer (timer->timer_id) != KE_OK) {
	printf ("Can NOT start hard timer ...\n");
	return (-1);
    }

    return 0;
}

/*-------------------
  タイマー削除
  -------------------*/
int
clear_timer (TimerCtx *timer)
{
    if (FreeHardTimer (timer->timer_id) != KE_OK) {
	printf ("Can NOT free hard timer ...\n");
	return (-1);
    }

    return 0;
}

int
stop_timer (TimerCtx* timer)
{
    int ret;

    ret = StopHardTimer (timer->timer_id);
    if (! (ret == KE_OK || ret == KE_TIMER_NOT_INUSE)) {
	printf ("Can NOT stop hard timer ...\n");
	return (-1);
    }

    return 0;
}

/*-------------------
  CallBackの例
  -------------------*/
int
metaMsgCB (unsigned char metaNo,unsigned char *bf, unsigned int len, unsigned int private)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream*)private;

#if 0
    printf ("META %02x\n", (int)metaNo);
#endif
    return True;
}

int
repeatCB (sceMidiLoopInfo *pInf, unsigned int private)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream*)private;

#if 0
    printf ("Repeat %s %d %d %x\n",
	    (pInf->type == sceMidiLoopInfoType_Midi) ? "Midi" : "Song",
	    (int)pInf->loopTimes, (int)pInf->loopCount, pInf->loopId);
#endif
    return True;
}


//--- トランスポーズの例
#define NOTESHIFT  4
int
channelMsgCB (unsigned int mes, unsigned int private_data)
{
    if (((mes & 0xf0) == 0x90) || ((mes & 0xf0) == 0x80)){
	mes += (NOTESHIFT) << 8;
    }

    return mes;
}

/*-------------------
  Module Context設定
  -------------------*/
void
set_module_context (void)
{

    ((sceCslMidiStream*)msinBf_R)->buffsize = MSIN_BUF_SIZE;
    ((sceCslMidiStream*)msinBf_R)->validsize = 0;
    ((sceCslMidiStream*)msinBf_T)->buffsize = MSIN_BUF_SIZE;
    ((sceCslMidiStream*)msinBf_T)->validsize = 0;


    midiCtx.extmod = NULL;
    midiCtx.callBack = NULL;
    midiCtx.buffGrpNum = 2;
    midiCtx.buffGrp = midiGrp;
      midiGrp [0].buffNum = 2;
      midiGrp [0].buffCtx = mInBfCtx;
      midiGrp [1].buffNum = 1;
      midiGrp [1].buffCtx = mOutBfCtx;
        mInBfCtx [0].sema = 0;
	mInBfCtx [0].buff = NULL;
	mInBfCtx [1].sema = 0;
	mInBfCtx [1].buff = &midiEnv;
	mOutBfCtx [0].sema = 0;
	mOutBfCtx [0].buff = streamBf; //-- HSynthと共有
	((sceCslMidiStream *)streamBf)->buffsize 
	    = STREAMBUF_SIZE + sizeof (sceCslMidiStream);
	((sceCslMidiStream *)streamBf)->validsize = 0;

    synthCtx.extmod = NULL;
    synthCtx.callBack = NULL;
    synthCtx.buffGrpNum = 1;
    synthCtx.buffGrp = &synthGrp;
      synthGrp.buffNum = 4;
      synthGrp.buffCtx = sInBfCtx;
        sInBfCtx [0].sema = 0;
	sInBfCtx [0].buff = streamBf; //-- midiと共有
	sInBfCtx [1].sema = 0;
	sInBfCtx [1].buff = &synthEnv [0];
	sInBfCtx [2].sema = 0;
	sInBfCtx [2].buff = msinBf_R;
	sInBfCtx [3].sema = 0;
	sInBfCtx [3].buff = &synthEnv [1];

    return;
}

int
MidiGetIopFileLength (char *filename)
{
    int flen, fd;

    if ((fd = open (filename, O_RDONLY)) < 0) {
	ERROR (("file open failed. %s \n", filename)); return -1;
    }
    if ((flen = lseek (fd, 0, SEEK_END)) < 0) {
	ERROR (("file open failed. %s \n", filename)); return -1;
    }
    if (lseek (fd, 0, SEEK_SET) < 0) {
	ERROR (("file open failed. %s \n", filename)); return -1;
    }
    close(fd);

    return flen;
}


/*--------------------------
  Midi Sequencerセットアップ
  --------------------------*/
int
MidiSetSq (int addr)
{
    int i;
    
    midiCtx.buffGrp [0].buffCtx [0].buff = (void *)addr;
    if (sceMidi_Load (&midiCtx, MIDI_IN_PORT) != sceHSynNoError) {
	printf ("sceMidi_Load(%d) error\n", i);
	return 1;
    }

//  midiEnv.chMsgCallBack = channelMsgCB;
    midiEnv.chMsgCallBack = NULL;
    midiEnv.chMsgCallBackPrivateData = (unsigned int)streamBf;
    midiEnv.metaMsgCallBack = metaMsgCB;
    midiEnv.metaMsgCallBackPrivateData = (unsigned int)streamBf;
    midiEnv.repeatCallBack = repeatCB;
    midiEnv.repeatCallBackPrivateData = (unsigned int)streamBf;
    midiEnv.excOutPort = 1;
    for (i = 0; i < sceMidiNumMidiCh; i++) {
	midiEnv.outPort [i] =  1 << 0;
    }

    if (sceMidi_SelectMidi (&midiCtx, MIDI_IN_PORT, 0) != sceMidiNoError) {
	printf("sceMidi_SelectMidi Error\n");
	return 1;
    }
    if (sceMidi_MidiSetLocation (&midiCtx, MIDI_IN_PORT, 0) != sceMidiNoError) {
	printf("sceMidi_MidiSetLocation Error\n");
	return 1;
    }

    return 0;
}


/*---------------------
  bdの分割ロード。
  ---------------------*/
int
MidiTransBdPacket (EZMIDI_BANK *bank)
{
    int mod, count, fd, size;
    int  total = 0;

    count = bank->bdSize / gSpuTransPacketSize;
    mod = bank->bdSize - count*gSpuTransPacketSize;

    if ((fd = open (bank->bdName, O_RDONLY)) < 0) {
	ERROR(("bd file open failed. %s \n", bank->bdName));
	return -1;
    }
    printf (" bd file open %s \n", bank->bdName);

    while (1) {
	size = read (fd, (unsigned char *)(gSpuTransPacketAddr), gSpuTransPacketSize);
	
	if (sceHSyn_VoiceTrans (1, (void *)gSpuTransPacketAddr,
				(void *)bank->spuAddr + total, size)
	    != sceHSynNoError) {
	    printf("sceHSyn_VoisTrans Error\n");
	    return -1;
	}

	total += size;
	if (size != gSpuTransPacketSize) break;
    }

    close (fd);

    return total;
}

/*----------------------------------------
  bdの転送
  IOPメモリへのロードはここでは行なわない。
  ----------------------------------------*/
int
MidiTransBd (EZMIDI_BANK *bank)
{
    if (sceHSyn_VoiceTrans (1, (void*)bank->bdAddr,
			    (void*)bank->spuAddr, bank->bdSize)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoisTrans Error\n");
	return -1;
    }

    return 0;
}

/*---------------------
  HardSynthセットアップ
  ---------------------*/
int
MidiSetHd (int port, EZMIDI_BANK *bank)
{
    int i;

    for (i = 0; i < 2; i++) {
	synthEnv [i].priority = 0;
	synthEnv [i].lfoWaveNum = 0;
	synthEnv [i].lfoWaveTbl = NULL;
	synthEnv [i].velocityMapNum = 0;
	synthEnv [i].velocityMapTbl = NULL;
    }

    if (sceHSyn_Load (&synthCtx, port, (void*)bank->spuAddr, (void*)bank->hdAddr, 0)
	!= sceHSynNoError) {
	printf("sceHSyn_Load(%d) error\n", MIDI_IN_PORT); return -1;
    }
    sceHSyn_SetVoiceStatBuffer(&gVstat);

    return 0;
}

/* CSL ハードウェアシンセサイザ: ポート属性の設定 */
void
MidiSetPortAttr (int port, int attr)
{
    synthEnv [port].priority     = (PORTATTR_MASK_PRIORITY & attr);
    synthEnv [port].maxPolyphony = (PORTATTR_MASK_MAXNOTE & attr) >> 8;
#if 0
    printf ("port %d pri %d, note %d \n",
	    port, synthEnv [port].priority, synthEnv [port].maxPolyphony);
#endif
    return;
}

/* CSL ハードウェアシンセサイザ: 演奏ボリュームの設定 */
void
MidiSetPortVolume (int port, int vol)
{
    sceHSyn_SetVolume (&synthCtx, port, vol);
    return;
}

/* CSL 環境の初期化と処理の開始 */
int
MidiInit (int allocsize)
{
    int oldstat;

    gSpuTransPacketSize = allocsize;
    CpuSuspendIntr (&oldstat);
    gSpuTransPacketAddr = (int)AllocSysMemory (0, allocsize, NULL);	
    CpuResumeIntr (oldstat);

    gThid = make_thread ();
    timer.thread_id = gThid;
    printf("EzMIDI: create thread ID= %d, ", gThid);

    /* スレッドを起動 */
    StartThread (gThid, (u_long)NULL);

    set_timer (&timer);

    //-- initialize modules
    set_module_context ();

    if (sceHSyn_Init (&synthCtx, ONE_240TH) != sceHSynNoError) { //-- 1/240sec
	printf("sceHSyn_Init Error\n");
	return 1;
    }
    if (sceMidi_Init (&midiCtx, ONE_240TH) != sceMidiNoError) { //-- 1/240sec
	printf("sceMidi_Init Error\n"); return 1;
    }

    start_timer(&timer);

    return (int)(&msinBf_T [0]);
}

/* 終了処理 */
int
MidiQuit (int foo)
{
    int oldstat;

    CpuSuspendIntr (&oldstat);
    FreeSysMemory ((void*)gSpuTransPacketAddr);
    CpuResumeIntr (oldstat);
    stop_timer (&timer);
    clear_timer (&timer);
    return 0;
}

/* 演奏停止 */
int
MidiStop (int addr)
{
    if (sceMidi_MidiPlaySwitch (&midiCtx, MIDI_IN_PORT, sceMidi_MidiPlayStop)
	!= sceMidiNoError) {
	printf("sceMidi_MidiPlaySwitch Error\n");
	return 1;
    }

    return 0;
}

/* 演奏中かの確認 */
int
MidiGetStatus (void)
{
    return gIsInPlay;
}

/* 演奏開始位置の変更 */
int
MidiSeek (int pos)
{
    if (sceMidi_MidiSetLocation(&midiCtx, MIDI_IN_PORT, pos)
	!= sceMidiNoError) {
	printf("sceMidi_MidiSetLocation Error\n");
	return 1;
    }

    return 0;
}

/* 演奏開始 */
int
MidiStart (int foo)
{
    //-- main loop
    if (sceMidi_MidiPlaySwitch(&midiCtx, MIDI_IN_PORT,sceMidi_MidiPlayStart)
	!= sceMidiNoError) {
	printf("sceMidi_MidiPlaySwitch Error\n");
	return 1;
    }

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
