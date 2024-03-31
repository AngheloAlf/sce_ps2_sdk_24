/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *                          Version 1.20
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         sqhard - main.c
 *                      MIDI Sequencer Sample
 *
 *     Version   Date          Design     Log
 *  --------------------------------------------------------------------
 *     1.20      Nov.28.1999   morita     changed for LoadModule from EE
 *     0.60      Oct.14.1999   morita     first checked in.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <kernel.h>
#include <stdio.h>
#include <sif.h>
#include <csl.h>
#include <cslmidi.h>
#include <modmidi.h>
#include <modssyn.h>
#include <libsd.h>
#include <timerman.h>
#include <sys/file.h>

/* ----------------------------------------------------------------
 * 定数マクロ/変数
 * ---------------------------------------------------------------- */
#define True 1

/* ----------------------------------------------------------------
 * modmidi 関連
 * ---------------------------------------------------------------- */
// 入力ポート
#define MIDI_IN_PORT 0

/* CSL context */
static sceCslCtx midiCtx;

/* CSL buffer group */
static sceCslBuffGrp midiGrp [2];   // 常に要素数は 2 ... 0/入力, 1/出力
static sceCslBuffCtx mInBfCtx  [2]; // 入力ポートの総数 x 2
static sceCslBuffCtx mOutBfCtx [1]; // 出力ポートの総数

/*
 * midi-stream buffer
 */
#define STREAMBUF_SIZE 1024
#define STREAMBUF_SIZE_ALL (sizeof (sceCslMidiStream) + STREAMBUF_SIZE) // バッファ + 属性

// 各入出力ポートが使うバッファ
static char streamBf [STREAMBUF_SIZE+sizeof(sceCslMidiStream)];
// 環境構造体
static sceMidiEnv midiEnv;

/* ----------------------------------------------------------------
 * modhsyn 関連
 * ---------------------------------------------------------------- */

/* CSL context */
static sceCslCtx synthCtx;

/* CSL buffer group */
static sceCslBuffGrp synthGrp;

/* CSL buffer context */
static sceCslBuffCtx sInBfCtx [2]; // 入力ポートの総数 x 2
static sceSSynEnv    synthEnv;     // 入力ポートの総数

char gFilename [64]; // SQ ファイル名

/* ----------------------------------------------------------------
 * タイマー
 * ---------------------------------------------------------------- */

// 4167 micro sec. = 1000 * 1000 / 240 = 1/240 sec
#define ONE_240TH 4167

typedef struct TimerCtx {
    int thread_id;
    int timer_id;
    int count;
} TimerCtx;

/* ----------------
 * 割り込みハンドラ
 * ---------------- */
unsigned int
timer_handler (void *common)
{
    TimerCtx *tc = (TimerCtx *) common;

    iWakeupThread (tc->thread_id); // wakeup atick()

    return (tc->count); // 新たな比較値を設定しカウントを続行
}

/* ----------------
 * タイマー設定
 * ---------------- */
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

/* ----------------
 * タイマー削除
 * ---------------- */
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

/* ----------------------------------------------------------------
 * 1 tick 毎の処理
 * ---------------------------------------------------------------- */
static void
atick (void)
{
    sceMidi_ATick (&midiCtx);
    sceSSyn_ATick (&synthCtx);
}


/* ----------------------------------------------------------------
 * CallBack の例
 * ---------------------------------------------------------------- */
/* ----------------
 * メタイベント受信
 * ---------------- */
int
callback_meta_msg (unsigned char metaNo, unsigned char *bf, unsigned int len, unsigned int private)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *)private;

#if 0
    printf ("META %02x\n", (int)metaNo);
#endif
    return True;
}

/* ----------------
 * 繰り返し制御
 * ---------------- */
int
callback_repeat (sceMidiLoopInfo *pInf, unsigned int private)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream*)private;

#if 0
    printf ("Repeat %s %d %d %x\n",
	    (pInf->type == sceMidiLoopInfoType_Midi) ? "Midi" : "Song",
	    (int)pInf->loopTimes, (int)pInf->loopCount, pInf->loopId);
#endif
    return True;
}

/* ----------------------------------------------------------------
 * 各 Mudule Context の設定
 * ---------------------------------------------------------------- */
