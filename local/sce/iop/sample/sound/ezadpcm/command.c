/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                  I/O Processor sample program
 *                          Version 0.11
 *                           Shift-JIS
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                      ezadpcm.irx - command.c
 *                           API functions
 *
 *	Version		Date		Design	Log
 *  --------------------------------------------------------------------
 *	0.10		Feb. 3, 2000	kaol		
 *	0.11		Feb.27, 2000	kaol	entire waveform data is
 *                                               read at once.
 *	0.12		Feb.15, 2001	kaol		
 */

#include <kernel.h>
#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <libsd.h>
#include "ezadpcm.h"

// ================================================================

// �g�`�f�[�^���
typedef struct {
    unsigned int     size;
    unsigned int     rest;
    unsigned int     sb;
} AdpcmWave;
AdpcmWave gWave [2];		// L, R

#define _SB_TOP   0x10000
#define _BUF_SIZE 0x4000
#define _BUF_HALF 0x2000
// SPU2 ���[�J�����������̔z�u
unsigned int gSBTop [2] = { _SB_TOP, _SB_TOP + _BUF_SIZE }; // L, R

int gThid = 0;
int gSem = 0;
int gFd [2] = { -1, -1 };	// L, R
int gBufSpu [2];		// IOP SMEM ��: L, R
int gCurAddr [2];		// IOP SMEM �����݈ʒu: L, R
int gVnum [2] = { 0, 1 };	// �g�p����{�C�X�ԍ�: L, R
volatile unsigned int gAdpcmVolume;  // �{�����[���ݒ�l
volatile int gAdpcmVolumeSet = 0;    // �{�����[���ݒ�v��������� 1
volatile int gAdpcmPreLoadStart = 0; // �X�g���[���J�n�v��������� 1
volatile int gAdpcmStart = 0;	     // �X�g���[���J�n
volatile int gAdpcmStop = 0;	     // �X�g���[����~�v��������� 1

// �X�g���[�~���O�����̏��
volatile int gAdpcmStatus = EzADPCM_STATUS_IDLE;

int _AdpcmPlay (int status);
static int _AdpcmDmaInt (int, void*);
static int _AdpcmSpu2Int (int, void *);

// ���]�����̏�ԑJ��
#define _DO_TRANS_L 0
#define _DO_TRANS_R 1
#define _DO_WAIT_IRQ 2
volatile int trans_stat = _DO_TRANS_L;

#define _L 0
#define _R 1
#define _Lch(x) ((x >> 16) & 0xffff)
#define _Rch(x) (x & 0xffff)
#define _VOICE (1 << gVnum[0] | 1 << gVnum[1])

// tr1?_pad.vb �̃t�@�C���T�C�Y
#define _FILESIZE 499712

// ================================================================
// RPC �R�}���h 1 �� 1 �Ή��֐�

// EzADPCM_SDINIT
void
AdpcmSdInit (int no_ch, int status)
{
    sceSdInit (0);

    //    Disk media: CD
    // Output format: PCM
    //    Copy guard: normal (one generation recordable / default)
    sceSdSetCoreAttr (SD_C_SPDIF_MODE, (SD_SPDIF_MEDIA_CD |
					SD_SPDIF_OUT_PCM  |
					SD_SPDIF_COPY_NORMAL));
    return;
}

