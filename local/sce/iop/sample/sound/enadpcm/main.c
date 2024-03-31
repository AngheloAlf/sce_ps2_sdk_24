/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *              I/O Processor Library Sample Program
 * 
 *                          - enadpcm -
 * 
 *                           Shift-JIS
 * 
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                            main.c
 * 
 *     Version   Date          Design     Log
 *  --------------------------------------------------------------------
 */

#include <kernel.h>
#include <stdio.h>
#include <sys/file.h>
#include <libsd.h>
#include <spucodec.h>

/* 利用する SPU2 の DMA チャンネル */
#define DMA_CH 0

/* 処理スレッドのスレッド優先度 */
#define BASE_priority  60

/* 既にエンコードされた波形データの情報 */
#define VAG_DATA_SIZE	(229296 + 16) /* 64 バイト単位に切り上げ */
#define VAG_FILENAME	"host1:/usr/local/sce/data/sound/wave/knot.vag"
#define VAG_HEADER_SIZE 48
#define VAG_ADDR	0x5010

/* エンコード処理の元データとして使用する PCM 波形データの情報 */
#define RAW_DATA_SIZE 802444
#define RAW_FILENAME	"host1:/usr/local/sce/data/sound/wave/knot_l.raw"
#define RAW_ADDR	(VAG_ADDR + VAG_DATA_SIZE)

/* 発音に使用するボイスのボイス番号 */
#define VOICE_VAG 0		/* 既にエンコードされた波形データの発音用 */
#define VOICE_RAW 1		/* エンコード処理で出力された波形データの発音用 */

/* デバッグ用 */
#ifdef DEBUG
#define PRINTF(x) printf x
#else
#define PRINTF(x)
#endif

/* ---------------------------------------------------------------- 	*/
/*
 * set_data()		領域を確保し、その領域にファイルの内容を読み込む
 *	引数:
 *		fname:	読み込むファイルの名前
 *		size:	読み込むファイルのサイズ
 *	返り値:
 *		NULL      ... 異常終了
 *		NULL 以外 ... ファイルの内容が読み込まれた領域の
 *			      先頭アドレス
 */
/* ---------------------------------------------------------------- 	*/
char *
set_data (char *fname, int *size)
{
    int fd;
    char *buffer;
    int oldstat;

    printf ("    fname = [%s]\n", fname);

    /* ファイルを開く */
    if ((fd = open (fname, O_RDONLY)) < 0) {
	printf ("\nfile open failed. %s\n", fname);
	return NULL;
    };
    *size = lseek (fd, 0, 2);	/* 終端まで移動してファイルサイズを得る */
    if (*size <= 0) {
	printf ("\nCan't load file to iop heap\n");
	return NULL;
    }
    lseek (fd, 0, 0);

    /* ファイルの内容を読み込む領域を確保 */
    PRINTF (("allocate IOP heap memory - "));
    CpuSuspendIntr (&oldstat);
    buffer = AllocSysMemory (0, *size, NULL);
    CpuResumeIntr (oldstat);
    if (buffer == NULL) {
	printf ("\nCan't alloc heap \n");
	return NULL;
    }
    PRINTF (("alloced 0x%x  \n", (int)buffer));

    /* ファイルの内容を読み込む */
    read (fd, buffer, *size);

    /* ファイルを閉じる */
    close (fd);

    return buffer;
}

/* ---------------------------------------------------------------- 	*/
/*
 * rawdata_encode_and_play()	PCM データをエンコードして演奏
 *	引数:	なし
 *	返り値:
 *		0 ... 正常終了
 *	       -1 ... set_data() が異常終了した場合、あるいは
 *		      エンコード結果用の領域が確保できなかった場合
 */
