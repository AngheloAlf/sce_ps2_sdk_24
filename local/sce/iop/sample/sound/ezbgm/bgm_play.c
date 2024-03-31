/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                  I/O Proseccor sample program
 *                          Version 1.20
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                      ezbgm.irx - bgm_play.c
 *                          API materials
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   1.20      Nov.23.1999     morita    first checked in.
 */

#include <stdio.h>
#include <sys/file.h>
#include <kernel.h>
#include <string.h>
#include <libsd.h>

#include "bgm_i.h"

// wav format ------------------------
#define RIFF_HEADER_SIZE 44
typedef struct {
    unsigned char     chunkID [4];
    unsigned int      chunkSize;
    unsigned short*   data;
} DATAHeader;

typedef struct {
    unsigned char     chunkID [4];
    unsigned int      chunkSize;
    unsigned short    waveFmtType;
    unsigned short    channel;
    unsigned int      samplesPerSec;
    unsigned int      bytesPerSec;
    unsigned short    blockSize;
    unsigned short    bitsPerSample;
} FMTHeader;

typedef struct {
    unsigned char     chunkID [4];
    unsigned int      chunkSize;
    unsigned char     formType [4];
    FMTHeader         fmtHdr;
    DATAHeader        dataHdr;
} RIFFHeader;
//------------------------------------

// 処理時の情報
typedef struct {
    unsigned int     size;
    unsigned int     offset;
    unsigned int     pos;
} BGM_WAVE;

BGM_WAVE gWave [2];
RIFFHeader gWavHdr;

int gThid = 0;			/* 演奏処理スレッド ID */
int gSem = 0;			/* バッファの発音終了待ち用セマフォ */
int gFd [2];			/* ファイル記述子 */
int gBuffRaw [2];		/* wav ファイルの読み込みバッファ位置 */
int gBuffSpu [2] = {0, 0};	/* SPU2 形式データの読み込みバッファ位置 */
int gRPacketSize [2];		/* パケットサイズ (wav) */
int gSPacketSize [2];		/* パケットサイズ (spu2) */
int gAllockedSize [2];		/* EE から指示された領域サイズ */
int gBgmVolume [2];		/* ボリューム */
int gBgmVolumeSet [2] = { 0, 0 }; /* ボリューム設定イベント: 1 なら指定あり */
volatile int gBgmIntr [2] = { 0, 0 }; /* チャンネル割り込みフラグ: 1 なら割り込みあり */
int gBgmPause [2] = { 0, 0 };	/* 演奏停止フラグ: 1 なら停止中 */
int gBgmMode [2] = { BGM_MODE_REPEAT_OFF, BGM_MODE_REPEAT_OFF }; /* 演奏状態 */

int  _BgmPlay (int status);
void _BgmRaw2Spu (u_long *src, u_long *dst,  u_long block_count);
void _BgmRaw2SpuMono (u_long *src, u_long *dst,  u_long block_count);

/* 転送終了割り込みハンドラ */
static int
IntFunc (int ch, void *common)
{
    gBgmIntr [ch] = 1;
    iSignalSema (* (int *) common);

    return 1;  //--割り込みを再度許可するのに必要
}

/* スレッド作成 */
static int
makeMyThread (void)
{
    struct ThreadParam param;
    int	thid;

    param.attr         = TH_C;
    param.entry        = _BgmPlay;
    param.initPriority = BASE_priority-3;
    param.stackSize    = 0x800;
    param.option = 0;

    /* スレッド作成 */
    thid = CreateThread(&param);

    return thid;
}

/* セマフォ作成 */
static int
makeMySem (void)
{
    struct SemaParam sem;

    sem.initCount = 0;
    sem.maxCount = 1;
    sem.attr = AT_THFIFO;

    /* セマフォ作成 */
    return CreateSema(&sem);
}

/* SPU2 が理解する形式にデータの並び・構造を変更 */
int
BgmRaw2Spu (int ch, int which, int mode)
{
    if ((mode & BGM_MODE_MONO) != 0){
	_BgmRaw2SpuMono ((u_long *)(gBuffRaw [ch] + (gRPacketSize [ch]) * which),
			 (u_long *)(gBuffSpu [ch] + (gSPacketSize [ch]) * which),
			 gSPacketSize [ch] / 1024);
    } else {
	_BgmRaw2Spu ((u_long *)(gBuffRaw [ch] + (gRPacketSize [ch]) * which),
		     (u_long *)(gBuffSpu [ch] + (gSPacketSize [ch]) * which),
		     gSPacketSize [ch] / 1024);
    }

    return 0;
}

/* ボリュームを即座に変更 */
void
BgmSetVolumeDirect (int ch, unsigned int vol)
{
    sceSdSetParam (ch | SD_P_BVOLL, vol >> 16);
    sceSdSetParam (ch | SD_P_BVOLR, vol & 0xffff);

    return;
}

/* マスターボリュームの変更 */
void
BgmSetMasterVolume (int ch, unsigned int vol)
{
    sceSdSetParam (ch | SD_P_MVOLL, vol >> 16);
    sceSdSetParam (ch | SD_P_MVOLR, vol & 0xffff);

    return;
}

