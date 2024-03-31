/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *                          Version 2.0
 *                           Shift-JIS
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         sqsong - main.c
 *                      SONG Sequencer Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.00      Jun. 2001     kaol     first checked in.
 */

#include <kernel.h>
#include <stdio.h>

#include <libsd.h>
#include <csl.h>
#include <cslmidi.h>
#include <modmidi.h>
#include <modhsyn.h>

#define MIDI_CHUNK 0
#define SONG_CHUNK 1

/* --- PLAY_CHUNK --- */
/* SONG 演奏の場合、SONG_CHUNK を指定する。
 *  MIDI 演奏の場合、MIDI_CHUNK を指定する。
 *  ... 通常、複数の MIDI Block にて SONG が形成されるため、MIDI_CHUNK を
 *      指定した場合、使用されるデータによっては曲の一部しか演奏されない */
#define PLAY_CHUNK SONG_CHUNK

/* ================================================================ */
/*  Constant macros / variables					    */
/* 定数マクロ/変数						    */
/* ================================================================ */

/* ----------------------------------------------------------------
 * size / memory allocation
 * ---------------------------------------------------------------- */
// SQ0/HD0/BD0
#define SQ0_ADDRESS	0x101000
#define HD0_ADDRESS	((void *) 0x120000)
#define BD0_ADDRESS	((void *) 0x130000)
#define BD0_SIZE	(474944+0) // 64 の倍数に
// BD top address in SPU2 local memory
#define SPU_ADDR0	((void *) 0x5010)

volatile int main_sema;		/* サウンド処理を待つセマフォ */

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
 * modmidi
 * ---------------------------------------------------------------- */
/* input port */
#define  IN_PORT_0   0		// For SQ0
#define  IN_PORT_NUM 1		// 入力ポートの総数

/* output port */
#define OUT_PORT_0   0		// SQ0 の全ての MIDI ch はここに出力
#define OUT_PORT_NUM 1		// 出力ポートの総数

/* CSL context */
static sceCslCtx midiCtx;

/* CSL buffer group */
static sceCslBuffGrp midiGrp [2]; // 常に要素数は 2 ... 0/入力, 1/出力

/* CSL buffer context */
static sceCslBuffCtx midiIn  [IN_PORT_NUM * 2]; // 入力ポートの総数 x 2
static sceCslBuffCtx midiOut [OUT_PORT_NUM];    // 出力ポートの総数

/* midi-stream buffer */
#define STREAMBUF_SIZE 1024	// バッファサイズ
#define STREAMBUF_SIZE_ALL (sizeof (sceCslMidiStream) + STREAMBUF_SIZE) // バッファ + 属性

// 各入出力ポートが使うバッファ: 入力ポートの総数
static char streamBf [IN_PORT_NUM][STREAMBUF_SIZE_ALL];
// 環境構造体: 入力ポートの総数
static sceMidiEnv midiEnv [IN_PORT_NUM];

/* ----------------------------------------------------------------
 * modhsyn
 * ---------------------------------------------------------------- */
/* input port */
#define HS_PORT_0 0		// OUT_PORT_0 の出力を入力
#define HS_PORT_NUM 1		// 入力ポートの総数
#define HS_BANK_0 0		// HD0/BD0 の HS_PORT_0 でのバンク番号

/* CSL context */
static sceCslCtx synthCtx;

/* CSL buffer group */
static sceCslBuffGrp synthGrp [1];	// 常に要素数は 1

/* CSL buffer context */
static sceCslBuffCtx synthIn [HS_PORT_NUM * 2];	 // 入力ポートの総数 x 2
static sceHSynEnv    synthEnv [HS_PORT_NUM];     // 入力ポートの総数

static sceHSyn_VoiceStat vstat;	// modhsyn 状態モニタバッファ

/* ================================================================
 * Timer
 * ================================================================ */
#include "timer.c"

/* ================================================================ */
/*  The example of `modmidi' callback functions			    */
/* `modmidi' コールバック関数の例				    */
/* ================================================================ */

/* 全てのチャンネルメッセージに対して呼ばれる
 * ---------------------------------------------------------------- */
unsigned int
chMsgCB (unsigned int msg, unsigned int private_data)
{
    /* `msg' format:
       bit  0- 7: MIDI status
       bit  8-15: MIDI 1st data
       bit 16-23: MIDI 2nd data		*/
#if 0
    Kprintf ("Channel message: %02x %02x %02x\n",
	     (msg & 0x0000ff),
	     (msg & 0x007f00) >>  8,
	     (msg & 0x7f0000) >> 16);
#endif

    /* 返り値の例 */

    /* 渡ってきた ch message をこの関数内で処理し、modmidi 内での
     *  処理を無効にしたい場合は sceMidi_ChMsgNoData を返す。*/
    // return (sceMidi_ChMsgNoData);

    /* 必ず MIDI ch = 0 にして返す */
    // return (msg & 0x7f7ff0);

    /* 通常はそのまま返す (あるいはコールバック関数そのものを設定しない)。*/
    return (msg);
}

