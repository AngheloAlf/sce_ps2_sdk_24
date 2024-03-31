/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                 I/O Processor Library Sample Program
 * 
 *                          - CD/DVD -
 *                           Shift-JIS
 * 
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                            <main.c>
 *                     <main function of file read>
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.01            Oct,13,1999     kashiwabara  first version
 *      0.02            Dec,05,1999     kashiwabara  rel_13 version
 */


#include <stdio.h>
#include <kernel.h>
#include <sys/file.h>
#include <string.h>
#include <libcdvd.h>

#define BASE_priority	(80)	/* サンプルプログラムのプライオリティ */ 
#define SAMPLE_WAIT	(16 * 1000)	/* 16(msec) */

extern int sprintf(char *, const char *, ...);


unsigned char  bf[2048 * 256];	/* 読み込みバッファ */
unsigned char  toc[2048];	/* TOC読み込みバッファ */

#define MEDIA_CD		/* 読み込みメディアは CD */

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

/* メディア読みとり関数等の、終了コールバック関数 */ 
void test_callback( int cb_reason )
{
        switch(cb_reason){
            case SCECdFuncRead:
                Kprintf("SCECdFuncRead Ended\n"); break;
            case SCECdFuncSeek:
                Kprintf("SCECdFuncSeek Ended\n"); break;
            case SCECdFuncStandby:
                Kprintf("SCECdFuncStandby Ended\n"); break;
            case SCECdFuncStop:
                Kprintf("SCECdFuncStop Ended\n"); break;
            case SCECdFuncPause:
                Kprintf("SCECdFuncPause Ended\n"); break;
            default:
                Kprintf("Other Ended \n"); break;
        }
        Kprintf("error code= 0x%08x\n",sceCdGetError());
}

