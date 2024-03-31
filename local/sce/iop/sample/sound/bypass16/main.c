/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                          bypass16 - main.c
 *         16 bit PCM data streaming with SPU2 bypass processing by libsd
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *               May 2001      kaol     first implementation
 */

#include <kernel.h>
#include <stdio.h>
#include <sys/file.h>
#include <libsd.h>

/*
 * CORE0 MEMIN �g�o�C�p�X�����h�ŗv�������f�[�^�`��
 * ----------------------------------------------------------------
 *   - 512 �o�C�g L/R �C���^�[���[�u���ꂽ�f�[�^ (CORE[0,1] MEMIN �ŋ���)
 *   - Lch �����́A16 bit PCM �f�[�^���A�C���^�[���[�u�P�ʃu���b�N�̐擪
 *     ���� L, R, L, R ... �� L/R �e 128 �T���v���A�v 256 �T���v��
 *     (���v 512 �o�C�g) �����ׂ�
 *   - Rch �����͑S�� 0 (�v 512 �o�C�g)
 */
/*
 * Data format for CORE0 MEMIN bypass processing:
 * ----------------------------------------------------------------
 *   - 512-byte L/R interleaved data (common in CORE[0,1] MEMIN)
 *   - Lch part is filled with valid 16 bit PCM data;
 *     L, R, L, R, ... successively, L/R 128 samples, sum. is 256 samples.
 *   - Rch part must be filled with 0 (512 bytes)
 */
#define NORMAL_INTERLEAVE 0
#define BYPASS_INTERLEAVE 1

#define WAVE_MODE BYPASS_INTERLEAVE

#if   WAVE_MODE == NORMAL_INTERLEAVE
#define WF_FSIZE 1605632
#define WF_FILE "host1:/usr/local/sce/data/sound/wave/knot2.int"
#elif WAVE_MODE == BYPASS_INTERLEAVE
#define WF_FSIZE 3211264
#define WF_FILE "host1:/usr/local/sce/data/sound/wave/knot2.inb"
#endif

/* WF_FILE is already 512-byte L/R interleaved */
#define WF_SIZE  (WF_FSIZE + 0)  /* unit: 1024 bytes */

/* filling CORE0 MEMIN area with 0 */
char _0_buf [0x1000];

/* transfer will loop */
#define TRANSFER_LOOP

volatile int transfer_count;

#ifdef TRANSFER_LOOP
/* interrupt handler:
   -- called at the half/end point of buffer */
int
transfer_intr (int ch, void *common)
{
    int *c = (int *)common;

    (*c) ++;
    Kprintf ("    ##### interrupt occurred: count = %2d #####\n", *c);

    return NEXT_ENABLE;
}
#endif

