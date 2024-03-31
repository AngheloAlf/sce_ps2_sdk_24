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
 *                         sesqhard - main.c
 *                        SE Sequence Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.0       Nov.,2000     kaol     for release
 */

#include <kernel.h>
#include <stdio.h>

#include <libsd.h>
#include <csl.h>
#include <cslse.h>
#include <modsesq.h>
#include <modhsyn.h>

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
#define SQ_ADDR		0x101000
#define SQ_TOP		0
#define SQ_BLOCK0	0	// 演奏ブロック番号: sakana.sq には 1 つしかない

/* ----------------
 * 波形データ情報
 * ---------------- */
#define HD0_ADDRESS	((void *) 0x120000) // IOP 内
#define BD0_ADDRESS	((void *) 0x130000) // IOP 内
#define BD0_SIZE	(43008+0) // 64 の倍数に

// BD の SPU2 ローカルメモリ内先頭アドレス
#define SPU_ADDR0	((void *) 0x5010)

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
static sceCslCtx sesqCtx;	// modsesq
static sceCslCtx synthCtx;	// modhsyn

/* CSL buffer group */
static sceCslBuffGrp sesqGrp [2]; // modsesq: 常に要素数は 2 ... 0/入力, 1/出力
static sceCslBuffGrp synthGrp;	  // modhsyn: 常に要素数は 1 ... 0/入力

#define SESQ_INPUT_NUM 1
#define SESQ_OUTPUT_NUM 1
#define HSYN_INPUT_NUM 1

/* CSL buffer context */
static sceCslBuffCtx sqinBfCtx  [SESQ_INPUT_NUM * 2];	// modsesq: 入力バッファ
static sceCslBuffCtx sqoutBfCtx [SESQ_OUTPUT_NUM];	// modsesq: 出力ポート
static sceSESqEnv    sesqEnv    [SESQ_INPUT_NUM];	// modsesq: 入力ポート環境
static sceCslBuffCtx synthBfCtx [HSYN_INPUT_NUM * 2];	// modhsyn: 入力バッファ
static sceHSynEnv    synthEnv   [HSYN_INPUT_NUM];	// modhsyn: 入力ポート環境

/* SE stream buffer */
// バッファ実サイズ
#define STREAMBUF_SIZE 1024
// 全体サイズ: 属性 + バッファ
#define STREAMBUF_SIZE_ALL (sizeof (sceCslSeStream) + STREAMBUF_SIZE)
// 各入出力ポートが使うバッファ
static char streamBf [SESQ_OUTPUT_NUM][STREAMBUF_SIZE_ALL];

/* ----------------------------------------------------------------
 * 各 Mudule Context の設定
 * ---------------------------------------------------------------- */
