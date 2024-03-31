/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
*/
/* 
 *             I/O Processor Library Sample Program
 * 
 *                         - CD/DVD -
 * 
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 * 
 */

#include <stdio.h>
#include <kernel.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <string.h>
#include <libsd.h>
#include <libcdvd.h>
#include <string.h>

/* ================================================================
 *
 *      Program
 *
 * ================================================================ */

#define BASE_priority   82
#define READ_SECTOR	1
#define BUFF_SIZE	(READ_SECTOR * 2048)
#define TOTAL_BUFF_SIZE	(READ_SECTOR * 2048 * 2)

#define MEDIA_CD

// �X�g���[�~���O�o�b�t�@
unsigned char  ring_buf[2048 * 80] __attribute__((aligned (16)));
// �X�g���[�gPCM�p�o�b�t�@
unsigned char  sbuf[TOTAL_BUFF_SIZE] __attribute__((aligned (16)));

int my_main(int arg);
int gSem = 0;

/* my_main()�֐��X���b�h���쐬���邽�߂̊֐� */
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

/* �]�����荞�݃n���h�� */
int IntFunc0(int channel, void* data )
{
        iSignalSema( gSem );
        // --���荞�݂��ēx������̂ɕK�v
        //return 1;  
        return 0;  
}

/* ���C�������֐� */
int my_main(int arg)
{
	int	cnt0, ret, file_sec, rfds;
 	u_int   err, which, *buf_addr, size;
        struct SemaParam sem;
	sceCdStmInit starg;
	u_char argp[80],resp[16];
        char filename[80]= "cdrom0:\\M_STEREO.INT;1";

	printf("sample start.\n");

        printf(" sceSdInit\n");
        sceSdInit(0);

        /* �W�����o�݂͂̂��g�p����̂�sceCdInit()�֐��̌ďo���͕K�v
            ����܂���                                                  */
        printf(" Drive Ready Wait\n");
        *((int *)argp)= 0;
        devctl("cdrom0:", CDIOC_DISKRDY, argp, 4, resp, 4);

        printf(" Media Mode Set\n");
#ifdef MEDIA_CD
        *((int *)argp)= SCECdCD;
#else
        *((int *)argp)= SCECdDVD;
#endif
        devctl("cdrom0:", CDIOC_MMODE, argp, 4, NULL, 0);

        starg.bufmax= 80;
        starg.bankmax= 5;
        starg.iop_bufaddr= (u_int)ring_buf;
        devctl("cdrom0:", CDIOC_STREAMINIT, (void *)&starg,
                                sizeof(sceCdStmInit), NULL, 0);

        printf(" Drive Ready Wait\n");
        *((int *)argp)= 0;
        devctl("cdrom0:", CDIOC_DISKRDY, argp, 4, resp, 4);

        /* �Z�}�t�H�쐬 */
	sem.initCount = 0;
        sem.maxCount = 1;
        sem.attr = AT_THFIFO;
        gSem= CreateSema(&sem);

        sceSdSetTransIntrHandler( 0, IntFunc0, NULL);

        /* �W�����o�͊֐��Ńt�@�C����ǂ� (�X�g���[��) */
	printf("open Filename: %s\n",filename);
        rfds= open(filename, SCE_CdSTREAM);
	if(rfds < 0){
	    printf("open fail :%d\n",rfds);
            return(-1);
	}
	size= lseek(rfds, 0, SEEK_END);
        if (size < 0) {
            printf("lseek() fails (%s): %d\n", filename, size);
            close(rfds);
            return(-1);
    	}
	ret = lseek(rfds, 0, SEEK_SET);
	if (ret < 0) {
            printf("lseek() fails (%s)\n", filename);
            close(rfds);
            return(-1);
    	}
	file_sec= size / 2048; if(size % 2048) file_sec++;

        ret= read(rfds, (u_int *)sbuf, 2048 * READ_SECTOR * 2);
	devctl("cdrom0:", CDIOC_GETERROR, NULL, 0, &err, 4);
        if(err || (ret < 0)){
                printf("read %d Disk error code= 0x%08x\n",ret, err);
        }

	/* �{�����[����ݒ肷��B*/
        for( cnt0= 0; cnt0 < 2; cnt0++ ){
            sceSdSetParam( cnt0 | SD_P_MVOLL , 0x3fff ) ;
            sceSdSetParam( cnt0 | SD_P_MVOLR , 0x3fff ) ;
        }
        sceSdSetParam( 0 | SD_P_BVOLL , 0x3fff );
        sceSdSetParam( 0 | SD_P_BVOLR , 0x3fff );

	/* �f�[�^�̓]���̐ݒ���s�� */
        sceSdBlockTrans( 0, SD_TRANS_MODE_WRITE|SD_BLOCK_LOOP,
						 sbuf, TOTAL_BUFF_SIZE );

	for(cnt0= READ_SECTOR * 2; cnt0 < file_sec; cnt0+= READ_SECTOR){

                // -- �o�b�t�@�̔����I���҂�
                WaitSema(gSem); 
                which = 1 - (sceSdBlockTransStatus( 0, 0 ) >> 24);
		buf_addr= (u_int *)(sbuf + (which * BUFF_SIZE));
        	ret= read(rfds, buf_addr, 2048 * READ_SECTOR);
		devctl("cdrom0:", CDIOC_GETERROR, NULL, 0, &err, 4);
        	if(err || (ret < 0)){
                	printf("read %d Disk error code= 0x%08x\n", ret, err);
        	}
        }
        sceSdSetParam( 0 | SD_P_BVOLL , 0 );
        sceSdSetParam( 0 | SD_P_BVOLR , 0 );
	sceSdBlockTrans( 0, SD_TRANS_MODE_STOP, NULL, 0 );
	close(rfds);

        printf("sample end.\n");
        return(0);
}
/* end of file.*/