/* meta event に対して呼ばれる
 * ---------------------------------------------------------------- */
int
metaMsgCB (unsigned char metaNo, unsigned char *current_point, unsigned int len,
	   unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;

#if 0
    /* メタイベント受信 */
    printf ("META %02x\n", (int) metaNo);
#endif

    /* 返り値の例 */

    /* 0 を返すと modmidi 内で current_point の位置から長さ len の
     * データが読み飛ばされる
     * → それらのデータはこのコールバック関数内で処理されたものと
     *    して扱われる */
    // return 0;

    /* 通常は 0 以外の値を返し、modmidi 内で処理させる
     * (あるいはコールバックそのものを設定しない)。*/
    return (NEXT_ENABLE);
}

/* exclusive message に対して呼ばれる
 * ---------------------------------------------------------------- */
int
exMsgCB (unsigned char *current_point, unsigned int len, unsigned int private_data)
{
    /* ... */	/* exclusive message に応じて処理を行う */

    /* 返り値の例 */

    /* 0 を返すと modmidi 内で current_point の位置から長さ len の
     * データが読み飛ばされる */
    // return 0;

    /* 通常は 0 以外の値を返し、modmidi 内で処理させる
     * (あるいはコールバックそのものを設定しない)。*/
    return (NEXT_ENABLE);
}

/* 拡張 MIDI メッセージ: repeat に対して呼ばれる
 * ---------------------------------------------------------------- */
int
repeatCB (sceMidiLoopInfo *pLoopInfo, unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;

#if 0
    printf ("Repeat %s %d %d %x\n",
	    (pLoopInfo->type == sceMidiLoopInfoType_Midi) ? "Midi" : "Song",
	    (int) pLoopInfo->loopTimes,
	    (int) pLoopInfo->loopCount, pLoopInfo->loopId);
#endif

    /* 返り値の例 */

    /* 0 を返すとリピートが行われない */
    // return 0;

    /* 通常は 0 以外の値を返し、リピートを続行させる
     * (あるいはコールバックそのものを設定しない)。*/
    return (NEXT_ENABLE);
}

/* ================================================================ */
/*  Set up each Mudule Context					    */
/* 各 Mudule Context の設定					    */
/* ================================================================ */

/*
================================================================
CSL context / buffer group / buffer context / stream buffer
================================================================

                                                  IOP memory
 +- midiCtx -----------+                           |            |
 |       [CSL context] |                  0x101000 |------------| <=+
 |       conf: NULL    |                           | SQ0        |   |
 |   callBack: NULL    |                           |            |   |
 |     extmod: NULL    |                           |            |   |
 | buffGrpNum: 2       |                           |------------|   |
 |                +- midiGrp ------------------+   |            |   |
 |    buffGrp: ==>|         [CSL buffer group] |                    |
 +----------------| 0 / input                  |                    |
                  |  buffNum: IN_PORT_NUM x 2  |                    |
                  |                   +- midiIn -------------+      |
                  |  buffCtx: =======>| [CSL buffer context] |      |
                  |                   | 0 / data             |      |
                  |                   |  sema: 0             |      |
                  |                   |  buff: =====================+
                  |                   +- - - - - - - - - - - +
                  |                   | 1 / environment      |
                  |                   |  sema: 0       +- midiEnv -----+
                  |                   |  buff: =======>| [environment] |
                  |                   +----------------|               |
                  +- - - - - - - - - - - - - - +       +---------------+
                  | 1 / output                 |
                  |  buffNum: OUT_PORT_NUM = 1 |
                  |                   +- midiOut ------------+
                  |  buffCtx: =======>| [CSL buffer context] |
                  +-------------------| 0 / data             |
                                      |  sema: 0             |
                                      |  buff: ===========================+
                                      +----------------------+            |
                                                                          |
                                              +- streamBf ------------+   |
                                              | [midi-stream buffer]  | <=+
                                              | buffsize:             | <=+
                                              |    STREAMBUF_SIZE_ALL |   |
 +- synthCtx ----------+                      | validsize: 0          |   |
 |       [CSL context] |                      |- - - - - - - - - - - -+   |
 |       conf: NULL    |                      | data:                 |   |
 |   callBack: NULL    |                      |                       |   |
 |     extmod: NULL    |                      +-----------------------+   |
 | buffGrpNum: 1       |                                                  |
 |                +- synthGrp -----------------+                          |
 |    buffGrp: ==>|         [CSL buffer group] |                          |
 +----------------| 0 / input                  |                          |
                  |  buffNum: HS_PORT_NUM * 2  |                          |
                  |                   +- synthIn ------------+            |
                  |  buffCtx: =======>| [CSL buffer context] |            |
                  |                   | 0 / data             |            |
                  |                   |  sema: 0             |            |
                  |                   |  buff: ===========================+
                  |                   +- - - - - - - - - - - +
                  |                   | 1 / environment      |
                  |                   |  sema: 0       +- synthEnv ----+
                  |                   |  buff: =======>| [environment] |<=+
                  |                   +----------------|               |  |
                  +----------------------------+       +---------------+  |
                                         IOP memory                       |
                                         |           |                    |
                                0x120000 |-----------|          bank #: 0 |
                                         |HD0        | ===================+
                                         |           |                    |
                                         |      SPU2 local memory         |
                                         |------|           |             |
                                         |      |-----------|0x5010       |
                                                |BD0        | ============+
                                                |           |
                                                |           |
                                                |-----------|
                                                |           |
*/

