/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *              Emotion Engine Library Sample Program
 * 
 *                         - CD/DVD -
 * 
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 * 
 */

#include <stdio.h>
#include <kernel.h>
#include <sys/file.h>
#include <string.h>
#include <libcdvd.h>
#include <string.h>


/* ================================================================
 *
 *      Program
 *
 * ================================================================ */


#define BASE_priority   (82)    /* サンプルプログラムのプライオリティ */

#define MEDIA_CD	/* 読み込みメディアは CD */

unsigned char  ring_buf[2048 * 80] __attribute__((aligned (16)));
			/* ストリーミングバッファ */
unsigned char  bf[2048] __attribute__((aligned (16)));
			/* 読み込みバッファ */

int my_main(int arg);

/* my_main()関数スレッドを作成するための関数 */
int start( int argc, char *argv[] )
{
        struct ThreadParam param;
        int     thid;

        /* --- initialize thread --- */
        param.attr         = TH_C;
        param.entry        = my_main;
        param.initPriority = BASE_priority;
        param.stackSize    = 0x800;
        param.option       = 0;
        thid = CreateThread(&param);
        if( thid > 0 ) {
                StartThread(thid,0);
                return 0;       /* 0: program is resident */
        } else {
                return 1;       /* 1: program is deleted */
        }
}

/* メイン処理関数 */
int my_main(int arg)
{
	int	rfd, wfd, cnt0, ret, 
	        start_st,file_sec, readsize;
 	u_int   err;
	sceCdlFILE fp;
        sceCdRMode mode;
        char *cw, *filename;

	printf("sample start.\n");

	printf(" sceCdInit\n");
	sceCdInit(SCECdINIT);
#ifdef MEDIA_CD
	sceCdMmode(SCECdCD);
#else
	sceCdMmode(SCECdDVD);
#endif
	printf(" sceCdStInit\n");
        sceCdStInit(80,5,(u_int)ring_buf);
	printf(" sceCdDiskReady\n");
	sceCdDiskReady(0);

        /* 標準入出力でファイルを読む */
        rfd = open("cdrom0:\\SYSTEM.CNF;1", 0);
        if ( rfd < 0 ){
                printf("Can't open SYSTEM.CNF\n");
                return(-1);
        }
        readsize = lseek(rfd, 0, SEEK_END);
        lseek(rfd, 0, SEEK_SET);
        read(rfd, (u_int *)bf, readsize);
        close(rfd);

        /* PlayStation実行のファイル名を取り出す。*/
        cw= (char *)bf;
        for(cnt0= 0; (*cw != '\n') && (cnt0 < readsize); cnt0++) cw++;
        if(cnt0 == readsize){
                 printf("Sorry: File name not Collect\n");
                 return(-1);
        }
        *cw= 0;
        cw= (char *)bf;
        for(cnt0= 0; (*cw != ':') && (cnt0 < readsize); cnt0++) cw++;
        if(cnt0 == readsize){
                printf("Sorry: File name not Collect\n");
                return(-1);
        }
        cw++; filename= cw;

        sceCdDiskReady(0);

        /* ライブラリ関数でファイルを読む */
	printf("Search Filename: %s\n",filename);
        /* ファイルの格納位置を検索する */
	ret= sceCdSearchFile(&fp, filename);
			
	printf("data= %x\n",fp.date[4]);
	if(!ret){
		printf("sceCdSearchFile fail :%d\n",ret);
                return(-1);
	}

	sceCdDiskReady(0);
	
        wfd = open("host1:data0.dat", O_WRONLY | O_TRUNC | O_CREAT );
        if (wfd < 0 ) {
                printf("Can't open file data.dat\n");
                return(-1);
        }

        /* ストリーム関数でファイルを読む */
        /* エラー発生時は２５５回リトライする。 */
        mode.trycount= 0;
        /* エラー発生時は回転速度を落してリード */
        mode.spindlctrl= SCECdSpinNom;
        /* データサイズは2048byte */
	/* ストリーマはデータサイズ２０４８byteのみサポート */
        mode.datapattern= SCECdSecS2048;
                       
	file_sec= fp.size / 2048; if(fp.size % 2048) file_sec++;
	start_st= fp.lsn;
	sceCdStStart(start_st , &mode);
	for(cnt0= 0; cnt0 < file_sec; cnt0++){
		printf("Stream READ LSN= %d sec= %d end_sec= %d\n",
					 start_st,cnt0,file_sec);
		sceCdStRead(1,(u_int *)bf, STMBLK, &err);
		if(err){
		     printf("Disk error code= 0x%08x\n", err);
		     break;
		}
                write(wfd, (u_int *)bf, 2048);
	}
	sceCdStStop();
        close(wfd);

        printf("sample end.\n");
        return(0);
}
/* end of file.*/