/* メイン処理関数 */
int my_main(int arg)
{
	int	 rfd, wfd, rsec, rs_flg, cnt0, lpcnt,
		 disk_type, last_track, readsize, trayflg, traycnt,*old_func;
	unsigned char *cp;
	char *cw, *filename,wfilename[32];
	sceCdlFILE fp;
	sceCdRMode mode;
	sceCdlLOCCD cl;

	printf("sample start.\n");

	printf(" sceCdInit\n");
	sceCdInit(SCECdINIT);
#ifdef MEDIA_CD
	sceCdMmode(SCECdCD);
#else
	sceCdMmode(SCECdDVD);
#endif

	printf(" sceCdDiskReady\n");
	sceCdDiskReady(0);

        old_func= sceCdCallback(test_callback);

        lpcnt= 0;
        while(1){
            if(lpcnt){
                /* メディア交換 */
		traycnt= 0;
		sceCdTrayReq(2,&trayflg);
		printf("Change your PlayStation CD/DVD-ROM\n");
		do{
		    /* 他スレッドへの影響を考えリソース返却 */
		    DelayThread(SAMPLE_WAIT);
		    while(!sceCdTrayReq(2,&trayflg)){
		        /* 他スレッドへの影響を考えリソース返却 */
		    	DelayThread(SAMPLE_WAIT);
		    }
		    traycnt+= trayflg;
		    disk_type= sceCdGetDiskType();
		}while(!traycnt ||
		      (disk_type == SCECdNODISC) ||
		      (disk_type == SCECdDETCT )    );
	    }
            sceCdDiskReady(0);

	    printf(" sceCdGetDiskType   ");
	    disk_type= sceCdGetDiskType();
	    switch(disk_type){
	        case SCECdIllgalMedia:
		    printf("Disk Type= IllgalMedia\n"); break;
	        case SCECdPS2DVD:
		    printf("Disk Type= PlayStation2 DVD\n"); break;
	        case SCECdPS2CD:
		    printf("Disk Type= PlayStation2 CD\n"); break;
	        case SCECdPS2CDDA:
		    printf("Disk Type= PlayStation2 CD with CDDA\n"); break;
	        case SCECdPSCD:
		    printf("Disk Type= PlayStation CD\n"); break;
	        case SCECdPSCDDA:
		    printf("Disk Type= PlayStation CD with CDDA\n"); break;
	        case SCECdDVDV:
		    printf("Disk Type= DVD video\n"); break;
	        case SCECdCDDA:
		    printf("Disk Type= CD-DA\n"); break;
	        case SCECdDETCT:
		    printf("Working\n"); break;
	        case SCECdNODISC: 
		    printf("Disk Type= No Disc\n"); break;
	        default:
		    printf("Disk Type= OTHER DISK\n"); break;
	    }

	    printf("Get TOC\n");
	    sceCdGetToc(toc);

            cp= (u_char *)toc;
            last_track= (int)btoi(*(cp + 17));
            printf("track max= %d\n",last_track);
            for(cnt0= 0,cp+= 30; cnt0 < last_track; cnt0+= 1){
                printf("track No= %d abs Minute= %d Second= %d Frame= %d\n\n",
                  cnt0 + 1, btoi(*(cp + 7)), btoi(*(cp + 8)), btoi(*(cp + 9)));
                cp+= 10;
            }

	    /* 標準入出力でファイルを読む */
            rfd = open("cdrom0:\\SYSTEM.CNF;1", 0);
            if ( rfd < 0 ){
                printf("Can't open SYSTEM.CNF\n");
        	lpcnt++;
                continue;
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
        	lpcnt++;
                continue;
            }
            *cw= 0;
            cw= (char *)bf;
            for(cnt0= 0; (*cw != ':') && (cnt0 < readsize); cnt0++) cw++;
            if(cnt0 == readsize){
                printf("Sorry: File name not Collect\n");
        	lpcnt++;
                continue;
            }
            cw++; filename= cw;

            sceCdDiskReady(0);

	    /* ライブラリ関数でファイルを読む */
	    /* エラー発生時は255回リトライ           */ 
	    mode.trycount= 0;	 
	    /* エラー発生時は回転速度を落してリード	 */
	    mode.spindlctrl= SCECdSpinNom;
            /* データサイズは2048byte	 */
	    mode.datapattern= SCECdSecS2048;
 
	    /* ファイルの格納位置を検索する */
	    printf("Search Filename: %s\n",filename);
	    sceCdSearchFile(&fp, filename);

	    /* 分秒セクタに変換して表示してみる。DVDでは意味が無い */ 
	    sceCdIntToPos(fp.lsn, &cl);
	    printf("File pos minute %d second %d sector %d\n",
				cl.minute,cl.second,cl.sector);
	    /* ブロックナンバに変換して表示する。 */
	    printf("File pos LSN= %d\n",sceCdPosToInt(&cl));

	    /* ファイルの読み込みセクタ数を算出する */
	    rsec= fp.size / 2048; if(fp.size % 2048) rsec++;
	    /* 読み込みバッファの都合により読み込みセクタ数を制限する。*/ 
            if(rsec > 256) rsec= 256;

	    /* ファイルの読み込みを開始する */
            printf("CD_READ LSN= %d sectors= %d\n", fp.lsn,rsec);
            sceCdDiskReady(0);
	    while(!sceCdRead(fp.lsn, rsec, (u_int *)bf, &mode)){
	        printf("Read cmd fail\n");
		DelayThread(SAMPLE_WAIT);
	    }
	    printf("ReadSync ");
            for( rs_flg= 1; rs_flg; ){
                rs_flg= sceCdSync(1); 
            /* ファイルの読み込みステータスを得る 0:終了 */
                printf("Cur Read Position= %d\n",sceCdGetReadPos());
		DelayThread(SAMPLE_WAIT);
	    }

	    printf("Disk error code= 0x%08x\n",sceCdGetError());

	    /* シークテスト */
//	    printf("CdSeek Test\n");
//	    sceCdDiskReady(0);
//	    while(!sceCdSeek(fp.lsn)){
//		DelayThread(SAMPLE_WAIT);
//		printf("Seek cmd fail\n");
//	    }
//	    sceCdSync(0); 

	    /* スタンドバイテスト */
	    printf("CdStandby Test\n");
            sceCdDiskReady(0);
	    while(!sceCdStandby()){
		DelayThread(SAMPLE_WAIT);
		printf("Standby cmd fail\n");
	    }
            sceCdSync(0); 

            /* ポーズテスト */
            printf("CdSPause Test\n");
            sceCdDiskReady(0);
            while(!sceCdPause()){
		DelayThread(SAMPLE_WAIT);
		printf("Pause cmd fail\n");
	    }
            sceCdSync(0);

	    /* テストのためディスクを止めてみる */
	    printf("CdStop Test\n");
            sceCdDiskReady(0);
	    while(!sceCdStop()){
		DelayThread(SAMPLE_WAIT);
		printf("Stop cmd fail\n");
	    }
            sceCdSync(0); 

	    /* 読み込みファイルをホストに保存する。*/
            sprintf(wfilename,"host1:data%d.dat",lpcnt);
	    wfd = open(wfilename, O_WRONLY | O_TRUNC | O_CREAT);
	    if (wfd < 0 ) {
		printf("Can't open file data.dat\n");
        	lpcnt++;
                continue;
	    }
	    write(wfd, (u_int *)bf, 2048 * rsec);
	    close(wfd);


            lpcnt++;
        }
}
/* end of file.*/
