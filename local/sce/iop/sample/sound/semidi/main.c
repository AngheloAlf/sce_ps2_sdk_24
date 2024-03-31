/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *                          Version 2.0
 *                           Shift-JIS
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         semidply - main.c
 *                      SE & MIDI Sequence Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.0       Sep.??,2000   kaol     for release
 */

#include <kernel.h>
#include <stdio.h>
#include <libsd.h>
#include <csl.h>
#include <cslmidi.h>
#include <cslse.h>
#include <modmidi.h>
#include <modhsyn.h>

// #define sceSEInCommandVersion_0
#include <modsein.h>

volatile int main_sema;		// サウンド処理を待つセマフォ
sceHSyn_VoiceStat vstat;	// modhsyn 状態モニタバッファ

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

    iWakeupThread (tc->thread_id); // wakeup ATick()

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

    if (StartHardTimer (timer_id) != KE_OK) {
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
    int ret;

    ret = StopHardTimer (timer->timer_id);
    if (! (ret == KE_OK || ret == KE_TIMER_NOT_INUSE)) {
	printf ("Can NOT stop hard timer ...\n");
	return (-1);
    }

    if (FreeHardTimer (timer->timer_id) != KE_OK) {
	printf ("Can NOT free hard timer ...\n");
	return (-1);
    }
    return 0;
}

/* ----------------------------------------------------------------
 *  CallBackの例
 * ---------------------------------------------------------------- */
/* ----------------
 * Meta Event
 * ---------------- */
int
metaMsgCB (unsigned char metaNo, unsigned char *bf, unsigned int len, unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;
#if 0
    printf ("META %02x\n", (int) metaNo);
#endif
    return True;
}

/* ----------------
 * Repeat
 * ---------------- */
int
repeatCB (sceMidiLoopInfo *pInf, unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;
#if 0
    printf ("Repeat %s %d %d %x\n",
	    (pInf->type == sceMidiLoopInfoType_Midi) ? "Midi" : "Song",
	    (int) pInf->loopTimes, (int) pInf->loopCount, pInf->loopId);
#endif
    return True;
}

/* ----------------------------------------------------------------
 * 定数マクロ/変数
 * ---------------------------------------------------------------- */
#define SQ_ADDR		0x101000
#define SQ_TOP		0
#define SQ_BLOCK0	0	// 演奏ブロック番号: sakana.sq には 1 つしかない

/* ----------------
 * 波形データ情報
 * ---------------- */
#define HD0_ADDRESS	((void *) 0x120000) // IOP 内
#define BD0_ADDRESS	((void *) 0x130000) // IOP 内
#define BD0_SIZE	(335520+32) // 64 の倍数に
#define HD1_ADDRESS	((void *) 0x128000) // IOP 内
#define BD1_ADDRESS	((void *) 0x190000) // IOP 内
#define BD1_SIZE	(127536+16) // 64 の倍数に
// BD の SPU2 ローカルメモリ内先頭アドレス
#define SPU_ADDR0	((void *) 0x5010)
#define SPU_ADDR1	((void *) (0x5010 + BD0_SIZE))

/* ----------------
 * CSL 関連
 * ---------------- */
#define   IN_BUF 0		// Buffer Group 0: 入力 / 環境(オプション)
#define  OUT_BUF 1		// Buffer Group 1: 出力
#define DATA_BUF 0		// context (N*2):   N 番のバッファ
#define  ENV_BUF 1		// context (N*2)+1: N 番のバッファに対応する環境

/* ----------------
 * モジュール関連
 * ---------------- */
/* CSL context */
static sceCslCtx midiCtx;	// modmidi
static sceCslCtx seinCtx;	// modsein
static sceCslCtx synthCtx;	// modhsyn