/* ---------------------------------------------------------------- 	*/
int
rawdata_encode_and_play (void)
{
    char *addr, *adpcm;
    int size;
    int encode_size;
    int oldstat;
    sceSpuEncodeEnv env;

    printf ("  Playing the encoded RAW data ...\n");

    /* PCM 波形データの読み込み */
    if ((addr = set_data (RAW_FILENAME, &size)) == NULL){
	return -1;
    }

    /* PCM データのサイズの半分の領域を確保
     *  ... PCM データのエンコード結果は約 1/3 だが、多少大きめに */ 
    CpuSuspendIntr (&oldstat);
    adpcm = AllocSysMemory (0, size / 2, NULL);
    CpuResumeIntr (oldstat);
    if (adpcm == NULL) {
	printf ("\nCan't alloc heap \n");
	return (-1);
    }

    /* 波形データエンコード属性の設定 */
    env.src        = (short *) addr;
    env.dest       = (short *) adpcm;
    env.size       = RAW_DATA_SIZE;
    env.loop_start = 0;				    /* ループ開始ポイント */
    env.loop       = SPUCODEC_ENCODE_NO_LOOP;	    /* ループ属性 */
    env.byte_swap  = SPUCODEC_ENCODE_ENDIAN_LITTLE; /* エンディアン属性 */
    env.proceed    = SPUCODEC_ENCODE_WHOLE;	    /* 全域エンコード or 進捗指定 */
    env.quality    = SPUCODEC_ENCODE_MIDDLE_QULITY; /* 音質レベル */
    env.work       = (short *) 0; // NULL ... temporary reserved.

    /* エンコード */
    printf ("    encoding ...\n");
    encode_size = sceSpuCodecEncode (&env);
    printf ("    encoding ... finished: encoded data size = %d [0x%x]\n",
	    encode_size, encode_size);

    /* エンコード結果を SPU2 ローカルメモリに転送 */
    sceSdVoiceTrans (DMA_CH,
		     SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
		     (u_char *) adpcm,
		     RAW_ADDR, (u_int) encode_size);
    PRINTF (("    TSA = %x \n", sceSdGetAddr (DMA_CH | SD_A_TSA)));

    /* 転送終了を待つ */
    PRINTF (("    Wait for transferring ...\n"));
    sceSdVoiceTransStatus (DMA_CH, SD_TRANS_STATUS_WAIT);
    PRINTF (("    Transfer is done ...\n"));

    /* エンコード結果を実際に演奏 */
    sceSdSetAddr   (SD_CORE_0 | (VOICE_RAW << 1) | SD_VA_SSA, RAW_ADDR);
    printf ("    and playing ...\n");
    sceSdSetSwitch (SD_CORE_0 | SD_S_KON,  1 << VOICE_RAW); /* 発音 */

    /* 10 秒待つ ... 波形データが演奏されるまで */
    DelayThread (10 * 1000 * 1000); // 10 sec.

    /* 消音 */
    sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, 1 << VOICE_RAW);

    /* エンコード結果の領域を解放 */
    CpuSuspendIntr (&oldstat);
    FreeSysMemory (addr);
    CpuResumeIntr (oldstat);

    printf ("  Playing the encoded RAW data ... completed...\n");

    return 0;
}

/* ---------------------------------------------------------------- 	*/
/*
 * vag_play()	事前にエンコードされた波形データを演奏
 *	引数:	なし
 *	返り値:
 *		0 ... 正常終了
 *	       -1 ... set_data() が異常終了した場合
 */