void
set_module_context (void)
{
    /*
     * modmidi
     */
    // CSL context
    midiCtx.extmod     = NULL;
    midiCtx.callBack   = NULL;
    midiCtx.buffGrpNum = 2;	// modmidi は常に 2
    midiCtx.buffGrp    = midiGrp;
      // CSL buffer group: 入力
      midiGrp [0].buffNum = 2;
      midiGrp [0].buffCtx = mInBfCtx;
	mInBfCtx [0].sema = 0;
	mInBfCtx [0].buff = NULL;
	mInBfCtx [1].sema = 0;
	mInBfCtx [1].buff = &midiEnv;
      // CSL buffer group: 出力
      midiGrp [1].buffNum = 1;
      midiGrp [1].buffCtx = mOutBfCtx;
	mOutBfCtx [0].sema = 0;
	mOutBfCtx [0].buff = streamBf;

    /*
     * modhsyn
     */
    // CSL context
    synthCtx.extmod     = NULL;
    synthCtx.callBack   = NULL;
    synthCtx.buffGrpNum = 1;	// modhsyn は常に 1
    synthCtx.buffGrp    = &synthGrp;
      // CSL buffer group: 入力
      synthGrp.buffNum = 2;
      synthGrp.buffCtx = sInBfCtx;
        sInBfCtx [0].sema = 0;
	sInBfCtx [0].buff = streamBf; // modmidi と共有
	sInBfCtx [1].sema = 0;
	sInBfCtx [1].buff = &synthEnv;

    /*
     * midi-stream buffer
     */
    ((sceCslMidiStream *)streamBf)->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslMidiStream *)streamBf)->validsize = 0;

    return;
}

/* ----------------------------------------------------------------
 * Software Synthesizer セットアップ
 * ---------------------------------------------------------------- */
int
set_ssyn (void)
{
    // modssyn 全体の初期化
    if (sceSSyn_Init (&synthCtx, ONE_240TH) != sceSSynNoError) { // 1/240sec
	printf("sceSSyn_Init Error\n");
	return 1;
    }

    return 0;
}


/* ----------------------------------------------------------------
 * Midi Sequencer セットアップ
 * ---------------------------------------------------------------- */
#define ALLOC_SIZE (128 * 1024)

int
set_midi (void)
{
    void* sq;
    int fd, i;
    int oldstat;

    /* SQ ファイルの読み込み
     * ---------------------------------------------------------------- */
    CpuSuspendIntr (&oldstat);
    sq = AllocSysMemory (0, ALLOC_SIZE, NULL);
    CpuResumeIntr (oldstat);
    if ((fd = open (gFilename, O_RDONLY)) < 0) {
	printf ("file open failed. %s\n", gFilename);
	return -1;
    }
    if (read (fd, sq, ALLOC_SIZE) < 0){
	printf ("file read failed. %s\n", gFilename);
	return -1;
    }

    // modmidi 全体の初期化
    if (sceMidi_Init (&midiCtx, ONE_240TH) != sceMidiNoError) { // 1/240sec
	printf ("sceMidi_Init Error\n");
	return 1;
    }

    /* modmidi 入力ポート 0 の設定
     * ---------------------------------------------------------------- */

    /* SQ を入力ポート 0 に登録 */
    midiCtx.buffGrp [0].buffCtx [0].buff = sq;
    if (sceMidi_Load (&midiCtx, MIDI_IN_PORT) != sceMidiNoError) {
	printf ("sceMidi_Load(%d) error\n", i);
	return 1;
    }

    /* 入力ポート 0 の設定 */

    // ... 全てのコールバック用データはバッファに置かれる
    midiEnv.chMsgCallBackPrivateData   = (unsigned int)streamBf;
    midiEnv.metaMsgCallBack            = callback_meta_msg;
    midiEnv.metaMsgCallBackPrivateData = (unsigned int)streamBf;
    midiEnv.repeatCallBack             = callback_repeat;
    midiEnv.repeatCallBackPrivateData  = (unsigned int)streamBf;
    midiEnv.excOutPort = 1 << 0; // エクスクルーシブは出力ポート 0 に出力

    // 入力ポート 0 の MIDI データは MIDI ch に関わらず出力ポート 0 に出力
    for (i = 0; i < sceMidiNumMidiCh; i++) {
	midiEnv.outPort [i] =  1 << 0;
    }

    // 入力ポート 0 の演奏対象として、登録された SQ のブロック番号 0 を指定
    if (sceMidi_SelectMidi (&midiCtx, MIDI_IN_PORT, 0) != sceMidiNoError) {
	printf("sceMidi_SelectMidi Error\n");
	return 1;
    }
    // 入力ポート 0 の演奏開始位置を先頭に
    if (sceMidi_MidiSetLocation (&midiCtx, MIDI_IN_PORT, 0) != sceMidiNoError) {
	printf("sceMidi_MidiSetLocation Error\n");
	return 1;
    }
    printf("LOCATION = %d\n", sceMidi_GetEnv(&midiCtx, 1)->position);
	
    /* 演奏
     * ---------------------------------------------------------------- */

    if (sceMidi_MidiPlaySwitch (&midiCtx, MIDI_IN_PORT, sceMidi_MidiPlayStart)
	!= sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n");
	return 1;
    }

    return 0;
}