/* CSL buffer group */
static sceCslBuffGrp midiGrp [2]; // modmidi: 常に要素数は 2 ... 0/入力, 1/出力
static sceCslBuffGrp seinGrp [2]; // modsein: 常に要素数は 2 ... 0/入力, 1/出力
static sceCslBuffGrp synthGrp;	  // modhsyn: 常に要素数は 1 ... 0/入力

/* CSL buffer context */
static sceCslBuffCtx minBfCtx   [1 * 2];	// modmidi: 入力バッファ
static sceCslBuffCtx moutBfCtx  [1];		// modmidi: 出力ポート
static sceMidiEnv    midiEnv    [1];		// modmidi: 入力ポート環境
static sceCslBuffCtx seinBfCtx  [1];		// modsein: 出力ポート
static sceCslBuffCtx synthBfCtx [2 * 2];	// modhsyn: 入力バッファ
static sceHSynEnv    synthEnv   [2];		// modhsyn: 入力ポート環境

/* SE stream buffer */
// バッファ実サイズ
#define STREAMBUF_SIZE 1024
// 全体サイズ: 属性 + バッファ
#define STREAMBUF_SIZE_ALL (sizeof (sceCslSeStream) + STREAMBUF_SIZE)
// 各入出力ポートが使うバッファ
static char streamBf [2][STREAMBUF_SIZE_ALL];

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
      midiGrp [ IN_BUF].buffNum = 1 * 2;	// 入力ポート数 x 2
      midiGrp [ IN_BUF].buffCtx = minBfCtx;
        // CSL buffer context
        minBfCtx  [0 * 2 + DATA_BUF].sema = 0;
	minBfCtx  [0 * 2 + DATA_BUF].buff = NULL; // set_midi() にて設定
	minBfCtx  [0 * 2 +  ENV_BUF].sema = 0;
	minBfCtx  [0 * 2 +  ENV_BUF].buff = midiEnv;
      // CSL buffer group: 出力
      midiGrp [OUT_BUF].buffNum = 1;		// 出力ポート数
      midiGrp [OUT_BUF].buffCtx = moutBfCtx;
        // 出力ポート 0 ... SQ0 の全ての MIDI ch はここに出力
	moutBfCtx [0].sema = 0;
	moutBfCtx [0].buff = &(streamBf [0]); // modhsyn と共有

    /*
     * modsein
     */
    // CSL context
    seinCtx.extmod     = NULL;
    seinCtx.callBack   = NULL;
    seinCtx.buffGrpNum = 2;	// modsein は常に 2
    seinCtx.buffGrp    = seinGrp;
      // CSL buffer group: 入力
      seinGrp [ IN_BUF].buffNum = 0;
      seinGrp [ IN_BUF].buffCtx = NULL;
      // CSL buffer group: 出力
      seinGrp [OUT_BUF].buffNum = 1;	// 出力ポート数
      seinGrp [OUT_BUF].buffCtx = seinBfCtx;
        // 出力ポート 0
	seinBfCtx [0].sema = 0;
	seinBfCtx [0].buff = &(streamBf [1]); // modhsyn と共有

    /*
     * modhsyn
     */
    // CSL context
    synthCtx.extmod   	= NULL;
    synthCtx.callBack 	= NULL;
    synthCtx.buffGrpNum = 1;	// modhsyn は常に 1
    synthCtx.buffGrp    = &synthGrp;
      // CSL buffer group: 入力
      synthGrp.buffNum = 2 * 2;	// 入力ポート数 x 2
      synthGrp.buffCtx = synthBfCtx;
        // modmidi 用
        synthBfCtx [0 * 2 + DATA_BUF].sema = 0;
	synthBfCtx [0 * 2 + DATA_BUF].buff = &(streamBf [0]); // modmidi と共有
	synthBfCtx [0 * 2 +  ENV_BUF].sema = 0;
	synthBfCtx [0 * 2 +  ENV_BUF].buff = &(synthEnv [0]);
        // modsein 用
        synthBfCtx [1 * 2 + DATA_BUF].sema = 0;
	synthBfCtx [1 * 2 + DATA_BUF].buff = &(streamBf [1]); // modsein と共有
	synthBfCtx [1 * 2 +  ENV_BUF].sema = 0;
	synthBfCtx [1 * 2 +  ENV_BUF].buff = &(synthEnv [1]);

    /*
     * MIDI/SE stream buffer
     */
    ((sceCslSeStream *) &(streamBf [0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslSeStream *) &(streamBf [0]))->validsize = 0;
    ((sceCslSeStream *) &(streamBf [1]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslSeStream *) &(streamBf [1]))->validsize = 0;

    return;
}

