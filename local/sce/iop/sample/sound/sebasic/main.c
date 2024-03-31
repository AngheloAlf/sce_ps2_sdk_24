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
 *                         sebasic - main.c
 *                        SE Stream Basic Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.0       Sep.??,2000   kaol     for release
 */

#include <kernel.h>
#include <stdio.h>

#include <libsd.h>
#include <csl.h>
#include <cslse.h>
#include <modhsyn.h>

#include <modsein.h>

#define USE_MAKEMSG

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
 * 定数マクロ/変数
 * ---------------------------------------------------------------- */
/* ----------------
 * サイズ/メモリ配置
 * ---------------- */
// 波形データ情報
#define HD0_ADDRESS	((void *) 0x120000)
#define BD0_ADDRESS	((void *) 0x130000)
#define BD0_SIZE	(127536+16) // 64 の倍数に

// BD の SPU2 ローカルメモリ内先頭アドレス
#define SPU_ADDR0	((void *) 0x5010)

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
 * modsein 関連
 * ---------------------------------------------------------------- */
// 出力ポート
#define OUT_PORT_0   0		// 全ての SE ストリームはここに出力
#define OUT_PORT_NUM 1		// 出力ポートの総数

/* CSL context */
static sceCslCtx seinCtx;

/* CSL buffer group */
static sceCslBuffGrp seinGrp [2]; // 常に要素数は 2 ... 0/無し, 1/出力

/* CSL buffer context */
static sceCslBuffCtx seinBfCtx [OUT_PORT_NUM];// 出力ポートの総数

/*
 * SE stream buffer
 */
#define STREAMBUF_SIZE 1024	// バッファサイズ
#define STREAMBUF_SIZE_ALL (sizeof (sceCslSeStream) + STREAMBUF_SIZE) // 属性 + バッファ

// 各入出力ポートが使うバッファ: 入力ポートの総数
static char streamBf [OUT_PORT_NUM][STREAMBUF_SIZE_ALL];

/* ----------------------------------------------------------------
 * modhsyn 関連
 * ---------------------------------------------------------------- */
// 入力ポート
#define HS_PORT_0   0		// OUT_PORT_0 の出力を入力
#define HS_PORT_NUM 1		// 入力ポートの総数
#define HS_BANK_0   15		// HD0/BD0 の HS_PORT_0 でのバンク番号

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
      seinGrp [OUT_BUF].buffNum = OUT_PORT_NUM;	// 出力ポート数
      seinGrp [OUT_BUF].buffCtx = seinBfCtx;
        // 出力ポート 0
	seinBfCtx [OUT_PORT_0].sema = 0;
	seinBfCtx [OUT_PORT_0].buff = &(streamBf [OUT_PORT_0]); // modhsyn と共有

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

    /*
     * SE-stream buffer
     */
    ((sceCslSeStream *) &(streamBf [OUT_PORT_0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslSeStream *) &(streamBf [OUT_PORT_0]))->validsize = 0;

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
    synthEnv [HS_PORT_0].portMode       = sceHSynModeSESyn; // ポートは SE モード
    synthEnv [HS_PORT_0].waveType       = sceHSynTypeHSyn;  // 波形データは従来通り
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

    // modhsyn の状態モニタバッファを設定
    sceHSyn_SetVoiceStatBuffer (pVstat);

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
static int
atick (void)
{
    unsigned int count;
    int finalize;

    count = finalize = 0;
    while (1) {
	SleepThread ();

	switch (count) {
	case 100:
	    printf ("Note on --- Comedy.\n");
	    //                              port, id,      bank, prog, note, vel, pan
#ifdef USE_MAKEMSG
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223344, HS_BANK_0, 0, 48, 0x7f, 64); // ver. 1
#else
	    sceSEIn_NoteOn     (&seinCtx, 0, 0x11223344, HS_BANK_0, 0, 48, 0x7f, 64); // ver. 1
#endif
	    break;
	case 500:
	    printf ("Note on --- Cheers!\n");
	    //                              port, id,      bank, prog, note, vel, pan
#ifdef USE_MAKEMSG
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223345, HS_BANK_0, 0, 72, 0x7f, 64); // ver. 1
#else
	    sceSEIn_NoteOn     (&seinCtx, 0, 0x11223345, HS_BANK_0, 0, 72, 0x7f, 64); // ver. 1
#endif
	    break;
	case 1000:
	    printf ("Note on --- Old telephone.\n");
	    //                               port, id,      bank, prog, note, vel, pan, SPU pitch
#ifdef USE_MAKEMSG
	    sceSEIn_MakePitchOn (&seinCtx, 0, 0x11223346, HS_BANK_0, 0, 96, 0x7f, 64, 0x3fff);
#else
	    sceSEIn_PitchOn     (&seinCtx, 0, 0x11223346, HS_BANK_0, 0, 96, 0x7f, 64, 0x3fff);
#endif
	    break;
	case 2000:
	    printf ("Note off.\n");
#ifdef USE_MAKEMSG
	    //                            port, id,      bank, prog, note, v, pan
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223344, HS_BANK_0, 0, 48, 0, 0);
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223345, HS_BANK_0, 0, 72, 0, 0);
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223346, HS_BANK_0, 0, 96, 0, 0);
#else
	    //                         port, id,      bank, prog, note
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223344, HS_BANK_0, 0, 48);
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223345, HS_BANK_0, 0, 72);
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223346, HS_BANK_0, 0, 96);
#endif
	    break;
	}

	if (count > 2000) {
	    finalize ++;
	}

	// Tick 処理
	sceSEIn_ATick (&seinCtx);	// modsein
	sceHSyn_ATick (&synthCtx);	// modhsyn

	if (finalize) {
	    // 発音ボイスの確認
	    if (! (vstat.pendingVoiceCount || vstat.workVoiceCount)) {
		// 発音されているボイス無し
		SignalSema (main_sema); // start() に処理を返す
	    }
	}

	count ++;
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
