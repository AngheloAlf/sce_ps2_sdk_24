/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *                          Version 2.0
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         sqhard - main.c
 *                      MIDI Sequencer Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     2.0.6     Nov.,2000     kaol     New timer functions are used.
 *     2.0       Jun.28,2000   kaol     Two SQ/HD/BD can be handled,
 *                                       and ATick is handled by thread.
 *               Jan.27,2000   kaol     gWaitFlag was not initialized,
 *                                        and modified in intr. routine.
 *     1.20      Nov.28.1999   morita   remove MMIX_SINE setting because
 *                                      it has been default.
 *     0.60      Oct.14.1999   morita   first checked in.
 */

#include <kernel.h>
#include <stdio.h>

#include <libsd.h>
#include <csl.h>
#include <cslmidi.h>
#include <modmidi.h>
#include <modhsyn.h>

#define True 1

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

/* ----------------
 * CallBack の例
 * ---------------- */
int
metaMsgCB (unsigned char metaNo, unsigned char *bf, unsigned int len, unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;

#if 0
    // メタイベント受信
    printf ("META %02x\n", (int) metaNo);
#endif
    return True;
}

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
/* ----------------
 * サイズ/メモリ配置
 * ---------------- */
// 1 つめの演奏データ: SQ0/HD0/BD0
#define SQ0_ADDR	0x101000
#define SQ0_BLOCK0	0	// 演奏ブロック番号: sakana.sq には 1 つしかない
#define HD0_ADDRESS	((void *) 0x120000)
#define BD0_ADDRESS	((void *) 0x130000)
#define BD0_SIZE	(335520+32) // 64 の倍数に
// BD の SPU2 ローカルメモリ内先頭アドレス
#define SPU_ADDR0	((void *) 0x5010)

// 2 つめの演奏データ: SQ1/HD1/BD1
#define SQ1_ADDR	0x104000
#define SQ1_BLOCK0	0	// 演奏ブロック番号: ___.sq には 1 つしかない
#define HD1_ADDRESS	((void *) 0x128000)
#define BD1_ADDRESS	((void *) 0x190000)
#define BD1_SIZE	(474944+0) // 64 の倍数に
#define SPU_ADDR1	((void *) (0x5010 + BD0_SIZE))

/* ----------------------------------------------------------------
 * CSL 関連
 * ---------------------------------------------------------------- */
// CSL buffer group 指定
#define   IN_BUF 0		// Buffer Group 0: 入力 / 環境(オプション)
#define  OUT_BUF 1		// Buffer Group 1: 出力
// CSL buffer context 指定
#define DATA_BUF 0		// context (N*2):   N 番のバッファ
#define  ENV_BUF 1		// context (N*2)+1: N 番のバッファに対応する環境

/* ----------------------------------------------------------------
 * modmidi 関連
 * ---------------------------------------------------------------- */
// 入力ポート
#define  IN_PORT_0   0		// SQ0 用
#define  IN_PORT_1   1		// SQ1 用
#define  IN_PORT_NUM 2		// 入力ポートの総数

// 出力ポート
#define OUT_PORT_0   0		// SQ0 の全ての MIDI ch はここに出力
#define OUT_PORT_1   1		// SQ1 の全ての MIDI ch はここに出力
#define OUT_PORT_NUM 2		// 出力ポートの総数

/* CSL context */
static sceCslCtx midiCtx;

/* CSL buffer group */
static sceCslBuffGrp midiGrp [2]; // 常に要素数は 2 ... 0/入力, 1/出力

/* CSL buffer context */
static sceCslBuffCtx mInBfCtx  [IN_PORT_NUM * 2]; // 入力ポートの総数 x 2
static sceCslBuffCtx mOutBfCtx [OUT_PORT_NUM];    // 出力ポートの総数

/*
 * midi-stream buffer
 */
#define STREAMBUF_SIZE 1024	// バッファサイズ
#define STREAMBUF_SIZE_ALL (sizeof (sceCslMidiStream) + STREAMBUF_SIZE) // バッファ + 属性

// 各入出力ポートが使うバッファ: 入力ポートの総数
static char streamBf [IN_PORT_NUM][STREAMBUF_SIZE_ALL];
// 環境構造体: 入力ポートの総数
static sceMidiEnv midiEnv [IN_PORT_NUM];

#define SQ_TOP 0

/* ----------------------------------------------------------------
 * modhsyn 関連
 * ---------------------------------------------------------------- */