/* ---------------------------------------------------------------- 	*/
int
vag_play (void)
{
    char *addr;
    int size;
    int oldstat;

    printf ("  Playing the normal VAG ...\n");

    /* 波形データの読み込み */
    if ((addr = set_data (VAG_FILENAME, &size)) == NULL){
	return -1;
    }

    PRINTF (("    Data transfer ...\n"));

    /* 波形データを SPU2 ローカルメモリに転送 */
    sceSdVoiceTrans (DMA_CH,
		     SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
		     (u_char *) addr,
		     VAG_ADDR, (u_int) VAG_DATA_SIZE);
    PRINTF (("    TSA = %x \n", sceSdGetAddr (DMA_CH | SD_A_TSA)));

    /* 転送終了を待つ */
    PRINTF (("    Wait for transferring ...\n"));
    sceSdVoiceTransStatus (DMA_CH, SD_TRANS_STATUS_WAIT);
    PRINTF (("    Transfer is done ...\n"));

    /* 波形データを演奏 */
    sceSdSetAddr  (SD_CORE_0 | (VOICE_VAG << 1) | SD_VA_SSA, VAG_ADDR);
    sceSdSetSwitch (SD_CORE_0 | SD_S_KON,  1 << VOICE_VAG); /* 発音 */

    /* 10 秒待つ ... 波形データが演奏されるまで */
    DelayThread (10 * 1000 * 1000); // 10 sec.

    /* 消音 */
    sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, 1 << VOICE_VAG);

    /* 波形データの領域を解放 */
    CpuSuspendIntr (&oldstat);
    FreeSysMemory (addr);
    CpuResumeIntr (oldstat);

    printf ("  Playing the normal VAG ... completed.\n");

    return 0;
}

/* ---------------------------------------------------------------- 	*/
/*
 * encode_test()	メイン処理
 *	引数:	なし
 *	返り値:	必ず 0
 */
/* ---------------------------------------------------------------- 	*/
int
encode_test (void)
{
    int core, v;
    int i;

    printf ("enadpcm start ================\n");

    /* 低レベルサウンドライブラリの初期化 */
    sceSdInit (0);

    PRINTF (("  Set all attributes ...\n"));

    /*
     * SPU2 を典型的な値で初期化
     */
    for (i = 0; i < 2; i++) {
	sceSdSetParam (i | SD_P_MVOLL, 0x3fff);
	sceSdSetParam (i | SD_P_MVOLR, 0x3fff);
    }
    for (core = 0; core < 2; core ++) {
	for (v = 0; v < 24; v ++) {
	    sceSdSetParam (core | (v << 1) | SD_VP_VOLL, 0x3fff);
	    sceSdSetParam (core | (v << 1) | SD_VP_VOLR, 0x3fff);
	    sceSdSetParam (core | (v << 1) | SD_VP_PITCH, 0x1000);
	    sceSdSetParam (core | (v << 1) | SD_VP_ADSR1, 
			   SD_ADSR1 (SD_ADSR_A_LINEAR, 0, 0, 0xf));
	    sceSdSetParam (core | (v << 1) | SD_VP_ADSR2,
			   SD_ADSR2 (SD_ADSR_S_EXP_INC, 0, SD_ADSR_R_EXP, 0));
	}
    }

    /* エフェクト処理は利用しない *//* effect disable */
    sceSdSetCoreAttr (SD_CORE_0 | SD_C_EFFECT_ENABLE, 0);

    /*
     *		---- メイン処理 ----
     */

    /* まず、事前にエンコードされた波形データを演奏 */
    vag_play ();

    /* 次に、同じ波形データの PCM データをエンコード処理して演奏 */
    rawdata_encode_and_play ();

    printf ("enadpcm completed ============\n");

    return 0;
}

/* ----------------------------------------------------------------
 * start
 * ---------------------------------------------------------------- */

int
start (int argc, char *argv [])
{
    struct ThreadParam param;
    int	thid;

    /* 処理スレッドの作成 *//* Create main thread */
    param.attr         = TH_C;
    param.entry        = encode_test;
    param.initPriority = BASE_priority;
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread(&param);
    if (thid > 0) {
	StartThread (thid, 0);
	/* ファイルの内容が全て読み込まれるまで待機 */
	DelayThread (25 * 1000 * 1000); /* for file service (dsifilesv) */
	return RESIDENT_END;	/* program is resident */
    } else {
	return NO_RESIDENT_END;	/* program is deleted */
    }
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