/* ----------------------------------------------------------------
 * SPU2 設定
 * ---------------------------------------------------------------- */
void
set_spu2 (void)
{
    int i;
    sceSdEffectAttr r_attr;

    // set digital output mode
    sceSdSetCoreAttr (SD_C_SPDIF_MODE, (SD_SPDIF_MEDIA_CD |
					SD_SPDIF_OUT_PCM  |
					SD_SPDIF_COPY_NORMAL));

    for (i = 0; i < 2; i++) {
	/*
	 * エフェクト設定
	 */
	// effect workarea end address を CORE0 と CORE1 とで別の領域に設定
	sceSdSetAddr (i | SD_A_EEA, 0x1fffff - 0x20000 * i);

	// set reverb attribute
	r_attr.depth_L  = 0;
	r_attr.depth_R  = 0;
	r_attr.mode = SD_REV_MODE_STUDIO_A | SD_REV_MODE_CLEAR_WA;
	sceSdSetEffectAttr (i, &r_attr);

	// reverb on
	sceSdSetCoreAttr (i | SD_C_EFFECT_ENABLE, 1);
	sceSdSetParam    (i | SD_P_EVOLL, 0x3fff);
	sceSdSetParam    (i | SD_P_EVOLR, 0x3fff);

	/*
	 * マスターボリューム設定
	 */
	sceSdSetParam (i | SD_P_MVOLL, 0x3fff);
	sceSdSetParam (i | SD_P_MVOLR, 0x3fff);
    }

    // core0 out は core1 のWET側には入れない
    sceSdSetParam (1 | SD_P_MMIX, ~(SD_MMIX_SINEL | SD_MMIX_SINER));

    return;
}

/*-------------------
  メイン
  -------------------*/
int
my_main (void)
{
    TimerCtx timer;
    int count = 0;

    // initialize hardware
#if 0
    // SPUの初期化はEE側のsceSdRemoteInitで行なわれている
    sceSdInit (0);
#endif
    set_spu2 ();

    timer.thread_id = GetThreadId ();
    set_timer (&timer);

    // initialize modules
    set_module_context ();

    set_ssyn ();
    set_midi ();

    // timer start
    start_timer (&timer);

    // main loop
    while (sceMidi_isInPlay (&midiCtx, MIDI_IN_PORT)) {
	atick ();
	SleepThread (); // 仕事の無い時は寝る
    }
	
    printf ("/////// count %d ///////", ++count);

    // timer stop
    stop_timer (&timer);
    clear_timer (&timer);

    printf ("EXIT............\n");

    return 0;
}

/*-------------------------
  mainをスレッドとして作る。
  -------------------------*/
// sdrdrv.irxと同時に動作するので、sdrdrv.irxのプライオリティ(現在のとこ
//   ろ34・変更の可能性あり)よりも低くしておく必要がある。

int
start (int argc, char *argv [])
{
    struct ThreadParam param;
    int thid;

    strcpy (gFilename, argv [1]); // EEからargを受け取っている
    printf ("filename = %s\n", gFilename);

    param.attr         = TH_C;
    param.entry        = my_main;
    param.initPriority = 36;     // sdrdrv.irxよりも低くする
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread (&param);
    if (thid > 0) {
	StartThread (thid,0);
	return RESIDENT_END;	/* 0: program is resident */
    } else {
	return NO_RESIDENT_END;	/* 1: program is deleted */
    }
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