int
start (int argc, char *argv[])
{
    unsigned char *addr;
    int oldei;

    printf ("\n");
    printf ("* SPU2 bypass processing sample (with libsd):\n");

    /* libsd �̏����� */
    sceSdInit (0);

    printf ("  SPDIF/digital output setup (normal) ...\n");

    /*   �f�W�^���o��: �f�t�H���g�ݒ� */
    sceSdSetCoreAttr (SD_C_SPDIF_MODE, (SD_SPDIF_MEDIA_CD |
					SD_SPDIF_OUT_PCM |
					SD_SPDIF_COPY_NORMAL));

    printf ("  CORE0 MEMIN: filling with 0 ...\n");

    /* CORE0 MEMIN: 0 �ŏ����� */
    memset (_0_buf, 0, 0x1000);

    /* 0x4000 - 0x4fff: CORE0 MEMIN */
    sceSdVoiceTrans (SD_CORE_0, (SD_TRANS_MODE_WRITE |
				 SD_TRANS_BY_DMA),
		     _0_buf, 0x4000, 0x1000);
    sceSdVoiceTransStatus (SD_CORE_0, SD_TRANS_STATUS_WAIT);

    printf ("  Set parameters ...\n");

#if WAVE_MODE == NORMAL_INTERLEAVE
    /* CORE1 external input volume */
    sceSdSetParam (SD_CORE_1 | SD_P_AVOLL, 0x7fff);
    sceSdSetParam (SD_CORE_1 | SD_P_AVOLR, 0x7fff);
    /* CORE0 sound data input volume */
    sceSdSetParam (SD_CORE_0 | SD_P_BVOLL, 0x7fff);
    sceSdSetParam (SD_CORE_0 | SD_P_BVOLR, 0x7fff);
    /* CORE1 sound data input volume */
    sceSdSetParam (SD_CORE_1 | SD_P_BVOLL, 0);
    sceSdSetParam (SD_CORE_1 | SD_P_BVOLR, 0);
    /* Master volume */
    sceSdSetParam (SD_CORE_0 | SD_P_MVOLL, 0x3fff);
    sceSdSetParam (SD_CORE_0 | SD_P_MVOLR, 0x3fff);
    sceSdSetParam (SD_CORE_1 | SD_P_MVOLL, 0x3fff);
    sceSdSetParam (SD_CORE_1 | SD_P_MVOLR, 0x3fff);
#elif WAVE_MODE == BYPASS_INTERLEAVE
    /* SAMPLE ONLY: CORE1 external input volume = 0 */
    sceSdSetParam (SD_CORE_1 | SD_P_AVOLL, 0);
    sceSdSetParam (SD_CORE_1 | SD_P_AVOLR, 0);
    /*      �K�{: CORE0 BVOL �� `�K��' 0 �ɐݒ� */
    sceSdSetParam (SD_CORE_0 | SD_P_BVOLL, 0);
    sceSdSetParam (SD_CORE_0 | SD_P_BVOLR, 0);
    /* SAMPLE ONLY: CORE1 sound data input volume = 0 */
    sceSdSetParam (SD_CORE_1 | SD_P_BVOLL, 0);
    sceSdSetParam (SD_CORE_1 | SD_P_BVOLR, 0);
    /* �T���v���̂�: �o�C�p�X�o�͂� MVOL �Ɉˑ����Ȃ� */
    sceSdSetParam (SD_CORE_0 | SD_P_MVOLL, 0);
    sceSdSetParam (SD_CORE_0 | SD_P_MVOLR, 0);
    sceSdSetParam (SD_CORE_1 | SD_P_MVOLL, 0);
    sceSdSetParam (SD_CORE_1 | SD_P_MVOLR, 0);
#endif
    /* �T�E���h�f�[�^�̈���m�� */
    CpuSuspendIntr (&oldei);
    /* �T���v���̂�: DTL-H10000 �� 2MB �ȏ�� IOP ������������ */
    addr = (unsigned char *) AllocSysMemory (0, WF_SIZE, NULL);
    CpuResumeIntr (oldei);
    if (addr == NULL) {
	printf ("[[ Error: Not allocate IOP memory ]]\n");
	return 0;
    }

    /* �T�E���h�f�[�^�̓ǂݍ��� */
    {
	int fd;
	int req = WF_FSIZE;
	int rsize = 0;
	int ret = 0;

	if ((fd = open (WF_FILE, O_RDONLY)) < 0) {
	    printf ("[[ Error: bad filename %s ]]\n", WF_FILE);
	    return (0);
	}
	while (rsize < WF_FSIZE) {
	    ret = read (fd, &addr [rsize], req);
	    rsize += ret;
	    if (ret < req) {
		req -= ret;
	    }
	}
	close (fd);
    }

    printf ("  SPDIF/digital output setup (bypass processing) ...\n");

#if WAVE_MODE == BYPASS_INTERLEAVE
    /* �f�W�^���o��: �o�C�p�X�w�� */
    sceSdSetCoreAttr (SD_C_SPDIF_MODE, (SD_SPDIF_MEDIA_CD |
					SD_SPDIF_OUT_BYPASS |
					SD_SPDIF_COPY_NORMAL));
#endif
    printf ("  Sound data transferring ...\n");

    /*     �K�{: �o�C�p�X������ CORE0 MEMIN �ł̂ݓ��� */
#define CHANNEL_ID SD_CORE_0

#ifdef TRANSFER_LOOP
    /* ���荞�݃n���h�� */
    sceSdSetTransIntrHandler (CHANNEL_ID,
			      (sceSdSpu2IntrHandler) transfer_intr,
			      (void *) &transfer_count);

    /* �]���J�n */
    transfer_count = 0;
    sceSdBlockTrans (CHANNEL_ID, (SD_TRANS_MODE_WRITE |
				  SD_BLOCK_LOOP), /* looping */
		     addr, WF_SIZE);

    /* 5 �񃋁[�v (1 ���[�v�����芄�荞�� 2 ��) ��҂� */
    while (transfer_count < 10) {
	DelayThread (100 * 1000); /* 0.1 sec */
    }
#else
    /* �]���J�n */
    sceSdBlockTrans (CHANNEL_ID, (SD_TRANS_MODE_WRITE |
				  SD_BLOCK_ONESHOT), /* one shot */
		     addr, WF_SIZE);

    /* �]���������I������܂ő҂� */
    while (sceSdBlockTransStatus (CHANNEL_ID, SD_TRANS_STATUS_WAIT) & 0xffffff) {
	DelayThread (100 * 1000); /* 0.1 sec */
    }
#endif

    /* �]�������̏I�� */
    sceSdBlockTrans (CHANNEL_ID, SD_TRANS_MODE_STOP, NULL, 0);

    printf ("  Bypass processing transferring ... done\n");

    /* �f�W�^���o��: �f�t�H���g�ݒ�ɖ߂� */
    sceSdSetCoreAttr (SD_C_SPDIF_MODE, (SD_SPDIF_MEDIA_CD |
					SD_SPDIF_OUT_PCM |
					SD_SPDIF_COPY_NORMAL));

#if WAVE_MODE == NORMAL_INTERLEAVE
    sceSdSetParam (SD_CORE_1 | SD_P_MVOLL, 0);
    sceSdSetParam (SD_CORE_1 | SD_P_MVOLR, 0);
#endif

    printf ("* SPU2 bypass processing sample (with libsd): finished\n");

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