void
set_module_context (void)
{
    /*
     * modsesq
     */
    // CSL context
    sesqCtx.extmod     = NULL;
    sesqCtx.callBack   = NULL;
    sesqCtx.buffGrpNum = 2;	// modsesq は常に 2
    sesqCtx.buffGrp    = sesqGrp;
      // CSL buffer group: 入力
      sesqGrp [ IN_BUF].buffNum = SESQ_INPUT_NUM * 2;	// 入力ポート数 x 2
      sesqGrp [ IN_BUF].buffCtx = sqinBfCtx;
        // CSL buffer context
        sqinBfCtx  [0 * 2 + DATA_BUF].sema = 0;
	sqinBfCtx  [0 * 2 + DATA_BUF].buff = NULL;	// set_sesq() にて設定
	sqinBfCtx  [0 * 2 +  ENV_BUF].sema = 0;
	sqinBfCtx  [0 * 2 +  ENV_BUF].buff = sesqEnv;
      // CSL buffer group: 出力
      sesqGrp [OUT_BUF].buffNum = SESQ_OUTPUT_NUM;	// 出力ポート数
      sesqGrp [OUT_BUF].buffCtx = sqoutBfCtx;
        // 出力ポート 0 ... 全てのメッセージはここに出力
	sqoutBfCtx [0].sema = 0;
	sqoutBfCtx [0].buff = &(streamBf [0]); // modhsyn と共有

    /*
     * modhsyn
     */
    // CSL context
    synthCtx.extmod   	= NULL;
    synthCtx.callBack 	= NULL;
    synthCtx.buffGrpNum = 1;	// modhsyn は常に 1
    synthCtx.buffGrp    = &synthGrp;
      // CSL buffer group: 入力
      synthGrp.buffNum = HSYN_INPUT_NUM * 2;		// 入力ポート数 x 2
      synthGrp.buffCtx = synthBfCtx;
        // modsesq 用
        synthBfCtx [0 * 2 + DATA_BUF].sema = 0;
	synthBfCtx [0 * 2 + DATA_BUF].buff = &(streamBf [0]); // modsesq と共有
	synthBfCtx [0 * 2 +  ENV_BUF].sema = 0;
	synthBfCtx [0 * 2 +  ENV_BUF].buff = &(synthEnv [0]);

    /*
     * SE Sequence stream buffer
     */
    ((sceCslSeStream *) &(streamBf [0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslSeStream *) &(streamBf [0]))->validsize = 0;

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

    /* 入力ポートと HD/BD の設定
     * ---------------------------------------------------------------- */

    // modhsyn 入力ポートの設定
    synthEnv [0].priority       = 0;
    synthEnv [0].portMode       = sceHSynModeSESyn;	// SE モード
    synthEnv [0].waveType       = sceHSynTypeTimbre;	// Timbre Chunk の指定
    synthEnv [0].lfoWaveNum 	= 0;
    synthEnv [0].lfoWaveTbl 	= NULL;
    synthEnv [0].velocityMapNum = 0;
    synthEnv [0].velocityMapTbl = NULL;

    // BD0 を SPU2 ローカルメモリに転送
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD0_ADDRESS, SPU_ADDR0, BD0_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n");
	return (-1);
    }

    // HD0 と BD0 を「入力ポート 0 のバンク番号 0」として登録
    if (sceHSyn_Load (&synthCtx, 0, SPU_ADDR0, HD0_ADDRESS, 0) != sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", 0);
	return (-1);
    }

    // modhsyn の状態モニタバッファを設定
    sceHSyn_SetVoiceStatBuffer (pVstat);

    // SE ボイス使用量
    sceHSyn_SESetMaxVoices (12);

    return 0;
}

/* ----------------------------------------------------------------
 * SE Sequencer セットアップ
 * ---------------------------------------------------------------- */
int
set_sesq (void)
{
    int ret;

    // modsesq 全体の初期化
    if ((ret = sceSESq_Init (&sesqCtx, ONE_240TH)) != sceSESqNoError) { // 1/240sec
	printf ("sceSESq_Init Error\n");
	return (-1);
    }

    /* modsesq 入力ポートの設定
     * ---------------------------------------------------------------- */

    /* SQ を入力ポート 0 に登録 */
    // sesqCtx.buffGrp [IN_BUF].buffCtx [0 * 2 + DATA_BUF].buff = (void *) SQ_ADDR;
    sqinBfCtx  [0 * 2 + DATA_BUF].buff = (void *) SQ_ADDR;
    if (sceSESq_Load (&sesqCtx, 0) != sceSESqNoError) {
	printf ("sceSESq_Load (%d) error\n", 0);
	return (-1);
    }

    /* 出力ポートの指定 */
    // デフォルトの出力ポート
    sesqEnv [0].defaultOutPort = 0;
    // outPort [0].port が以下の値の場合、割り当てを調査しない
    sesqEnv [0].outPort [0].port = sceSESqEnv_NoOutPortAssignment;

    /* 基本属性の設定 */
    sesqEnv [0].masterVolume    = sceSESq_Volume0db;
    sesqEnv [0].masterPanpot    = sceSESq_PanpotCenter;
    sesqEnv [0].masterTimeScale = sceSESq_BaseTimeScale;

    return 0;
}