// EzADPCM_INIT
int
AdpcmInit (int allocsize)
{
    int oldstat;

    if (gSem == 0){		// �Z�}�t�H�쐬
	struct SemaParam sem;
	sem.initCount = 0;
	sem.maxCount = 1;
	sem.attr = AT_THFIFO;
	gSem = CreateSema (&sem);
    }
    if (gThid == 0) {		// �X���b�h�쐬
	struct ThreadParam param;
	param.attr         = TH_C;
	param.entry        = _AdpcmPlay;
	param.initPriority = BASE_priority-3;
	param.stackSize    = 0x800;
	param.option = 0;
	gThid = CreateThread (&param);
	printf ("EzADPCM: create thread ID= %d\n", gThid);
	// �X���b�h���N��
	StartThread (gThid, (u_long) NULL);
    }

    // ���荞�݃n���h��
    sceSdSetTransIntrHandler (0, (sceSdTransIntrHandler) _AdpcmDmaInt,
			      (void *) &gSem); // �]���I�����荞��
    sceSdSetSpu2IntrHandler ((sceSdSpu2IntrHandler) _AdpcmSpu2Int,
			     (void *) &gSem); // SPU ���荞��

    // �g�`�f�[�^�p�o�b�t�@�m��
    CpuSuspendIntr (&oldstat);
    gBufSpu [_L] = (int) AllocSysMemory (0, allocsize, NULL);
    gBufSpu [_R] = (int) AllocSysMemory (0, allocsize, NULL);
    CpuResumeIntr (oldstat);
    PRINTF ((" alloc memory 0x%08x - 0x%08x (%x)\n",
	     gBufSpu [_L], gBufSpu [_L] + allocsize, allocsize));
    PRINTF ((" alloc memory 0x%08x - 0x%08x (%x)\n",
	     gBufSpu [_R], gBufSpu [_R] + allocsize, allocsize));
    return gThid;
}

// EzADPCM_QUIT
void
AdpcmQuit (void)
{
    int oldstat;

    // �e�����̉��
    CpuSuspendIntr (&oldstat);
    FreeSysMemory ((void*)gBufSpu [_L]);
    FreeSysMemory ((void*)gBufSpu [_R]);
    CpuResumeIntr (oldstat);
    DeleteThread (gThid);
    gThid = 0;
#if 0
    DeleteSema (gSem);
    gSem = 0;
#endif
    return;
}

// EzADPCM_OPEN
int
AdpcmOpen (int ch, char *filename)
{
    PRINTF (("filename %s\n", filename));

    // ADPCM �t�@�C���̃I�[�v��
    if ((gFd [ch] = open (filename, O_RDONLY)) < 0) {
	ERROR(("file open failed. %s \n", filename));
	return -1;
    }
    PRINTF (("fd %d, ch %d\n", gFd [ch], ch));

    gWave [ch].size = gWave[ch].rest = _FILESIZE;
    gWave [ch].sb = gSBTop [ch];
    
    return (gWave[ch].size);
}

// EzADPCM_CLOSE
void
AdpcmClose (void)
{
    if (gFd [_L] >= 0) close (gFd [_L]);
    if (gFd [_R] >= 0) close (gFd [_R]);
    return;
}

// EzADPCM_PRELOAD
int
AdpcmPreLoad (void)
{
    extern void AdpcmSetVolumeDirect (unsigned int);

    if (gAdpcmStatus != EzADPCM_STATUS_IDLE) {
	return -1;		// ��ԑJ�ڃG���[
    }

    gWave [_L].rest = gWave [_L].size;
    gWave [_R].rest = gWave [_R].size;
    lseek (gFd[_L], 0, SEEK_SET);
    lseek (gFd[_R], 0, SEEK_SET);
    AdpcmSetVolumeDirect (gAdpcmVolume);
    
    if (read (gFd [_L], (unsigned char *) gBufSpu [_L], _FILESIZE) != _FILESIZE) {
	ERROR (("AdpcmPreLoad: read failed \n"));
	return -1;
    }
    if (read (gFd [_R], (unsigned char *) gBufSpu [_R], _FILESIZE) != _FILESIZE) {
	ERROR (("AdpcmPreLoad: read failed \n"));
	return -1;
    }
    gAdpcmStatus = EzADPCM_STATUS_PRELOAD;

    gCurAddr [_L] = gBufSpu [_L];
    gCurAddr [_R] = gBufSpu [_R];

    // �Z�}�t�H�ɃV�O�i���𑗂� (preload, start �̂�)
    SignalSema (gSem);

    return 0;
}

