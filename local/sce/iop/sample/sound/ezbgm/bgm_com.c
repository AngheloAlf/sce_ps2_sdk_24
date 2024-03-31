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
 *                      ezbgm.irx - bgm_com.c
 *                    command dispatch routine
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   1.20      Nov.23.1999     morita    first checked in.
 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include "sif.h"
#include "sifcmd.h"
#include "sifrpc.h"
#include <libsd.h>
#include "bgm_i.h"

int gRpcArg [16];	//--- EE����]�������RPC�̈����̎󂯌�

static void* bgmFunc(unsigned int fno, void *data, int size);

extern int  BgmInit (int ch, int allocsize);
extern void BgmQuit (int channel, int status);
extern int  BgmOpen (int ch, char *filename);
extern void BgmClose (int ch, int status);
extern int  BgmPreLoad (int ch, int status);
extern void BgmStart (int ch, int status);
extern void BgmStop (int ch, int status);
extern void BgmSeek (int ch, unsigned int vol);
extern void BgmSetVolume (int ch, unsigned int vol);
extern int BgmRaw2Spu (int ch, int which, int mode);
extern void BgmSetVolumeDirect (int ch, unsigned int vol);
extern void BgmSetMasterVolume (int ch, unsigned int vol);
extern unsigned int BgmGetMode (int ch, int status);
extern void BgmSetMode (int ch, u_int mode);
extern void BgmSdInit (int ch, int status);

/* ------------------------------------------------------------------------
   ezbgm���W���[���̃��C���X���b�h�B
   ���s��A���荞�݊��̏������ƃR�}���h�̓o�^���s���A�Ȍ��EE���烊�N�G�X
   �g������܂ŃE�G�C�g����B
   ------------------------------------------------------------------------*/
int
sce_bgm_loop (void)
{
    sceSifQueueData qd;
    sceSifServeData sd;

    //--- ���N�G�X�g�ɂ���ăR�[�������֐���o�^
    sceSifInitRpc(0);

    sceSifSetRpcQueue (&qd, GetThreadId());
    sceSifRegisterRpc (&sd, EZBGM_DEV, bgmFunc, (void*)gRpcArg, NULL, NULL, &qd);
    PRINTF(("goto bgm cmd loop\n"));

    //--- �R�}���h�҂����[�v
    sceSifRpcLoop(&qd);

    return 0;
}

/* ------------------------------------------------------------------------
   EE����̃��N�G�X�g�ɂ���ċN�������֐��B
   ������*data�Ɋi�[����Ă���B�擪4�o�C�g�͗\���p�Ŏg���Ă��Ȃ��B
   ���̊֐��̕Ԓl���AEE����RPC�̕Ԓl�ɂȂ�B
   �������\���̂̏ꍇ�́AgRpcData�ɑ����Ă���̂ł�����g�p����B
   �\���̂�EE�ɕԂ��ꍇ�́A��P�����̃A�h���X�iEE���j�ɒl�𑗂�B
   ------------------------------------------------------------------------*/
int ret = 0;

static void *
bgmFunc (unsigned int command, void *data, int size)
{ 
    int ch;

    PRINTF ((" bgmfunc %x, %x, %x, %x\n",
	     *((int *)data + 0), *((int *)data + 1),
	     *((int *)data + 2), *((int *)data + 3)));

    ch = command & 0x000f;
	
    switch (command & 0xfff0) {
    case EzBGM_INIT:				/* ���������� */
	ret = BgmInit (ch, *((int *)data));
	break;
    case EzBGM_QUIT:				/* �I������ */
	BgmQuit (ch, *((int *)data));
	break;
    case EzBGM_OPEN:				/* �g�`�f�[�^�t�@�C�����I�[�v�� */
	ret = BgmOpen (ch, (char *)((int *)data));
	break;
    case EzBGM_CLOSE:				/* �g�`�f�[�^�t�@�C�����N���[�Y */
	BgmClose (ch, *((int *)data));
	break;
    case EzBGM_PRELOAD:				/* �g�`�f�[�^���o�b�t�@����ǂ� */
	ret = BgmPreLoad (ch, *((int *)data));
	break;
    case EzBGM_START:				/* ���t�J�n */
	BgmStart (ch, *((int *)data));
	break;
    case EzBGM_STOP:				/* ���t��~ */
	BgmStop (ch, *((int *)data));
	break;
    case EzBGM_SEEK:				/* �t�@�C���̓ǂݏo���ʒu�̈ړ� */
	BgmSeek (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_SETVOL:				/* �{�����[���ݒ� */
	BgmSetVolume (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_SETVOLDIRECT:			/* �{�����[���𑦍��ɕύX */
	BgmSetVolumeDirect (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_SETMASTERVOL:			/* �}�X�^�[�{�����[���̕ύX */
	BgmSetMasterVolume (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_SETMODE:				/* �X�e���I/���m�����̐ݒ� */
	BgmSetMode (ch, (unsigned int)*((int *)data));
	break;
    case EzBGM_GETMODE:				/* �X�e���I/���m�����̌��ݒl�̎擾 */
	ret = BgmGetMode (ch, *((int *)data));
	break;
    case EzBGM_SDINIT:				/* �჌�x���T�E���h���C�u�����̏����� */
	BgmSdInit (ch, *((int *)data));
	break;
    default:
	ERROR(("EzBGM driver error: unknown command %d \n", *((int *)data)));
	break;
    }
    PRINTF(("return value = %x \n", ret)); 

    return (void *)(&ret);
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
