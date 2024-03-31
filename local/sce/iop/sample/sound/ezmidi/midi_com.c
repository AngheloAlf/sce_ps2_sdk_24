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
 *                      ezmidi.irx - midi_com.c
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
#include "ezmidi_i.h"

int gRpcArg [16];	//--- EE����]�������RPC�̈����̎󂯌�

static void* midiFunc (unsigned int fno, void *data, int size);

extern int MidiStart (int foo);
extern int MidiInit (int foo);
extern int MidiSetHd (int port, EZMIDI_BANK *bank);
extern int MidiSetSq (int addr);
extern int MidiStop (int addr);
extern int MidiSeek (int addr);
extern int MidiTransBd (EZMIDI_BANK *bank);
extern int MidiTransBdPacket (EZMIDI_BANK *bank);
extern int MidiGetIopFileLength (char *filename);
extern int MidiGetStatus (void);
extern void MidiSetPortAttr (int port, int attr);
extern void MidiSetPortVolume (int port, int vol);

/* ------------------------------------------------------------------------
   ezmidi���W���[���̃��C���X���b�h�B
   ���s��A���荞�݊��̏������ƃR�}���h�̓o�^���s���A�Ȍ��EE���烊�N�G�X
   �g������܂ŃE�G�C�g����B
   ------------------------------------------------------------------------*/
int
sce_midi_loop (void)
{
    sceSifQueueData qd;
    sceSifServeData sd;

    //--- ���N�G�X�g�ɂ���ăR�[�������֐���o�^
    sceSifInitRpc (0);

    sceSifSetRpcQueue (&qd, GetThreadId ());
    sceSifRegisterRpc (&sd, EZMIDI_DEV, midiFunc, (void *)gRpcArg, NULL, NULL, &qd);
    PRINTF (("goto midi cmd loop\n"));

    //--- �R�}���h�҂����[�v
    sceSifRpcLoop (&qd);

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
midiFunc (unsigned int command, void *data, int size)
{ 
    int ch;

    PRINTF ((" midifunc %x, %x, %x, %x\n",
	     *((int *)data + 0), *((int *)data + 1),
	     *((int *)data + 2), *((int *)data + 3)));
    
    ch = command & 0x000f;

    switch (command & 0xfff0) {
    case EzMIDI_START:
	MidiStart (*((int *)data));		/* ���t�J�n */
	break;
    case EzMIDI_INIT:
	ret = MidiInit (*((int *)data));	/* CSL ���̏������Ə����̊J�n */
	break;
    case EzMIDI_SETSQ:
	MidiSetSq (*((int *)data));		/* ���ʃf�[�^���̐ݒ� */
	break;
    case EzMIDI_SETHD_P0:			/* �g�`�f�[�^���̐ݒ� */
    case EzMIDI_SETHD_P1:
	MidiSetHd (ch, (EZMIDI_BANK *)((int *)data));
	break;
    case EzMIDI_STOP:				/* ���t��~ */
	MidiStop (*((int *)data));
	break;
    case EzMIDI_SEEK:				/* ���t�J�n�ʒu�̕ύX */
	MidiSeek (*((int *)data));
	break;
    case EzMIDI_TRANSBD:			/* BD �t�@�C���̓]�� */
	ret = MidiTransBd ((EZMIDI_BANK *)((int *)data));
	break;
    case EzMIDI_TRANSBDPACKET:			/* BD �t�@�C���̓ǂݍ��� + �]�� */
	ret = MidiTransBdPacket ((EZMIDI_BANK *)((int *)data));
	break;
    case EzMIDI_GETFILELENGTH:			/* �t�@�C���T�C�Y�̎擾 */
	ret = MidiGetIopFileLength ((char *)((int *)data));
	break;
    case EzMIDI_GETSTATUS:			/* ���t�����̊m�F */
	ret = MidiGetStatus ();
	break;
    case EzMIDI_SETATTR_P0:			/* CSL �n�[�h�E�F�A�V���Z�T�C�U */
    case EzMIDI_SETATTR_P1:			/* �|�[�g�����̐ݒ� */
	MidiSetPortAttr (ch, *((int *)data));
	break;
    case EzMIDI_SETVOL_P0:			/* ���t�{�����[���̐ݒ� */
    case EzMIDI_SETVOL_P1:
	MidiSetPortVolume (ch, *((int *)data));
	break;
    default:
	ERROR (("EzMIDI driver error: unknown command %d \n", *((int *)data)));
	break;
    }
    PRINTF (("return value = %x \n", ret)); 

    return (void*)(&ret);
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */

