/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *              I/O Processor Library Sample Program
 * 
 *                          - voice -
 * 
 *                           Shift-JIS
 * 
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                            main.c
 * 
 *     Version   Date          Design     Log
 *  --------------------------------------------------------------------
 *     1.50      Mar.28.2000   kaol       For new interrupt callbacks
 *     1.20      Nov.29.1999   morita     load on-the-fly
 *     0.60      Oct.14.1999   morita     first checked in.
 */

#include <kernel.h>
#include <stdio.h>
#include <libsd.h>
#include <sys/file.h>

/* コンパイル時オプション */
#define BATCH_MODE      0          // バッチ処理を行う
#define DMA_CB_TEST	1          // DMA 割り込みのテスト
#define IRQ_CB_TEST	1          // IRQ 割り込みのテスト
#define IRQ_ADDR_OFST	0x1000     // IRQの位置 (波形先頭からの offset)

#define TMP_OUT		(u_int *)0x110000  //バッチの結果出力

/* IOP メモリ → SPU2 ローカルメモリに転送する時の DMA チャンネル */
#define DMA_CH 0

#define PRINTF(x) printf x
#define BASE_priority  60

#define WAV_DATA_SIZE	20944
#define VAG_FILENAME	"host1:/usr/local/sce/data/sound/wave/piano.vag"
/* VAG ファイルのヘッダ部分のサイズ。SPU2 ローカルメモリ内では不要 */
#define VAG_HEADER_SIZE 48
/* 転送先の SPU2 ローカルメモリ内の先頭アドレス */
#define VAG_ADDR	0x15010
#define	REVERB_DEPTH 	0x3fff

#include "init_bat.h"  // libsd バッチコマンド

char gDoremi [8] = {36, 38, 40, 41, 43, 45, 47, 48}; 
int gEndFlag = 0;

/* ----------------------------------------------------------------
 * 割り込みハンドラ
 * ---------------------------------------------------------------- */
int
IntTrans (int ch, void * common)
{
    int *c = (int *) common;

    (*c) ++;
    Kprintf("##### interrupt detected. count: %d CORE ch: %d #####\n", *c, ch);

    return 1;
}

int
IntFunc (int core, void *common)
{
    int *c = (int *) common;
    (*c) ++;
    Kprintf ("///// interrupt detected (%d). CORE-bit: %d /////\n", *c, core);
    return 1;
}

/* ----------------------------------------------------------------
 * データ読み込み
 * ---------------------------------------------------------------- */
char *
set_data (int *size)
{
    int fd;
    char *buffer;
    int oldstat;

    if ((fd = open (VAG_FILENAME, O_RDONLY)) < 0) {
	printf ("\nfile open failed. %s\n", VAG_FILENAME);
	return NULL;
    };
    *size = lseek (fd, 0, 2);	/* ファイルサイズの取得 */
    if (*size <= 0) {
	printf ("\nCan't load VAG file to iop heap\n");
	return NULL;
    }
    lseek (fd, 0, 0);		/* 読み込み位置を先頭に */

    PRINTF (("allocate IOP heap memory - "));
    CpuSuspendIntr (&oldstat);
    buffer = AllocSysMemory (0, *size, NULL); /* 領域確保 */
    CpuResumeIntr (oldstat);
    if (buffer == NULL) {
	printf ("\nCan't alloc heap \n");
	return NULL;
    }
    PRINTF (("alloced 0x%x  \n", (int)buffer));

    read (fd, buffer, *size);	/* ファイルの内容を読み込み */
    close (fd);

    return buffer;
}

/* ----------------------------------------------------------------
 * メイン処理
 * ---------------------------------------------------------------- */
int
sound_test (void)
{
    int core, v;
    int i, size;
    char *wavBuffer;
    sceSdEffectAttr r_attr;
#if BATCH_MODE
    int ret;
#endif

    PRINTF(("voice start...\n"));

    /* ファイルの内容を IOP メモリ内に読み込み */
    if ((wavBuffer = set_data (&size)) == NULL){
	return -1;
    }

    /* 低レベルサウンドライブラリの初期化 */
    sceSdInit (0);

#if BATCH_MODE
    /* バッチ処理: 4 つのコマンドを実行 */
    i = sceSdProcBatch (gBatchCommonInit, NULL, 4);
    PRINTF (("sceSdProcBatch count = %d \n", i));

    /* バッチ処理: 7 つのコマンドを実行: 返り値を領域 TMP_OUT に出力 */
    sceSdProcBatch (gBatchCommonInit2, TMP_OUT, 7);
    printf(" check wite_iop: %x\n", *((short *)0x90000));
#else
    /* マスターボリュームを設定 */
    for (i = 0; i < 2; i++) {
	sceSdSetParam (i | SD_P_MVOLL, 0x3fff);
	sceSdSetParam (i | SD_P_MVOLR, 0x3fff);
    }
#endif

    /*
     * データ転送
     */
    PRINTF (("Data transfer ...\n"));
#if DMA_CB_TEST
    /* DMA 転送終了割り込みハンドラを設定 */
    sceSdSetTransIntrHandler (DMA_CH, (sceSdSpu2IntrHandler) IntTrans, (void *) &gEndFlag);
#endif
    /* 転送開始 */
    sceSdVoiceTrans (DMA_CH,
		     SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
		     (u_char *)(wavBuffer + VAG_HEADER_SIZE),
		     VAG_ADDR, (u_int)WAV_DATA_SIZE);
    PRINTF (("TSA = %x \n", sceSdGetAddr (DMA_CH | SD_A_TSA)));

#if DMA_CB_TEST
    while (gEndFlag == 0) {   /* 割り込みハンドラが呼ばれるまで待つ */
	DelayThread (100 * 1000); /* インターバル: 0.1 秒 */
    }
#else
    sceSdVoiceTransStatus (DMA_CH, SD_TRANS_STATUS_WAIT); /* 転送終了を待つ */
#endif

#if IRQ_CB_TEST
    /*
     * SPU2 割り込みの設定
     */
    /* SPU2 割り込みが起こるアドレスを設定
     * ... SPU2 ローカルメモリ内の、波形データの先頭から 0x1000 の位置 */
    sceSdSetAddr (SD_CORE_0 | SD_A_IRQA, VAG_ADDR + IRQ_ADDR_OFST);
    gEndFlag = 0;
    /* SPU2 割り込みハンドラの登録 */
    sceSdSetSpu2IntrHandler ((sceSdSpu2IntrHandler) IntFunc, (void *) &gEndFlag);
    /* SPU2 割り込みを有効に (CORE0 側) */
    sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 1);