// 入力ポート
#define HS_PORT_0 0		// OUT_PORT_0 の出力を入力
#define HS_PORT_1 1		// OUT_PORT_1 の出力を入力
#define HS_PORT_NUM 2		// 入力ポートの総数
#define HS_BANK_0 0		// HD0/BD0 の HS_PORT_0 でのバンク番号
#define HS_BANK_1 0		// HD1/BD1 の HS_PORT_1 でのバンク番号

/* CSL context */
static sceCslCtx synthCtx;

/* CSL buffer group */
static sceCslBuffGrp synthGrp;

/* CSL buffer context */
static sceCslBuffCtx sInBfCtx [HS_PORT_NUM * 2]; // 入力ポートの総数 x 2
static sceHSynEnv    synthEnv [HS_PORT_NUM];     // 入力ポートの総数

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
      midiGrp [ IN_BUF].buffNum = IN_PORT_NUM * 2;	// 入力ポート数 x 2
      midiGrp [ IN_BUF].buffCtx = mInBfCtx;
        // CSL buffer context
        // SQ0 用
        mInBfCtx  [IN_PORT_0 * 2 + DATA_BUF].sema = 0;
	mInBfCtx  [IN_PORT_0 * 2 + DATA_BUF].buff = NULL; // set_midi() にて設定
	mInBfCtx  [IN_PORT_0 * 2 +  ENV_BUF].sema = 0;
	mInBfCtx  [IN_PORT_0 * 2 +  ENV_BUF].buff = &(midiEnv [IN_PORT_0]);
        // SQ1 用
        mInBfCtx  [IN_PORT_1 * 2 + DATA_BUF].sema = 0;
	mInBfCtx  [IN_PORT_1 * 2 + DATA_BUF].buff = NULL; // set_midi() にて設定
	mInBfCtx  [IN_PORT_1 * 2 +  ENV_BUF].sema = 0;
	mInBfCtx  [IN_PORT_1 * 2 +  ENV_BUF].buff = &(midiEnv [IN_PORT_1]);
      // CSL buffer group: 出力
      midiGrp [OUT_BUF].buffNum = OUT_PORT_NUM;		// 出力ポート数
      midiGrp [OUT_BUF].buffCtx = mOutBfCtx;
        // 出力ポート 0 ... SQ0 の全ての MIDI ch はここに出力
	mOutBfCtx [OUT_PORT_0              ].sema = 0;
	mOutBfCtx [OUT_PORT_0              ].buff = &(streamBf [OUT_PORT_0]); // modhsyn と共有
        // 出力ポート 1 ... SQ1 の全ての MIDI ch はここに出力
	mOutBfCtx [OUT_PORT_1              ].sema = 0;
	mOutBfCtx [OUT_PORT_1              ].buff = &(streamBf [OUT_PORT_1]); // modhsyn と共有

    /*
     * modhsyn
     */
    // CSL context
    synthCtx.extmod   	= NULL;
    synthCtx.callBack 	= NULL;
    synthCtx.buffGrpNum = 1;	// modhsyn は常に 1
    synthCtx.buffGrp    = &synthGrp;
      // CSL buffer group: 入力
      synthGrp.buffNum = HS_PORT_NUM * 2;	// 入力ポート数 x 2
      synthGrp.buffCtx = sInBfCtx;
        // OUT_PORT_0 用
        sInBfCtx [HS_PORT_0 * 2 + DATA_BUF].sema = 0;
	sInBfCtx [HS_PORT_0 * 2 + DATA_BUF].buff = &(streamBf [OUT_PORT_0]); // modmidiと共有
	sInBfCtx [HS_PORT_0 * 2 +  ENV_BUF].sema = 0;
	sInBfCtx [HS_PORT_0 * 2 +  ENV_BUF].buff = &(synthEnv [HS_PORT_0]);
        // OUT_PORT_1 用
        sInBfCtx [HS_PORT_1 * 2 + DATA_BUF].sema = 0;
	sInBfCtx [HS_PORT_1 * 2 + DATA_BUF].buff = &(streamBf [OUT_PORT_1]); // modmidiと共有
	sInBfCtx [HS_PORT_1 * 2 +  ENV_BUF].sema = 0;
	sInBfCtx [HS_PORT_1 * 2 +  ENV_BUF].buff = &(synthEnv [HS_PORT_1]);

    /*
     * midi-stream buffer
     */
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_0]))->validsize = 0;
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_1]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_1]))->validsize = 0;

    return;
}

/* ----------------------------------------------------------------
 * Hardware Synthesizer セットアップ
 * ---------------------------------------------------------------- */