void
set_module_context (void)
{
    /* modmidi
     * ---------------------------------------------------------------- */

    // CSL context: modmidi は常に 2 つの buffer group
    midiCtx.extmod     = NULL;
    midiCtx.callBack   = NULL;
    midiCtx.buffGrpNum = 2;				// modmidi は常に 2
    midiCtx.buffGrp    = midiGrp;

    /* ... input ... */

    // CSL buffer group: 入力ポート数は IN_PORT_NUM
    midiGrp [IN_BUF].buffNum = IN_PORT_NUM * 2;		// 入力ポート数 x 2
    midiGrp [IN_BUF].buffCtx = midiIn;

    // CSL buffer context:
    //   入力ポート IN_PORT_0:  SQ データのアドレスは後で設定 → NULL に
    midiIn [IN_PORT_0 * 2 + DATA_BUF].sema = 0;
    midiIn [IN_PORT_0 * 2 + DATA_BUF].buff = NULL;	// set_midi() にて設定
    midiIn [IN_PORT_0 * 2 +  ENV_BUF].sema = 0;
    midiIn [IN_PORT_0 * 2 +  ENV_BUF].buff = &(midiEnv [IN_PORT_0]);

    /* ... output ... */

    // CSL buffer group: 出力ポート数は OUT_PORT_NUM
    midiGrp [OUT_BUF].buffNum = OUT_PORT_NUM;		// 出力ポート数
    midiGrp [OUT_BUF].buffCtx = midiOut;

    // CSL buffer context:
    //   出力ポート OUT_PORT_0
    //   ... 入力ポート IN_PORT_0 の全ての MIDI ch はここに出力
    midiOut [OUT_PORT_0].sema = 0;
    midiOut [OUT_PORT_0].buff = &(streamBf [OUT_PORT_0]); // modhsyn と共有

    /* modhsyn
     * ---------------------------------------------------------------- */

    // CSL context: modhsyn は常に 1
    synthCtx.extmod   	= NULL;
    synthCtx.callBack 	= NULL;
    synthCtx.buffGrpNum = 1;				// modhsyn は常に 1
    synthCtx.buffGrp    = synthGrp;

    /* ... input ... */

    // CSL buffer group: 入力ポート数は HS_PORT_NUM
    synthGrp [IN_BUF].buffNum = HS_PORT_NUM * 2;	// 入力ポート数 x 2
    synthGrp [IN_BUF].buffCtx = synthIn;

    // CSL buffer context:
    //   入力ポート HS_PORT_0 は、streamBf[OUT_PORT_0] を modmidi と共有
    synthIn [HS_PORT_0 * 2 + DATA_BUF].sema = 0;
    synthIn [HS_PORT_0 * 2 + DATA_BUF].buff = &(streamBf [OUT_PORT_0]); // modmidiと共有
    synthIn [HS_PORT_0 * 2 +  ENV_BUF].sema = 0;
    synthIn [HS_PORT_0 * 2 +  ENV_BUF].buff = &(synthEnv [HS_PORT_0]);

    /* midi-stream buffer
     * ---------------------------------------------------------------- */

    // サイズは STREAMBUF_SIZE_ALL
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_0]))->validsize = 0;

    return;
}

/* ----------------------------------------------------------------
 * Hardware Synthesizer セットアップ
 * ---------------------------------------------------------------- */
