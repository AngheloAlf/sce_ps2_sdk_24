/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
*/
/* 
 *              I/O Processor Library Sample Program
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
#include <string.h>
#include <libsd.h>
#include <libcdvd.h>

/* ================================================================
 *
 *      Program
 *
 * ================================================================ */

#define BASE_priority   82

#define MEDIA_CD

#define _SB_TOP   0x10000
#define _SEC_SIZE 2
#define _BUF_SIZE (2048 * _SEC_SIZE)
#define _SEC_HALF 1
#define _BUF_HALF (2048 * _SEC_HALF)
#define _VOICE_NUM 22
#define _VOICE (1 << _VOICE_NUM)
#define _VOLUME 0x3fff

// for GETSTATUS
#define ADPCM_STATUS_PRELOADED        0x00003000
#define ADPCM_STATUS_RUNNING          0x00005000
#define ADPCM_STATUS_BUFCHG           0x00006000

volatile int gAdpcmStatus= ADPCM_STATUS_PRELOADED;
int buf_side= 0;
int gSem = 0;

// ���[�J�����������̔z�u
unsigned char  ring_buf[2048 * 80] __attribute__((aligned (16)));
unsigned char  sbuf[_BUF_SIZE] __attribute__((aligned (16)));

#define _ADPCM_MARK_START 0x04
#define _ADPCM_MARK_LOOP  0x02
#define _ADPCM_MARK_END   0x01

#define _AdpcmSetMarkSTART(a,s) { *((unsigned char *)((a)+1)) = (_ADPCM_MARK_LOOP | _ADPCM_MARK_START); *((unsigned char *)((a)+0x10+1)) = _ADPCM_MARK_LOOP; *((unsigned char *)((a)+(s)-0x0f)) = _ADPCM_MARK_LOOP; FlushDcache (); }

#define _AdpcmSetMarkEND(a,s) { *((unsigned char *)((a)+1)) = _ADPCM_MARK_LOOP;  *((unsigned char *)((a)+0x10+1)) = _ADPCM_MARK_LOOP; *((unsigned char *)((a)+(s)-0x0f)) = (_ADPCM_MARK_LOOP | _ADPCM_MARK_END); FlushDcache (); }

#define _AdpcmSetMarkSTARTpre(a,s) { *((unsigned char *)((a)+1)) = (_ADPCM_MARK_LOOP | _ADPCM_MARK_START); *((unsigned char *)((a)+0x10+1)) = _ADPCM_MARK_LOOP;  FlushDcache (); }

#define _AdpcmSetMarkENDpre(a,s) { *((unsigned char *)((a)+(s)-0x0f)) = (_ADPCM_MARK_LOOP | _ADPCM_MARK_END); FlushDcache (); }

int my_main(int arg);

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

/* internal */
static int 
_AdpcmDmaInt (int channel, void* data)     // DMA Interrupt
{
    iSignalSema (gSem);
    // ���荞�݂��ēx����
    return 0;  
}

/* internal */
static int
_AdpcmSpu2Int (int core_bit, void* data)            // SPU2 Interrupt
{
    iSignalSema (gSem);
    // ���荞�݂��ēx����
    return 0;
}

// ADPCM_SETVOICE
void
AdpcmSetVoice (int ch, unsigned int vnum)
{
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_VOLL, 0);
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_VOLR, 0);
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_PITCH, 0x1000);
//  pitch�̐ݒ�� �T���v�����O���[�g 48K    PITCH 0x1000
//                                   44.1K  PITCH 0x0eb3
//                                   32K    PITCH 0x0aab
//                                   22.05K PITCH 0x075a
//                                   16K    PITCH 0x0555
//                                   11.25K PITCH 0x03ad
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_ADSR1, 0x000f);
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_ADSR2, 0x1fc0);
    sceSdSetAddr  (SD_CORE_0 | (vnum << 1) | SD_VA_SSA, _SB_TOP);
    return;
}

// ADPCM_SETVOLDIRECT
void
AdpcmSetVolumeDirect (unsigned int vol,int vnum)
{
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_VOLL, vol);
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_VOLR, vol);
    return;
}