int
set_hsyn (sceHSyn_VoiceStat* pVstat)
{
    // modhsyn 全体の初期化
    if (sceHSyn_Init (&synthCtx, ONE_240TH) != sceHSynNoError) { // 1/240sec
	printf ("sceHSyn_Init Error\n");
	return (-1);
    }

    /* 入力ポート 0 と HD0/BD0 の設定
     * ---------------------------------------------------------------- */

    // modhsyn 入力ポート 0 の設定
    synthEnv [HS_PORT_0].priority       = 0;
    synthEnv [HS_PORT_0].lfoWaveNum 	= 0;
    synthEnv [HS_PORT_0].lfoWaveTbl 	= NULL;
    synthEnv [HS_PORT_0].velocityMapNum = 0;
    synthEnv [HS_PORT_0].velocityMapTbl = NULL;

    // BD0 を SPU2 ローカルメモリに転送
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD0_ADDRESS, SPU_ADDR0, BD0_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n");
	return (-1);
    }

    // HD0 と BD0 を「入力ポート 0 のバンク番号 HS_BANK_0 (= 0)」として登録
    if (sceHSyn_Load (&synthCtx, HS_PORT_0, SPU_ADDR0, HD0_ADDRESS, HS_BANK_0)
	!= sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", HS_PORT_0);
	return (-1);
    }

    /* 入力ポート 1 と HD1/BD1 の設定
     * ---------------------------------------------------------------- */

    // modhsyn 入力ポート 1 の設定
    synthEnv [HS_PORT_1].priority       = 0;
    synthEnv [HS_PORT_1].lfoWaveNum 	= 0;
    synthEnv [HS_PORT_1].lfoWaveTbl 	= NULL;
    synthEnv [HS_PORT_1].velocityMapNum = 0;
    synthEnv [HS_PORT_1].velocityMapTbl = NULL;

    // BD1 を SPU2 ローカルメモリに転送
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD1_ADDRESS, SPU_ADDR1, BD1_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n");
	return (-1);
    }

    // HD1 と BD1 を「入力ポート 1 のバンク番号 HS_BANK_1 (= 0)」として登録
    if (sceHSyn_Load (&synthCtx, HS_PORT_1, SPU_ADDR1, HD1_ADDRESS, HS_BANK_1)
	!= sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", HS_PORT_1);
	return (-1);
    }

    // modhsyn の状態モニタバッファを設定
    sceHSyn_SetVoiceStatBuffer (pVstat);

    return 0;
}

/* ----------------------------------------------------------------
 * Midi Sequencer セットアップ
 * ---------------------------------------------------------------- */
int
set_midi (void)
{
    int i;

    // modmidi 全体の初期化
    if (sceMidi_Init (&midiCtx, ONE_240TH) != sceMidiNoError) { // 1/240sec
	printf ("sceMidi_Init Error\n");
	return (-1);
    }

    /* modmidi 入力ポート 0 の設定
     * ---------------------------------------------------------------- */

    /* SQ0 を入力ポート 0 に登録 */
    // midiCtx.buffGrp [IN_BUF].buffCtx [IN_PORT_0 * 2 + DATA_BUF].buff = (void *) SQ0_ADDR;
    mInBfCtx  [IN_PORT_0 * 2 + DATA_BUF].buff = (void *) SQ0_ADDR;
    if (sceMidi_Load (&midiCtx, IN_PORT_0) != sceHSynNoError) {
	printf ("sceMidi_Load (%d) error\n", i);
	return (-1);
    }

    /* 入力ポート 0 の設定 */

    // ... 全てのコールバック用データはバッファに置かれる
    midiEnv [IN_PORT_0].chMsgCallBackPrivateData   = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].metaMsgCallBack            = metaMsgCB;
    midiEnv [IN_PORT_0].metaMsgCallBackPrivateData = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].repeatCallBack             = repeatCB;
    midiEnv [IN_PORT_0].repeatCallBackPrivateData  = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].excOutPort = 1 << OUT_PORT_0; // エクスクルーシブは出力ポート 0 に出力

    // 入力ポート 0 の MIDI データは MIDI ch に関わらず出力ポート 0 に出力
    for (i = 0; i < sceMidiNumMidiCh; i++) { // i: MIDI ch
	midiEnv [IN_PORT_0].outPort [i] =  1 << OUT_PORT_0;
    }

    // 入力ポート 0 の演奏対象として、登録された SQ (SQ0) のブロック番号 0 を指定
    if (sceMidi_SelectMidi (&midiCtx, IN_PORT_0, SQ0_BLOCK0) != sceMidiNoError) {
	printf ("sceMidi_SelectMidi Error\n");
	return (-1);
    }
    // 入力ポート 0 の演奏開始位置を先頭に
    if (sceMidi_MidiSetLocation (&midiCtx, IN_PORT_0, SQ_TOP) != sceMidiNoError) {
	printf ("sceMidi_MidiSetLocation Error\n");
	return (-1);
    }