/* ----------------------------------------------------------------
 * Midi Sequencerセットアップ
 * ---------------------------------------------------------------- */
int
set_midi (void)
{
    int i;

    // modmidi 全体の初期化
    if (sceMidi_Init (&midiCtx, 4167) != sceMidiNoError) { // 4167 = 1/240sec
	printf ("sceMidi_Init Error\n"); return (-1);
    }

    /* modmidi 入力ポート 0 の設定
     * ---------------------------------------------------------------- */

    /* SQ を入力ポート 0 に登録 */
    // midiCtx.buffGrp [IN_BUF].buffCtx [0 * 2 + DATA_BUF].buff = (void *) SQ_ADDR;
    minBfCtx  [0 * 2 + DATA_BUF].buff = (void *) SQ_ADDR;
    if (sceMidi_Load (&midiCtx, 0) != sceHSynNoError) {
	printf ("sceMidi_Load (%d) error\n", 0); return (-1);
    }

    /* 入力ポート 0 の設定 */
    // ... 全てのコールバック用データはバッファに置かれる
    midiEnv [0].chMsgCallBackPrivateData   = (unsigned int) &(streamBf [0]);
    midiEnv [0].metaMsgCallBack            = metaMsgCB;
    midiEnv [0].metaMsgCallBackPrivateData = (unsigned int) &(streamBf [0]);
    midiEnv [0].repeatCallBack             = repeatCB;
    midiEnv [0].repeatCallBackPrivateData  = (unsigned int) &(streamBf [0]);
    midiEnv [0].excOutPort = 1 << 0; // エクスクルーシブは出力ポート 0 に出力
    for (i = 0; i < sceMidiNumMidiCh; i++) { // i: MIDI ch
	// 入力ポート 0 の MIDI データは MIDI ch に関わらず出力ポート 0 に出力
	midiEnv [0].outPort [i] = 1 << 0;
    }

    // 入力ポート 0 の演奏対象として、登録された SQ (SQ0) のブロック番号 0 を指定
    if (sceMidi_SelectMidi (&midiCtx, 0, SQ_BLOCK0) != sceMidiNoError) {
	printf ("sceMidi_SelectMidi Error\n"); return (-1);
    }
    // 入力ポート 0 の演奏開始位置を先頭に
    if (sceMidi_MidiSetLocation (&midiCtx, 0, SQ_TOP) != sceMidiNoError) {
	printf ("sceMidi_MidiSetLocation Error\n"); return (-1);
    }
    // printf ("LOCATION = %d\n", sceMidi_GetEnv (&midiCtx, 0)->position); // 位置の確認

    /* 演奏
     * ---------------------------------------------------------------- */
    // 入力ポート 0
    if (sceMidi_MidiPlaySwitch (&midiCtx, 0, sceMidi_MidiPlayStart) != sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n"); return (-1);
    }

    return 0;
}

/* ----------------------------------------------------------------
 * Hardware Synthesizer セットアップ
 * ---------------------------------------------------------------- */