// EzADPCM_PRELOADSTART
int
AdpcmPreLoadStart ()
{
    int ret;

    gAdpcmPreLoadStart ++;
    trans_stat = _DO_TRANS_L;
    ret = AdpcmPreLoad ();

    if (ret < 0) {
	gAdpcmPreLoadStart = 0;
	return -1;
    }
    return 0;
}

// EzADPCM_STOP
int
AdpcmStop (void)
{
    switch (gAdpcmStatus) {
    case EzADPCM_STATUS_RUNNING:
	gAdpcmStop ++;
	return 0;
    default:
	return -1;		// ��ԑJ�ڃG���[
    }
}

// EzADPCM_SETVOICE
void
AdpcmSetVoice (int ch, unsigned int vnum)
{
    gVnum [ch] = vnum;
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_VOLL, 0);
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_VOLR, 0);
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_PITCH, 0x1000);
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_ADSR1, 0x000f);
    sceSdSetParam (SD_CORE_0 | (vnum << 1) | SD_VP_ADSR2, 0x1fc0);
    sceSdSetAddr  (SD_CORE_0 | (vnum << 1) | SD_VA_SSA, gWave[ch].sb);

    return;
}

// EzADPCM_SETVOL
void
AdpcmSetVolume (int no_ch, unsigned int vol)
{
    gAdpcmVolumeSet = 1;	// _AdpcmPlay() �̌��Őݒ�
    gAdpcmVolume = vol;
    return;
}

// EzADPCM_SETVOLDIRECT
void
AdpcmSetVolumeDirect (unsigned int vol)
{
    gAdpcmVolume = vol;
    sceSdSetParam (SD_CORE_0 | (gVnum[0] << 1) | SD_VP_VOLL, _Lch (vol));
    sceSdSetParam (SD_CORE_0 | (gVnum[0] << 1) | SD_VP_VOLR, 0);
    sceSdSetParam (SD_CORE_0 | (gVnum[1] << 1) | SD_VP_VOLL, 0);
    sceSdSetParam (SD_CORE_0 | (gVnum[1] << 1) | SD_VP_VOLR, _Rch (vol));
    return;
}

// EzADPCM_SETMASTERVOL
void
AdpcmSetMasterVolume (int core, unsigned int vol)
{
    sceSdSetParam (core | SD_P_MVOLL, _Lch (vol));
    sceSdSetParam (core | SD_P_MVOLR, _Rch (vol));
    return;
}

// EzADPCM_GETSTATUS
unsigned int
AdpcmGetStatus (void)
{
    return gAdpcmStatus;
}

// ================================================================

#define _ADPCM_MARK_START 0x04
#define _ADPCM_MARK_LOOP  0x02
#define _ADPCM_MARK_END   0x01

#define _AdpcmSetMarkSTART(a,s) { \
  *((unsigned char *)((a)+1)) =       (_ADPCM_MARK_LOOP | _ADPCM_MARK_START); \
  *((unsigned char *)((a)+0x10+1))   = _ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+(s)-0x0f)) = _ADPCM_MARK_LOOP; \
  FlushDcache (); }
#define _AdpcmSetMarkEND(a,s) { \
  *((unsigned char *)((a)+1))        =  _ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+0x10+1))   =  _ADPCM_MARK_LOOP; \
  *((unsigned char *)((a)+(s)-0x0f)) = (_ADPCM_MARK_LOOP | _ADPCM_MARK_END); \
  FlushDcache (); }

#define _AdpcmSetMarkSTARTpre(a,s) { \
  *((unsigned char *)((a)+1))      = (_ADPCM_MARK_LOOP | _ADPCM_MARK_START); \
  *((unsigned char *)((a)+0x10+1)) =  _ADPCM_MARK_LOOP; }
#define _AdpcmSetMarkENDpre(a,s) { \
  *((unsigned char *)((a)+(s)-0x0f)) = (_ADPCM_MARK_LOOP | _ADPCM_MARK_END); }

/* internal */
static int
_AdpcmDmaInt (int ch, void *common)	// DMA Interrupt
{
    iSignalSema (* (int *) common);
    return 1;  // ���荞�݂��ēx����
}