/* ���C�������֐� */
int my_main(int arg)
{
	int	cnt0, ret, start_st,file_sec;
	u_int read_sec;
 	u_int   err;
        struct SemaParam sem;
        sceCdlFILE fp;
        sceCdRMode mode;
        char filename[16]= "\\PS_MONO.VB;1";

	printf("sample start.\n");

	printf(" sceSdInit\n");
        sceSdInit(0);
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

        /* �Z�}�t�H�쐬 */
	sem.initCount = 0;
        sem.maxCount = 1;
        sem.attr = AT_THFIFO;
        gSem= CreateSema(&sem);

	// ���荞�݃R�[���o�b�N�֐�
        // �]���I�����荞��
        sceSdSetTransIntrHandler (0, _AdpcmDmaInt, NULL); 
	
        // SPU ���荞��
    	sceSdSetSpu2IntrHandler (_AdpcmSpu2Int, NULL); 

        /* ���C�u�����֐��Ńt�@�C����ǂ� */
	printf("Search Filename: %s\n",filename);
        /* �t�@�C���̊i�[�ʒu���������� */
	ret= sceCdSearchFile(&fp, filename); 
	if(!ret){
		printf("sceCdSearchFile fail :%d\n",ret);
                return(-1);
	}
	sceCdDiskReady(0);
	
        /* �X�g���[���֐��Ńt�@�C����ǂ� */
        /* �G���[�������͂Q�T�T�񃊃g���C����B */
        mode.trycount= 0;
	/* �G���[�������͉�]���x�𗎂��ă��[�h */
        mode.spindlctrl= SCECdSpinNom;
	/* �X�g���[�}�̓f�[�^�T�C�Y�Q�O�S�Wbyte�̂݃T�|�[�g */
        mode.datapattern= SCECdSecS2048;
			 
	file_sec= fp.size / 2048; if(fp.size % 2048) file_sec++;
	start_st= fp.lsn;
	sceCdStStart(start_st , &mode);

	// ���C���{�����[���ݒ�
        for( cnt0= 0; cnt0 < 2; cnt0++ ){
            sceSdSetParam( cnt0 | SD_P_MVOLL , 0x3fff ) ;
            sceSdSetParam( cnt0 | SD_P_MVOLR , 0x3fff ) ;
        }

	AdpcmSetVoice (0, _VOICE_NUM);

	read_sec= 0;
        // �ŏ��̓]��: �o�b�t�@�S��Ƀf�[�^�𑗂�
        read_sec+= sceCdStRead(_SEC_SIZE,(u_int *)sbuf, STMBLK, &err);
        if(err){
            printf("Disk ERROR Found Code= %d\n",err);
	    sceCdStStop();
            return(-1);
        }
        _AdpcmSetMarkSTARTpre(sbuf, _BUF_SIZE);
        _AdpcmSetMarkENDpre(sbuf, _BUF_SIZE);
        sceSdVoiceTrans(0, (SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA),
                          sbuf, _SB_TOP, _BUF_SIZE);
	while(read_sec < file_sec){

            // -- �o�b�t�@�̔����I���҂�
            WaitSema(gSem);
	    switch (gAdpcmStatus) {
                // SPU2 ���荞�ݑ҂� �S��]���I���A���t�J�n
	        case ADPCM_STATUS_PRELOADED:
            	    sceSdSetAddr (SD_CORE_0 | SD_A_IRQA,
						 (u_int)_SB_TOP + _BUF_HALF);
        	    AdpcmSetVolumeDirect ( _VOLUME, _VOICE_NUM );
                    // SPU ���荞�ݗL��
        	    sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 1);
                    // �L�[�I��
        	    sceSdSetSwitch (SD_CORE_0 | SD_S_KON, _VOICE);
                    gAdpcmStatus= ADPCM_STATUS_RUNNING;
                    // preload ��͑O���ɓ]��
            	    buf_side= 0;    
            	    break;
	        case ADPCM_STATUS_RUNNING:
		    // �o�b�t�@�̔����̗̈�̉��t�I����҂�
		    // �񉉑t�̈�Ƀf�[�^��]��
        	    read_sec+= sceCdStRead(_SEC_HALF,(u_int *)sbuf,
							    STMBLK, &err);
		    if(err){
		        printf("Disk ERROR Found Code= %d\n",err);
		        read_sec= file_sec;
		        continue;
		    }
               	    sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 0);
                    // �}�[�N�̏C��
               	    if(!buf_side){
		        _AdpcmSetMarkSTART(sbuf, _BUF_HALF);
		    }else{	 
		        _AdpcmSetMarkEND(sbuf, _BUF_HALF);
		    }
                    // _BUF_HALF ���]��
               	    sceSdVoiceTrans (0, (SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA),
			       sbuf, _SB_TOP + _BUF_HALF * buf_side,
			       _BUF_HALF);
                    // ��ԑJ��
                    gAdpcmStatus= ADPCM_STATUS_BUFCHG; 
               	    break;
	        case ADPCM_STATUS_BUFCHG:
		    // �]���I����o�b�t�@�ؑւ�
                    // SPU2 ���荞�݃A�h���X�ύX
                    sceSdSetAddr (SD_CORE_0 | SD_A_IRQA,
					 _SB_TOP + _BUF_HALF * buf_side);
                    // �o�b�t�@��Ԃ̐ؑւ�
                    buf_side ^= 1; 
                    // SPU2 ���荞�ݗL��
                    sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 1);
                    // ��ԑJ��
                    gAdpcmStatus= ADPCM_STATUS_RUNNING; 
           	    break;
        	default:
           	    break;
	    }
        }
        // SPU2 ���荞�ݒ�~
	sceSdBlockTrans( 0, SD_TRANS_MODE_STOP, NULL, 0 ); 
        // �{�C�X��~
        sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 0); 
	AdpcmSetVolumeDirect (0,_VOICE_NUM);
        sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, _VOICE);
	sceCdStStop();

        printf("sample end.\n");
        return(0);
}
/* end of file.*/