int
set_hsyn (sceHSyn_VoiceStat* pVstat)
{
    // modhsyn 全体の初期化
    if (sceHSyn_Init (&synthCtx, 4167) != sceHSynNoError) { // 4167 = 1/240sec
	printf ("sceHSyn_Init Error\n"); return (-1);
    }

    /* 入力ポート 0 と HD/BD の設定
     * ---------------------------------------------------------------- */

    // modhsyn 入力ポート 0 の設定
    synthEnv [0].priority       = 0;
#if 0 /* デフォルトでこの設定 */
    synthEnv [0].portMode       = sceHSynModeHSyn;	// 通常モード
    synthEnv [0].waveType       = sceHSynTypeHSyn;	// 波形データは従来通り
#endif
    synthEnv [0].lfoWaveNum 	= 0;
    synthEnv [0].lfoWaveTbl 	= NULL;
    synthEnv [0].velocityMapNum = 0;
    synthEnv [0].velocityMapTbl = NULL;

    // BD0 を SPU2 ローカルメモリに転送
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD0_ADDRESS, SPU_ADDR0, BD0_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n"); return (-1);
    }

    // HD0 と BD0 を「入力ポート 0 のバンク番号 0」として登録
    if (sceHSyn_Load (&synthCtx, 0, SPU_ADDR0, HD0_ADDRESS, 0)
	!= sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", 0); return (-1);
    }

    /* 入力ポート 1 と HD/BD の設定
     * ---------------------------------------------------------------- */

    // modhsyn 入力ポート 0 の設定
    synthEnv [1].priority       = 0;
    synthEnv [1].portMode       = sceHSynModeSESyn;	// ポートは SE 用
    synthEnv [1].waveType       = sceHSynTypeHSyn;	// 波形データは従来通り
    synthEnv [1].lfoWaveNum 	= 0;
    synthEnv [1].lfoWaveTbl 	= NULL;
    synthEnv [1].velocityMapNum = 0;
    synthEnv [1].velocityMapTbl = NULL;

    // BD1 を SPU2 ローカルメモリに転送
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD1_ADDRESS, SPU_ADDR1, BD1_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n"); return (-1);
    }

    // HD1 と BD1 を「入力ポート 1 のバンク番号 0」として登録
    if (sceHSyn_Load (&synthCtx, 1, SPU_ADDR1, HD1_ADDRESS, 0)
	!= sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", 1); return (-1);
    }

    // modhsyn の状態モニタバッファを設定
    sceHSyn_SetVoiceStatBuffer (pVstat);

    // SE ボイス使用量
    sceHSyn_SESetMaxVoices (12);

    return 0;
}

/* ----------------------------------------------------------------
 * SE Stream input セットアップ
 * ---------------------------------------------------------------- */
int
set_sein (void)
{
    // modmidi 全体の初期化
    if (sceSEIn_Init (&seinCtx) != sceSEInNoError) {
	printf ("sceSEIn_Init Error\n");
	return (-1);
    }
    return 0;
}

/* ----------------------------------------------------------------
 * 1 tick 毎の処理
 * ---------------------------------------------------------------- */
//static char vnum [48];