/* internal */
static int
_AdpcmSpu2Int (int core, void *common)		// SPU2 Interrupt
{
    iSignalSema (* (int *) common);
    return 1;	// ���荞�݂��ēx����
}

/* internal */
// gAdpcmVolume ��ύX�����Ƀ{�C�X�E�{�����[���� 0 �ɂ���
void
_AdpcmSetVoiceMute (void)
{
    sceSdSetParam (SD_CORE_0 | (gVnum[0] << 1) | SD_VP_VOLL, 0);
    sceSdSetParam (SD_CORE_0 | (gVnum[0] << 1) | SD_VP_VOLR, 0);
    sceSdSetParam (SD_CORE_0 | (gVnum[1] << 1) | SD_VP_VOLL, 0);
    sceSdSetParam (SD_CORE_0 | (gVnum[1] << 1) | SD_VP_VOLR, 0);
    return;
}

#define _1st 0
#define _2nd 1

/* internal */
int
_AdpcmPlay (int status)
{
    int buf_side = _1st;	// preload ��͑O���ɓ]��
    int count = 0;

    while (1) {
	// �o�b�t�@�̔����I���҂�
        WaitSema(gSem);
	if (gAdpcmVolumeSet){
	    gAdpcmVolumeSet = 0;
	    // �{�����[���ύX�C�x���g
	    AdpcmSetVolumeDirect (gAdpcmVolume);
	}

	switch (gAdpcmStatus) {
	case EzADPCM_STATUS_IDLE:
	case EzADPCM_STATUS_RUNNABLE:
	    break;
	case EzADPCM_STATUS_PRELOAD: // �ŏ��̓]��: �o�b�t�@�S��Ƀf�[�^�𑗂�
	    buf_side = _1st;	// preload ��͑O���ɓ]��
	    // L ch �S��]��
	    trans_stat = _DO_TRANS_R;
	    gWave [_L].rest -= _BUF_SIZE;
	    _AdpcmSetMarkSTARTpre (gBufSpu [_L], _BUF_SIZE);
	    _AdpcmSetMarkENDpre   (gBufSpu [_L], _BUF_SIZE);
	    FlushDcache ();
	    gAdpcmStatus = EzADPCM_STATUS_PRELOADING;
	    sceSdVoiceTrans (0, (SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA),
			     (unsigned char *) gBufSpu [_L],
			     gWave [_L].sb,
			     _BUF_SIZE);
	    break;
	case EzADPCM_STATUS_PRELOADING:
	    // R ch �S��]�� �� SPU2 ���荞�ݑ҂�
	    trans_stat = _DO_WAIT_IRQ;
	    gWave [_R].rest -= _BUF_SIZE;
	    _AdpcmSetMarkSTARTpre (gBufSpu [_R], _BUF_SIZE);
	    _AdpcmSetMarkENDpre   (gBufSpu [_R], _BUF_SIZE);
	    FlushDcache ();
	    gAdpcmStatus = EzADPCM_STATUS_PRELOADED;
	    sceSdVoiceTrans (0, (SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA),
			     (unsigned char *) gBufSpu [_R],
			     gWave [_R].sb,
			     _BUF_SIZE);
	    break;
	case EzADPCM_STATUS_PRELOADED:
	    // SPU2 ���荞�ݑ҂�
	    trans_stat = _DO_TRANS_L;
	    gCurAddr [_L] += _BUF_SIZE;
	    gCurAddr [_R] += _BUF_SIZE;
	    sceSdSetAddr (SD_CORE_0 | SD_A_IRQA, gWave [_L].sb + _BUF_HALF);
	    gAdpcmStatus = EzADPCM_STATUS_RUNNABLE;
	    if (gAdpcmPreLoadStart > 0) { // Preload �ɑ����� Start
		gAdpcmPreLoadStart = 0;
		gAdpcmStart ++;
	    }
	    break;
	case EzADPCM_STATUS_RUNNING:
	    /* SPU2 ���荞�ݒ�~ �� L �]�� �� R �]�� �� SPU2 ���荞�ݑ҂� */
	    switch (trans_stat) {
	    case _DO_TRANS_L:
		// SPU2 ���荞�ݒ�~
		sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 0);
		// �}�[�N�̏C��
		if (buf_side == _1st) {
		    _AdpcmSetMarkSTART (gCurAddr [_L], _BUF_HALF);
		} else {
		    _AdpcmSetMarkEND   (gCurAddr [_L], _BUF_HALF);
		}
		trans_stat = _DO_TRANS_R; // ��ԑJ��
		// _BUF_HALF ���]��
		gWave [_L].rest -= _BUF_HALF;
		sceSdVoiceTrans (0, (SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA),
				 (unsigned char *) gCurAddr [_L],
				 (gWave [_L].sb + _BUF_HALF * buf_side),
				 _BUF_HALF);
		break;
	    case _DO_TRANS_R:
		// �}�[�N�̏C��
		if (buf_side == _1st) {
		    _AdpcmSetMarkSTART (gCurAddr [_R], _BUF_HALF);
		} else {
		    _AdpcmSetMarkEND   (gCurAddr [_R], _BUF_HALF);
		}
		trans_stat = _DO_WAIT_IRQ; // ��ԑJ��
		// _BUF_HALF ���]��
		gWave [_R].rest -= _BUF_SIZE;
		sceSdVoiceTrans (0, (SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA),
				 (unsigned char *) gCurAddr [_R],
				 (gWave [_R].sb + _BUF_HALF * buf_side),
				 _BUF_HALF);
		break;
	    case _DO_WAIT_IRQ:
		trans_stat = _DO_TRANS_L; // ��ԑJ��
		// SPU2 ���荞�݃A�h���X�ύX
		sceSdSetAddr (SD_CORE_0 | SD_A_IRQA, gWave [_L].sb + _BUF_HALF * buf_side);
		gCurAddr [_L] += _BUF_HALF;
		gCurAddr [_R] += _BUF_HALF;
		// �o�b�t�@��Ԃ̐ؑւ�
		buf_side = (buf_side == _1st) ? _2nd : _1st;
		if (gWave [_L].rest <= 0) {
		    // �f�[�^�����߂�
		    gWave [_L].rest = gWave [_L].size;
		    gWave [_R].rest = gWave [_R].size;
		    gCurAddr [_L] = gBufSpu [_L];
		    gCurAddr [_R] = gBufSpu [_R];
		    count ++;
		    PRINTF (("-- rewind (%d)\n", count));
		}
		if (gAdpcmStop) { // �X�g���[�~���O��~�v��
		    gAdpcmStop = 0;
		    // ��ԑJ��
		    gAdpcmStatus = EzADPCM_STATUS_TERMINATE;
		}
		// SPU2 ���荞�ݗL��
		sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 1);
		break;
	    }
	    break;
	case EzADPCM_STATUS_TERMINATE: // �X�g���[�~���O��~
	    // SPU2 ���荞�ݒ�~
	    sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 0);
	    // ��ԑJ��
	    gAdpcmStatus = EzADPCM_STATUS_IDLE;
	    // �{�C�X��~
	    _AdpcmSetVoiceMute ();
	    sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, _VOICE);
	    continue;
	    break;
	default:
	    break;
	}

	if (gAdpcmStart) {
	    if (gAdpcmStatus == EzADPCM_STATUS_RUNNABLE) {
		gAdpcmStart = 0;
		// �{�����[���ݒ�
		AdpcmSetVolumeDirect (gAdpcmVolume);
		// ��ԑJ��
		gAdpcmStatus = EzADPCM_STATUS_RUNNING;
		// SPU ���荞�ݗL��
		sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 1);
		// �L�[�I��
		sceSdSetSwitch (SD_CORE_0 | SD_S_KON, _VOICE);
	    }
	}
    }
    return 0;
}