int
set_hsyn (sceHSyn_VoiceStat* pVstat)
{
    /* modhsyn 全体の初期化
     * ---------------------------------------------------------------- */
    if (sceHSyn_Init (&synthCtx, ONE_240TH) != sceHSynNoError) { // 1/240sec
	printf ("sceHSyn_Init Error\n");
	return (-1);
    }

    /* 入力ポート HS_PORT_0 と HD0/BD0 の設定
     * ---------------------------------------------------------------- */

    // modhsyn 入力ポート HS_PORT_0 の設定
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

    /* modmidi 全体の初期化
     * ---------------------------------------------------------------- */
    if (sceMidi_Init (&midiCtx, ONE_240TH) != sceMidiNoError) { // 1/240sec
	printf ("sceMidi_Init Error\n");
	return (-1);
    }

    /* modmidi 入力ポート IN_PORT_0 の設定
     * ---------------------------------------------------------------- */

    // SQ0 を入力ポート IN_PORT_0 に登録
    midiIn  [IN_PORT_0 * 2 + DATA_BUF].buff = (void *) SQ0_ADDRESS;

    if (sceMidi_Load (&midiCtx, IN_PORT_0) != sceHSynNoError) {
	printf ("sceMidi_Load (%d) error\n", IN_PORT_0);
	return (-1);
    }

    // コールバック関数に渡るプライベートデータは全てのバッファに設定
    midiEnv [IN_PORT_0].chMsgCallBack              = chMsgCB;
    midiEnv [IN_PORT_0].chMsgCallBackPrivateData   = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].metaMsgCallBack            = metaMsgCB;
    midiEnv [IN_PORT_0].metaMsgCallBackPrivateData = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].repeatCallBack             = repeatCB;
    midiEnv [IN_PORT_0].repeatCallBackPrivateData  = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].excMsgCallBack             = exMsgCB;
    midiEnv [IN_PORT_0].excMsgCallBackPrivateData  = (unsigned int) &(streamBf [IN_PORT_0]);

    // エクスクルーシブデータは出力ポート OUT_PORT_0 に出力
    midiEnv [IN_PORT_0].excOutPort = 1 << OUT_PORT_0;

    // 入力ポート IN_PORT_0 の MIDI データは、
    // MIDI ch に関わらず出力ポート OUT_PORT_0 に出力
    for (i = 0; i < sceMidiNumMidiCh; i++) { // i: MIDI ch
	midiEnv [IN_PORT_0].outPort [i] =  1 << OUT_PORT_0;
    }

    // 入力ポート 0 の演奏対象を設定
#if PLAY_CHUNK == SONG_CHUNK
    // 登録された SQ (SQ0) の SONG table 0 番を指定
    if (sceMidi_SelectSong (&midiCtx, IN_PORT_0, 0) != sceMidiNoError) {
	printf ("sceMidi_SelectSong Error\n");
	return (-1);
    }
#elif PLAY_CHUNK == MIDI_CHUNK
    // 登録された SQ (SQ0) の MIDI Data Block 0 番を指定
    if (sceMidi_SelectMidi (&midiCtx, IN_PORT_0, 0) != sceMidiNoError) {
	printf ("sceMidi_SelectMidi Error\n");
	return (-1);
    }
#endif

    // 入力ポート 0 の演奏開始位置を先頭に
#if PLAY_CHUNK == SONG_CHUNK
    if (sceMidi_SongSetLocation (&midiCtx, IN_PORT_0, 0, sceMidi_SSL_WithPreCommand)
	!= sceMidiNoError) {
	printf ("sceMidi_SongSetLocation Error\n");
	return (-1);
    }
#elif PLAY_CHUNK == MIDI_CHUNK
    if (sceMidi_MidiSetLocation (&midiCtx, IN_PORT_0, 0) != sceMidiNoError) {
	printf ("sceMidi_MidiSetLocation Error\n");
	return (-1);
    }
#endif

    /* 演奏
     * ---------------------------------------------------------------- */

    // 入力ポート IN_PORT_0 のデータを演奏
#if PLAY_CHUNK == SONG_CHUNK
    if (sceMidi_SongPlaySwitch (&midiCtx, IN_PORT_0, sceMidi_SongPlayStart)
	!= sceMidiNoError) {
	printf ("sceMidi_SongPlaySwitch Error\n");
	return (-1);
    }
#elif PLAY_CHUNK == MIDI_CHUNK
    if (sceMidi_MidiPlaySwitch (&midiCtx, IN_PORT_0, sceMidi_MidiPlayStart)
	!= sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n");
	return (-1);
    }
#endif

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

#if PLAY_CHUNK == SONG_CHUNK
	if (! ((sceMidi_GetEnv (&midiCtx, IN_PORT_0)->status) & sceMidiSongStat_inPlay)) {
#elif PLAY_CHUNK == MIDI_CHUNK
	if (! (sceMidi_isInPlay (&midiCtx, IN_PORT_0))) {
#endif
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