#endif

    for (core = 0; core < 2; core ++) {
#if BATCH_MODE
	/* バッチ処理: 指定ボイスを一括処理
	 * 第4引数/0xffffff ... 全てのビットが 1 = ボイス 0 ～ 23 を全て設定 */
	ret = sceSdProcBatchEx (gBatchVoiceInit  [core], NULL,    6, 0xffffff);
	ret = sceSdProcBatchEx (gBatchVoiceInit2 [core], TMP_OUT, 6, 0xffffff);
	PRINTF (("sceSdProcBatchEx count = %d \n", ret));
#else
	for (v = 0; v < 24; v ++) {
	    /* ボイスの属性をそれぞれ設定 */
	    sceSdSetParam (core | (v << 1) | SD_VP_VOLL, 0x1eff);
	    sceSdSetParam (core | (v << 1) | SD_VP_VOLR, 0x1eff);
	    sceSdSetParam (core | (v << 1) | SD_VP_PITCH, 0x400);
	    sceSdSetAddr  (core | (v << 1) | SD_VA_SSA, VAG_ADDR);
	    sceSdSetParam (core | (v << 1) | SD_VP_ADSR1, 
			   SD_ADSR1 (SD_ADSR_A_EXP, 30, 14, 14));
	    sceSdSetParam (core | (v << 1) | SD_VP_ADSR2,
			   SD_ADSR2 (SD_ADSR_S_EXP_DEC, 52, SD_ADSR_R_EXP, 13));
	}
#endif
    }

    /*
     * エフェクト属性の設定
     */
    r_attr.depth_L  = 0;	/* 最初は 0 に */
    r_attr.depth_R  = 0;
    /* r_attr.delay    = 30; */
    /* r_attr.feedback = 200; */
    /* モード: ホール + ワークエリアを初期化 */
    r_attr.mode = SD_REV_MODE_HALL | SD_REV_MODE_CLEAR_WA;
    sceSdSetEffectAttr (SD_CORE_0, &r_attr);

    // effect on
    /* CORE0: エフェクト有効 */
    sceSdSetCoreAttr (SD_CORE_0 | SD_C_EFFECT_ENABLE, 1);
    /* CORE0: エフェクトセンドボリューム */
    sceSdSetParam    (SD_CORE_0 | SD_P_EVOLL, REVERB_DEPTH);
    sceSdSetParam    (SD_CORE_0 | SD_P_EVOLR, REVERB_DEPTH);
    /* CORE0: 全てのボイスの L/R 出力をエフェクトに接続  */
    sceSdSetSwitch   (SD_CORE_0 | SD_S_VMIXEL, 0xffffff);
    sceSdSetSwitch   (SD_CORE_0 | SD_S_VMIXER, 0xffffff);

    // Ring!
    for (v = 15; v < 23; v ++) {
#if IRQ_CB_TEST
	/* SPU2 割り込みが起きた場合、一旦無効にして再設定する */
	sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 0);
	sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 1);
#endif
	/* ピッチ設定 */
	sceSdSetParam (SD_CORE_0 | (v << 1) | SD_VP_PITCH, 
		       sceSdNote2Pitch (60, 0, gDoremi[v - 15], 0));
	/* 発音 */
	sceSdSetSwitch (SD_CORE_0 | SD_S_KON, 1 << v);

	PRINTF((" ++ key_on  pitchA %x, pitchB %x \n", 
		sceSdNote2Pitch (60, 0, gDoremi[v - 15], 0), 
		sceSdGetParam (SD_CORE_0 | (v << 1) | SD_VP_PITCH)));
	
	DelayThread (1000 * 1000); /* 1 秒待つ */

	/* 消音 */
	sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, 1 << v);
    }

    /* 念のため CORE0 全てのボイスを消音 */
    sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, 0xffffff);

    PRINTF(("voice completed...\n"));

    return 0;
}

/* ----------------------------------------------------------------
 * スタート関数
 * ---------------------------------------------------------------- */

int
start (int argc, char *argv [])
{
    struct ThreadParam param;
    int	thid;

    /* Create thread */
    param.attr         = TH_C;
    param.entry        = sound_test;
    param.initPriority = BASE_priority;
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread(&param);
    if (thid > 0) {
	StartThread(thid,0);	/* スレッドの起動 */
	return RESIDENT_END;	/* program is resident */
    } else {
	return NO_RESIDENT_END;	/* program is deleted */
    }
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