/* 低レベルサウンドライブラリの初期化 */
void
BgmSdInit (int ch, int status)
{
    sceSdInit (0);

    return;
}

/* 初期化処理 ... 演奏処理スレッドの開始 */
int
BgmInit (int ch, int allocsize)
{
    int oldstat;

    if (gSem == 0) {
	gSem = makeMySem ();
    }
    if (gThid == 0){
	gThid = makeMyThread ();
	printf ("EzBGM: create thread ID= %d, ", gThid);
	/* スレッドを起動 */
	StartThread (gThid, (u_long)NULL);
    }

    CpuSuspendIntr (&oldstat);
    gBuffSpu [ch] = (int)AllocSysMemory (0, allocsize, NULL);
    CpuResumeIntr (oldstat);
    gAllockedSize [ch] = allocsize;
    printf (" alloc memory 0x%x - 0x%x\n",
	    gBuffSpu [ch], gBuffSpu [ch] + allocsize);

    return  gThid;
}

/* 終了処理 */
void
BgmQuit (int ch, int status)
{
    int oldstat;

    CpuSuspendIntr (&oldstat);
    FreeSysMemory ((void *)gBuffSpu [ch]);
    CpuResumeIntr (oldstat);
    gBuffSpu [ch] = 0;

    //-- もう一方のチャンネルも使われていないなら、リソースを開放する
    if (gBuffSpu [1-ch] == 0){
	if (gThid != 0) TerminateThread (gThid);
	if (gSem  != 0) DeleteSema (gSem);
	if (gThid != 0) DeleteThread (gThid);
	gThid = 0;
	gSem  = 0;
    }

    return;
}

/* 波形データファイルをオープン */
int
BgmOpen (int ch, char *filename)
{
    unsigned int channel = 0;

    PRINTF (("filename %s\n", filename));

    if ((gFd [ch] = open (filename, O_RDONLY)) < 0) {
	ERROR (("file open failed. %s \n", filename));
	return -1;
    }
    PRINTF (("fd %d, ch %d\n", gFd [0], ch));

    if (read (gFd [ch], (unsigned char*)(&gWavHdr), RIFF_HEADER_SIZE)
	!= RIFF_HEADER_SIZE) {
	ERROR(("file read failed. %s \n", filename));
	return -1;
    }
    gWave [ch].size   = gWavHdr.dataHdr.chunkSize;
    gWave [ch].offset = ((unsigned int)&(gWavHdr.dataHdr.data) -
			 (unsigned int)&gWavHdr);
    gWave [ch].pos    = 0;

    if (gWavHdr.fmtHdr.channel == 2)
	channel = WAV_STEREO_BIT;
    printf("channel %d\n", gWavHdr.fmtHdr.channel);

    //--- データの先頭までシーク
    lseek (gFd [ch], gWave [ch].offset, SEEK_SET);

    printf("wave size %d  offset %d\n", gWave [ch].size, gWave [ch].offset);

    sceSdSetTransIntrHandler (ch, (sceSdTransIntrHandler) IntFunc, (void *) &gSem);

    return (gWave [ch].size | channel);
}

/* 波形データファイルをクローズ */
void
BgmClose (int ch, int status)
{
    close (gFd [ch]);

    return;
}

/* 波形データをバッファ分先読み */
int
BgmPreLoad (int ch, int status)
{
    if (read (gFd [ch], (unsigned char*)(gBuffRaw [ch]), gRPacketSize [ch] * 2)
	!= gRPacketSize [ch] * 2) {
	ERROR (("BgmPreLoad: read failed \n"));
	return -1;
    }
    BgmRaw2Spu (ch, 0, gBgmMode [ch]);
    BgmRaw2Spu (ch, 1, gBgmMode [ch]);

    if (read (gFd [ch], (unsigned char*)(gBuffRaw [ch]), (gRPacketSize [ch] * 2))
	!= gRPacketSize [ch]*2) {
	ERROR (("BgmPreLoad: read failed \n"));
	return -1;
    }

    gWave [ch].pos += gRPacketSize [ch] * 4;

    return 0;
}

/* 演奏開始 */
void
BgmStart (int ch, int status)
{
    sceSdBlockTrans (ch, SD_TRANS_MODE_WRITE | SD_BLOCK_LOOP,
		     (u_char*)gBuffSpu [ch], (gSPacketSize [ch] * 2));

    BgmSetVolumeDirect (ch, gBgmVolume [ch]);
    // gBgmVolumeSet [ch] = 1;

    gBgmMode [ch] &= BGM_MASK_STATUS;
    gBgmMode [ch] |= BGM_MODE_RUNNING;

    return;
}

/* 演奏停止 */
void
_BgmStop (int ch, int status)
{
    sceSdBlockTrans (ch, SD_TRANS_MODE_STOP, NULL, 0);
    gBgmMode [ch] &= BGM_MASK_STATUS; // IDLE mode にする

    return;
}