#if 0
    printf ("LOCATION = %d\n", sceMidi_GetEnv (&midiCtx, IN_PORT_0)->position); // 位置の確認
#endif

    /* modmidi 入力ポート 1 の設定
     * ---------------------------------------------------------------- */

    /* SQ1 を入力ポート 0 に登録 */
    // midiCtx.buffGrp [IN_BUF].buffCtx [IN_PORT_1 * 2 + DATA_BUF].buff = (void *) SQ1_ADDR;
    mInBfCtx  [IN_PORT_1 * 2 + DATA_BUF].buff = (void *) SQ1_ADDR;
    if (sceMidi_Load (&midiCtx, IN_PORT_1) != sceHSynNoError) {
	printf ("sceMidi_Load (%d) error\n", i);
	return (-1);
    }

    /* 入力ポート 1 の設定 */
    // ... 全てのコールバック用データはバッファに置かれる
    midiEnv [IN_PORT_1].chMsgCallBackPrivateData   = (unsigned int) &(streamBf [IN_PORT_1]);
    midiEnv [IN_PORT_1].metaMsgCallBack            = metaMsgCB;
    midiEnv [IN_PORT_1].metaMsgCallBackPrivateData = (unsigned int) &(streamBf [IN_PORT_1]);
    midiEnv [IN_PORT_1].repeatCallBack             = repeatCB;
    midiEnv [IN_PORT_1].repeatCallBackPrivateData  = (unsigned int) &(streamBf [IN_PORT_1]);
    midiEnv [IN_PORT_1].excOutPort = 1 << OUT_PORT_1; // エクスクルーシブは出力ポート 1 に出力
    for (i = 0; i < sceMidiNumMidiCh; i++) { // i: MIDI ch
	// 入力ポート 1 の MIDI データは MIDI ch に関わらず出力ポート 1 に出力
	midiEnv [IN_PORT_1].outPort [i] =  1 << OUT_PORT_1;
    }

    // 入力ポート 1 の演奏対象として、登録された SQ (SQ1) のブロック番号 0 を指定
    if (sceMidi_SelectMidi (&midiCtx, IN_PORT_1, SQ1_BLOCK0) != sceMidiNoError) {
	printf ("sceMidi_SelectMidi Error\n");
	return (-1);
    }

    // 入力ポート 1 の演奏開始位置を先頭に
    if (sceMidi_MidiSetLocation (&midiCtx, IN_PORT_1, SQ_TOP) != sceMidiNoError) {
	printf ("sceMidi_MidiSetLocation Error\n");
	return (-1);
    }
#if 0
    printf ("LOCATION = %d\n", sceMidi_GetEnv (&midiCtx, IN_PORT_1)->position); // 位置の確認
#endif

    /* 演奏
     * ---------------------------------------------------------------- */
    // 入力ポート 0
    if (sceMidi_MidiPlaySwitch (&midiCtx, IN_PORT_0, sceMidi_MidiPlayStart) != sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n");
	return (-1);
    }
    // 入力ポート 1
    if (sceMidi_MidiPlaySwitch (&midiCtx, IN_PORT_1, sceMidi_MidiPlayStart) != sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n");
	return (-1);
    }

    return 0;
}

/* ----------------------------------------------------------------
 * 1 tick 毎の処理
 * ---------------------------------------------------------------- */
static int
atick (void)
{
    while (1) {
	SleepThread ();

	// Tick 処理
	sceMidi_ATick (&midiCtx);	// modmidi
	sceHSyn_ATick (&synthCtx);	// modhsyn

	if (! (sceMidi_isInPlay (&midiCtx, IN_PORT_0) ||
	       sceMidi_isInPlay (&midiCtx, IN_PORT_1))) {
	    // 曲が終了
	    if (! (vstat.pendingVoiceCount || vstat.workVoiceCount)) {
		// 発音されているボイス無し
		SignalSema (main_sema); // start() に処理を返す
	    }
	}
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

    // initialize hardware
    sceSdInit (0);
    set_spu2 ();		// Digital effect/master volume

    // initialize modules environment
    set_module_context ();	// CSL module context
    set_hsyn (&vstat);		// modhsyn
    set_midi ();		// modmidi

    // semaphore: for finishing the play
    main_sema = make_semaphore ();

    // Thread: atick() process
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