int
play_sq (int *sesq_id, int set_no, int seq_no)
{

    // 入力ポート 0 の演奏対象として、登録された SQ の SE Sequence Set 
    // 番号と SE Sequence 番号を指定
    if ((*sesq_id = sceSESq_SelectSeq (&sesqCtx, 0, set_no, seq_no))
	== sceSESqError) {
	printf ("sceSESq_SelectSeq Error\n");
	return (-1);
    }

    /* 演奏
     * ---------------------------------------------------------------- */
    // 入力ポート 0
    if (sceSESq_SeqPlaySwitch (&sesqCtx, 0, *sesq_id, sceSESq_SeqPlayStart)
	!= sceSESqNoError) {
	printf ("sceSESq_SeqPlaySwitch Error\n");
	return (-1);
    }

    return 0;
}

int
stop_sq (int *sesq_id)
{
    if (sceSESq_SeqPlaySwitch (&sesqCtx, 0, *sesq_id, sceSESq_SeqPlayStop)
	!= sceSESqNoError) {
	printf ("sceSESq_SeqPlaySwitch Error\n");
	return (-1);
    }

    if (sceSESq_UnselectSeq (&sesqCtx, 0, *sesq_id) != sceSESqNoError) {
	printf ("sceSESq_UnselectSeq Error\n");
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
    int count;
    int sesq_id;

    count = 0;
    while (1) {
	SleepThread ();

	// Tick 処理
	sceSESq_ATick (&sesqCtx);	// modsesq
	sceHSyn_ATick (&synthCtx);	// modhsyn

	count ++;

	switch (count) {
	case 1:
	    printf ("Play --- Alarm whistle\n");
	    play_sq (&sesq_id, 0, 0);
	    break;
	case 400:
	    printf ("Stop --- Alarm whistle\n");
	    stop_sq (&sesq_id);
	    break;

	case 500:
	    printf ("Play --- Clash!\n");
	    play_sq (&sesq_id, 0, 1);
	    break;
	case 900:
	    printf ("Stop --- Clash!\n");
	    stop_sq (&sesq_id);
	    break;

	case 1000:
	    printf ("Play --- Alarm whistle .. (0.5 sec.) .. Clash!\n");
	    play_sq (&sesq_id, 0, 2);
	    break;
	case 1400:
	    printf ("Stop --- Alarm whistle .. (0.5 sec.) .. Clash!\n");
	    stop_sq (&sesq_id);
	    break;

	case 1500:
	    printf ("Play --- (blank: 1 sec.) .. Alarm whistle .. (0.5 sec.) .. Clash!\n");
	    play_sq (&sesq_id, 0, 3);
	    break;
	case 2200:
	    printf ("Stop --- (blank: 1 sec.) .. Alarm whistle .. (0.5 sec.) .. Clash!\n");
	    stop_sq (&sesq_id);
	    break;

	case 2300:
	    printf ("Play --- strings (loop) .. (2 sec.) .. Off\n");
	    play_sq (&sesq_id, 1, 0);
	    break;
	case 3000:
	    printf ("Stop --- strings (loop) .. (2 sec.) .. Off\n");
	    stop_sq (&sesq_id);
	    break;

	case 3100:
	    printf ("Play --- strings (loop) .. (up / 2 sec. .. down / 2 sec.) .. (1 sec.) .. Off\n");
	    play_sq (&sesq_id, 1, 1);
	    break;
	case 4500:
	    printf ("Stop --- strings (loop) .. (up / 2 sec. .. down / 2 sec.) .. (1 sec.) .. Off\n");
	    stop_sq (&sesq_id);
	    break;
	}
	    
#if 0
	if ((count % 240) == 0) {
	    printf (".");
	}
#endif
	if (count > 4600) {
	    break;		/* 演奏終了 */
	}
    }
    SignalSema (main_sema); // start に処理を返す

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
    set_sesq ();		// modsesq

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
	printf ("clearTimer: something wrong ...");
    }

    printf ("Fine ............\n");

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