/* 演奏停止 ... 実際には _BgmStop() にて処理が行われる */
void
BgmStop (int ch, unsigned int vol)
{
    gBgmPause [ch] = 1;
    gBgmMode [ch] &= BGM_MASK_STATUS;
    gBgmMode [ch] |= BGM_MODE_PAUSE;

    return;
}

/* ボリューム設定 */
void
BgmSetVolume (int ch, unsigned int vol)
{
    gBgmVolumeSet [ch] = 1;
    gBgmVolume [ch] = vol;

    return;
}

/* ステレオ/モノラルとその環境の設定 */
void
BgmSetMode (int ch, u_int mode)
{
    gBgmMode [ch] &= BGM_MASK_REPEAT;
    gBgmMode [ch] &= BGM_MASK_STEREO;
    gBgmMode [ch] |= mode;

    if ((mode&BGM_MODE_MONO) != 0){
	gSPacketSize [ch] = gAllockedSize [ch] / 3;
	gRPacketSize [ch] = gSPacketSize [ch] / 2;
    } else {
	gSPacketSize [ch] = gAllockedSize [ch] / 4;
	gRPacketSize [ch] = gSPacketSize [ch];
    }
    gBuffRaw [ch] = (int)(gBuffSpu [ch] + (gSPacketSize [ch] * 2));
}

/* ステレオ/モノラルの現在値の取得 */
unsigned int
BgmGetMode (int ch, int status)
{
    return gBgmMode [ch];
}

/* ファイルの読み出し位置の移動 */
void
BgmSeek (int ch, unsigned int value)
{
    lseek (gFd [ch], gWave [ch].offset+value, SEEK_SET);
    gWave [ch].pos = value;

    return;
}

/* 演奏処理スレッド */
int
_BgmPlay (int status)
{
    int i, ch, read_size, which;
    int *addr, remain;
    int terminate [2] = {0, 0};
    int continue_max [2] = {2, 2};

    while (1) {
	//-- バッファの発音終了待ち
	WaitSema(gSem);

	//-- どちらのチャンネルからの割り込みか
	if      ((gBgmIntr [0] == 1) && (ch != 0))  ch = 0;
	else if ((gBgmIntr [1] == 1) && (ch != 1))  ch = 1;
	else if (gBgmIntr [0] == 1)  ch = 0;
	else if (gBgmIntr [1] == 1)  ch = 1;
	else continue;

	gBgmIntr [ch] = 0;

	which = 1 - (sceSdBlockTransStatus (ch, 0) >> 24);

	//--- ボリューム変更イベント
	if (gBgmVolumeSet [ch] == 1){
	    BgmSetVolumeDirect (ch, gBgmVolume [ch]);
	    gBgmVolumeSet [ch] = 0;
	}

	//--- バッファの変換
	BgmRaw2Spu (ch, which, gBgmMode [ch]);

	//--- データ終端による停止（ループ無し）
	if ((gBgmMode [ch] & BGM_MODE_TERMINATE) != 0){
	    if (terminate [ch] < continue_max [ch]) {
		terminate [ch] ++;
		continue;
	    } else {
		// WaitSema(gSem); //もう１回割り込みが来るまで待つ
		_BgmStop (ch, 0);
		BgmSetVolumeDirect (ch, 0x0);
		continue;
	    }
	}

	//--- バッファ用ファイルREAD
	remain = gWave [ch].size - gWave [ch].pos;
	if (remain > gRPacketSize [ch]){
	    //--- データ終端ではない
	    read_size = read (gFd [ch],
			      (unsigned char *)(gBuffRaw [ch] + gRPacketSize [ch] * which),
			      gRPacketSize [ch]);

	    if (read_size < gRPacketSize [ch])
		continue; //retry
	    gWave [ch].pos += read_size;
	} else{
	    //--- データ終端
	    read_size = read (gFd [ch],
			      (unsigned char *)(gBuffRaw [ch] + gRPacketSize [ch] * which),
			      remain);
	    if (read_size < remain)
		continue; //retry
	    PRINTF (("end of file - ch %d\n", ch));

	    //--- ループする場合
	    if ((gBgmMode [ch] & ~BGM_MASK_REPEAT) != BGM_MODE_REPEAT_OFF){
		lseek (gFd [ch], gWave [ch].offset, SEEK_SET);
		read (gFd [ch], (unsigned char *)(gBuffRaw [ch] + gRPacketSize [ch] * which + remain),
		      gRPacketSize [ch] - remain);
		gWave [ch].pos = gRPacketSize [ch]-remain;  //先頭に帰る場合
	    }
	    //--- ループしない場合
	    else{
		addr = (int *)(gBuffRaw [ch] + gRPacketSize [ch] * which + remain);
		for (i = 0; i < ((gRPacketSize [ch] - remain) >> 2); i++){
		    *(addr++) = 0;
		}
		gBgmMode [ch] &= BGM_MASK_STATUS;
		gBgmMode [ch] |= BGM_MODE_TERMINATE;
	    }
	}

	//-- 停止イベント
	if ((gBgmPause [ch] == 1)) {
	    _BgmStop (ch, 0);
	    BgmSetVolumeDirect (ch, 0x0);
	    gBgmPause [ch] = 0;
	}
    }

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
