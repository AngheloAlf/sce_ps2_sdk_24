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

/* �R���p�C�����I�v�V���� */
#define BATCH_MODE      0          // �o�b�`�������s��
#define DMA_CB_TEST	1          // DMA ���荞�݂̃e�X�g
#define IRQ_CB_TEST	1          // IRQ ���荞�݂̃e�X�g
#define IRQ_ADDR_OFST	0x1000     // IRQ�̈ʒu (�g�`�擪����� offset)

#define TMP_OUT		(u_int *)0x110000  //�o�b�`�̌��ʏo��

/* IOP ������ �� SPU2 ���[�J���������ɓ]�����鎞�� DMA �`�����l�� */
#define DMA_CH 0

#define PRINTF(x) printf x
#define BASE_priority  60

#define WAV_DATA_SIZE	20944
#define VAG_FILENAME	"host1:/usr/local/sce/data/sound/wave/piano.vag"
/* VAG �t�@�C���̃w�b�_�����̃T�C�Y�BSPU2 ���[�J�����������ł͕s�v */
#define VAG_HEADER_SIZE 48
/* �]����� SPU2 ���[�J�����������̐擪�A�h���X */
#define VAG_ADDR	0x15010
#define	REVERB_DEPTH 	0x3fff

#include "init_bat.h"  // libsd �o�b�`�R�}���h

char gDoremi [8] = {36, 38, 40, 41, 43, 45, 47, 48}; 
int gEndFlag = 0;

/* ----------------------------------------------------------------
 * ���荞�݃n���h��
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
 * �f�[�^�ǂݍ���
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
    *size = lseek (fd, 0, 2);	/* �t�@�C���T�C�Y�̎擾 */
    if (*size <= 0) {
	printf ("\nCan't load VAG file to iop heap\n");
	return NULL;
    }
    lseek (fd, 0, 0);		/* �ǂݍ��݈ʒu��擪�� */

    PRINTF (("allocate IOP heap memory - "));
    CpuSuspendIntr (&oldstat);
    buffer = AllocSysMemory (0, *size, NULL); /* �̈�m�� */
    CpuResumeIntr (oldstat);
    if (buffer == NULL) {
	printf ("\nCan't alloc heap \n");
	return NULL;
    }
    PRINTF (("alloced 0x%x  \n", (int)buffer));

    read (fd, buffer, *size);	/* �t�@�C���̓��e��ǂݍ��� */
    close (fd);

    return buffer;
}

/* ----------------------------------------------------------------
 * ���C������
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

    /* �t�@�C���̓��e�� IOP ���������ɓǂݍ��� */
    if ((wavBuffer = set_data (&size)) == NULL){
	return -1;
    }

    /* �჌�x���T�E���h���C�u�����̏����� */
    sceSdInit (0);

#if BATCH_MODE
    /* �o�b�`����: 4 �̃R�}���h�����s */
    i = sceSdProcBatch (gBatchCommonInit, NULL, 4);
    PRINTF (("sceSdProcBatch count = %d \n", i));

    /* �o�b�`����: 7 �̃R�}���h�����s: �Ԃ�l��̈� TMP_OUT �ɏo�� */
    sceSdProcBatch (gBatchCommonInit2, TMP_OUT, 7);
    printf(" check wite_iop: %x\n", *((short *)0x90000));
#else
    /* �}�X�^�[�{�����[����ݒ� */
    for (i = 0; i < 2; i++) {
	sceSdSetParam (i | SD_P_MVOLL, 0x3fff);
	sceSdSetParam (i | SD_P_MVOLR, 0x3fff);
    }
#endif

    /*
     * �f�[�^�]��
     */
    PRINTF (("Data transfer ...\n"));
#if DMA_CB_TEST
    /* DMA �]���I�����荞�݃n���h����ݒ� */
    sceSdSetTransIntrHandler (DMA_CH, (sceSdSpu2IntrHandler) IntTrans, (void *) &gEndFlag);
#endif
    /* �]���J�n */
    sceSdVoiceTrans (DMA_CH,
		     SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
		     (u_char *)(wavBuffer + VAG_HEADER_SIZE),
		     VAG_ADDR, (u_int)WAV_DATA_SIZE);
    PRINTF (("TSA = %x \n", sceSdGetAddr (DMA_CH | SD_A_TSA)));

#if DMA_CB_TEST
    while (gEndFlag == 0) {   /* ���荞�݃n���h�����Ă΂��܂ő҂� */
	DelayThread (100 * 1000); /* �C���^�[�o��: 0.1 �b */
    }