// ================================================================
// RPC �R�}���h�f�B�X�p�b�`

static int rpc_arg [16];	// EE ���� RPC �o�R�œ]����������
volatile int ret = 0;		// EE �ւ̕Ԃ�l

/* ----------------------------------------------------------------
 * EE ����̃��N�G�X�g�ɂ���ċN�������֐��B
 * ������*data�Ɋi�[����Ă���B�擪 4 �o�C�g�͗\���p�Ŏg���Ă��Ȃ��B
 * ���̊֐��̕Ԓl���AEE ���� RPC �̕Ԓl�ɂȂ�B
 * �������\���̂̏ꍇ�́AgRpcData �ɑ����Ă���̂ł�����g�p����B
 * �\���̂� EE �ɕԂ��ꍇ�́A��P�����̃A�h���X (EE ��) �ɒl�𑗂�B
 * ---------------------------------------------------------------- */
static void*
dispatch (unsigned int command, void *data_, int size)
{ 
    int ch;
    int          data  = *((         int *) data_);
    unsigned int dataU = *((unsigned int *) data_);

    PRINTF (("# dispatch [%04x] %x, %x, %x, %x\n",
	      command,
	      *((int*) data_ + 0), 
	      *((int*) data_ + 1),
	      *((int*) data_ + 2),
	      *((int*) data_ + 3)));

    ch = command & EzADPCM_CH_MASK;
    switch (command & EzADPCM_COMMAND_MASK) {
    case EzADPCM_INIT:	       ret = AdpcmInit (data);			break;
    case EzADPCM_QUIT:		     AdpcmQuit ();			break;
    case EzADPCM_OPEN:	       ret = AdpcmOpen (ch, (char *) data_);	break;
    case EzADPCM_CLOSE:		     AdpcmClose ();			break;
//  case EzADPCM_PRELOAD:      ret = AdpcmPreLoad ();			break;
//  case EzADPCM_START:	       ret = AdpcmStart ();			break;
    case EzADPCM_PRELOADSTART: ret = AdpcmPreLoadStart ();		break;
    case EzADPCM_STOP:	       ret = AdpcmStop ();			break;
    case EzADPCM_SETVOL:	     AdpcmSetVolume (ch, dataU);	break;
    case EzADPCM_SETVOLDIRECT:       AdpcmSetVolumeDirect (dataU);	break;
    case EzADPCM_SETMASTERVOL:       AdpcmSetMasterVolume (ch, dataU);	break;
    case EzADPCM_GETSTATUS:    ret = AdpcmGetStatus ();			break;
    case EzADPCM_SETVOICE:           AdpcmSetVoice (ch, data);		break;
//  case EzADPCM_SETADDR:            AdpcmSetAddr (ch, data);		break;
//  case EzADPCM_SETSIZE:            AdpcmSetSize (ch, data);		break;
//  case EzADPCM_SEEK:		     AdpcmSeek (ch, dataU); 		break;
    case EzADPCM_SDINIT:	     AdpcmSdInit (ch, data);		break;
    default:
	ERROR (("EzADPCM driver error: unknown command %d \n", data));
	break;
    }
    PRINTF (("! return value = %x \n", ret)); 
    return (void*)(&ret);
}

/* ----------------------------------------------------------------
 * ezadpcm ���W���[���̃��C���X���b�h�B
 * ���s��A���荞�݊��̏������ƃR�}���h�̓o�^���s���A�Ȍ�� EE ����
 * ���N�G�X�g������܂ŃE�G�C�g����B
 * ---------------------------------------------------------------- */
int
sce_adpcm_loop (void)
{
    sceSifQueueData qd;
    sceSifServeData sd;

    // ���N�G�X�g�ɂ���ăR�[�������֐���o�^
    sceSifInitRpc (0);
    sceSifSetRpcQueue (&qd, GetThreadId ());
    sceSifRegisterRpc (&sd, EzADPCM_DEV, dispatch, (void*)rpc_arg, NULL, NULL, &qd);
    PRINTF (("goto adpcm cmd loop\n"));
    
    // �R�}���h�҂����[�v
    sceSifRpcLoop (&qd);

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