static int
atick (void)
{
    unsigned int count;
    int finalize;
    //char mnum;

    count = finalize = 0;
    while (1) {
	SleepThread ();

	switch (count) {
	case 100:
#ifdef sceSEInCommandVersion_0
	    sceSEIn_NoteOn (&seinCtx,
			    0,		// modsein port number
			    0x11223344,	// id
			    0,		// bank number
			    0,		// program number
			    48,		// note number
			    0x7f);	// velocity
#else
	    sceSEIn_NoteOn (&seinCtx,
			    0,		// modsein port number
			    0x11223344,	// id
			    0,		// bank number
			    0,		// program number
			    48,		// note number
			    0x7f,	// velocity
			    40);	// panpot
#endif
	    break;
	case 500:
#ifdef sceSEInCommandVersion_0
	    sceSEIn_NoteOn (&seinCtx, 0, 0x11223345, 0, 0, 72, 0x7f);
#else
	    sceSEIn_NoteOn (&seinCtx, 0, 0x11223345, 0, 0, 72, 0x7f, 64);
#endif
	    break;
	case 900:
#ifdef sceSEInCommandVersion_0
	    sceSEIn_NoteOn (&seinCtx, 0, 0x11223346, 0, 0, 96, 0x7f);
#else
	    sceSEIn_NoteOn (&seinCtx, 0, 0x11223346, 0, 0, 96, 0x7f, 64);
#endif
	    break;
	case 1500:
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223344, 0, 0, 48);
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223345, 0, 0, 72);
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223346, 0, 0, 96);
	    break;
	}

	// Tick 処理
	sceMidi_ATick (&midiCtx);	// modmidi
	sceSEIn_ATick (&seinCtx);	// modsein
	sceHSyn_ATick (&synthCtx);	// modhsyn

	/*
	// 指定した SE ID で使用されているボイス数取得の例:
	//     mnum には指定した ID で発音しているボイス数 (上限 10)
	//     vnum には指定した ID で発音しているボイス番号 (上限 10)
	mnum = sceHSyn_SERetrieveVoiceNumberByID (&synthCtx,
						  1, // modhsyn port
						  0x11223344, // id
						  vnum,
						  10); // max
	// printf ("%02d\n", mnum);
	// ...
	*/					  

	if (! (sceMidi_isInPlay (&midiCtx, 0))) {
	    // 曲が終了
	    if (! finalize) {
		sceSEIn_NoteOff (&seinCtx, 0, 0x11223344, 0, 0, 48);
		sceSEIn_NoteOff (&seinCtx, 0, 0x11223345, 0, 0, 72);
		sceSEIn_NoteOff (&seinCtx, 0, 0x11223346, 0, 0, 96);
		finalize ++;
	    }
	    if (! (vstat.pendingVoiceCount || vstat.workVoiceCount)) {
		// ボイス処理終了
		SignalSema (main_sema); // start に処理を返す
	    }
	}

	count ++;
	if (count > 2000) count = 0;
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
	r_attr.mode = SD_REV_MODE_HALL | SD_REV_MODE_CLEAR_WA;
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

    return;
}

/* ----------------------------------------------------------------
 * 処理終了待ちセマフォの作成
 * ---------------------------------------------------------------- */
int
make_semaphore (void)
{
    struct SemaParam sema;

    sema.initCount = 0;
    sema.maxCount  = 1;
    sema.attr      = AT_THFIFO;

    /* セマフォ作成 */
    return (CreateSema (&sema));
}

/* ----------------------------------------------------------------
 * tick 処理用スレッドの作成
 * ---------------------------------------------------------------- */
int
make_thread (void)
{
    struct ThreadParam param;

    param.attr         = TH_C;
    param.entry        = atick;
    param.initPriority = 32 - 3;
    param.stackSize    = 0x800;
    param.option       = 0;

    /* スレッド作成 */
    return (CreateThread (&param));
}

/* ----------------------------------------------------------------
 * メイン処理
 * ---------------------------------------------------------------- */
int
start (int argc, char *argv [])
{
    int thid;
    TimerCtx timer;

    // initialize SPU2 hardware
    sceSdInit (0);
    set_spu2 ();		// Digital effect/master volume

    // initialize modules environment
    set_module_context ();	// CSL module context
    set_hsyn (&vstat);		// modhsyn
    set_midi ();		// modmidi
    set_sein ();		// modsein

    // semaphore: for finishing the play
    main_sema = make_semaphore ();

    // Thread: ATick() process
    thid = make_thread ();
    StartThread (thid, (unsigned long) NULL);

    // set timer
    timer.thread_id = thid;
    if (set_timer (&timer) < 0) {
	printf ("set_timer: something wrong ...");
    }

    // waiting for finishing the play
    WaitSema (main_sema);

    // timer disable
    if (clear_timer (&timer) < 0) {
	printf ("clear_timer: something wrong ...");
    }

    printf ("Fine ............\n");

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