#else
    sceSdVoiceTransStatus (DMA_CH, SD_TRANS_STATUS_WAIT); /* �]���I����҂� */
#endif

#if IRQ_CB_TEST
    /*
     * SPU2 ���荞�݂̐ݒ�
     */
    /* SPU2 ���荞�݂��N����A�h���X��ݒ�
     * ... SPU2 ���[�J�����������́A�g�`�f�[�^�̐擪���� 0x1000 �̈ʒu */
    sceSdSetAddr (SD_CORE_0 | SD_A_IRQA, VAG_ADDR + IRQ_ADDR_OFST);
    gEndFlag = 0;
    /* SPU2 ���荞�݃n���h���̓o�^ */
    sceSdSetSpu2IntrHandler ((sceSdSpu2IntrHandler) IntFunc, (void *) &gEndFlag);
    /* SPU2 ���荞�݂�L���� (CORE0 ��) */
    sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 1);
#endif

    for (core = 0; core < 2; core ++) {
#if BATCH_MODE
	/* �o�b�`����: �w��{�C�X���ꊇ����
	 * ��4����/0xffffff ... �S�Ẵr�b�g�� 1 = �{�C�X 0 �` 23 ��S�Đݒ� */
	ret = sceSdProcBatchEx (gBatchVoiceInit  [core], NULL,    6, 0xffffff);
	ret = sceSdProcBatchEx (gBatchVoiceInit2 [core], TMP_OUT, 6, 0xffffff);
	PRINTF (("sceSdProcBatchEx count = %d \n", ret));
#else
	for (v = 0; v < 24; v ++) {
	    /* �{�C�X�̑��������ꂼ��ݒ� */
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
     * �G�t�F�N�g�����̐ݒ�
     */
    r_attr.depth_L  = 0;	/* �ŏ��� 0 �� */
    r_attr.depth_R  = 0;
    /* r_attr.delay    = 30; */
    /* r_attr.feedback = 200; */
    /* ���[�h: �z�[�� + ���[�N�G���A�������� */
    r_attr.mode = SD_REV_MODE_HALL | SD_REV_MODE_CLEAR_WA;
    sceSdSetEffectAttr (SD_CORE_0, &r_attr);

    // effect on
    /* CORE0: �G�t�F�N�g�L�� */
    sceSdSetCoreAttr (SD_CORE_0 | SD_C_EFFECT_ENABLE, 1);
    /* CORE0: �G�t�F�N�g�Z���h�{�����[�� */
    sceSdSetParam    (SD_CORE_0 | SD_P_EVOLL, REVERB_DEPTH);
    sceSdSetParam    (SD_CORE_0 | SD_P_EVOLR, REVERB_DEPTH);
    /* CORE0: �S�Ẵ{�C�X�� L/R �o�͂��G�t�F�N�g�ɐڑ�  */
    sceSdSetSwitch   (SD_CORE_0 | SD_S_VMIXEL, 0xffffff);
    sceSdSetSwitch   (SD_CORE_0 | SD_S_VMIXER, 0xffffff);

    // Ring!
    for (v = 15; v < 23; v ++) {
#if IRQ_CB_TEST
	/* SPU2 ���荞�݂��N�����ꍇ�A��U�����ɂ��čĐݒ肷�� */
	sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 0);
	sceSdSetCoreAttr (SD_CORE_0 | SD_C_IRQ_ENABLE, 1);
#endif
	/* �s�b�`�ݒ� */
	sceSdSetParam (SD_CORE_0 | (v << 1) | SD_VP_PITCH, 
		       sceSdNote2Pitch (60, 0, gDoremi[v - 15], 0));
	/* ���� */
	sceSdSetSwitch (SD_CORE_0 | SD_S_KON, 1 << v);

	PRINTF((" ++ key_on  pitchA %x, pitchB %x \n", 
		sceSdNote2Pitch (60, 0, gDoremi[v - 15], 0), 
		sceSdGetParam (SD_CORE_0 | (v << 1) | SD_VP_PITCH)));
	
	DelayThread (1000 * 1000); /* 1 �b�҂� */

	/* ���� */
	sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, 1 << v);
    }

    /* �O�̂��� CORE0 �S�Ẵ{�C�X������ */
    sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, 0xffffff);

    PRINTF(("voice completed...\n"));

    return 0;
}

/* ----------------------------------------------------------------
 * �X�^�[�g�֐�
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
	StartThread(thid,0);	/* �X���b�h�̋N�� */
	return RESIDENT_END;	/* program is resident */
    } else {
	return NO_RESIDENT_END;	/* program is deleted */
    }
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
