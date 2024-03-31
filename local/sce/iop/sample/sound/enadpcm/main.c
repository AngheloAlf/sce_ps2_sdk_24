/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *              I/O Processor Library Sample Program
 * 
 *                          - enadpcm -
 * 
 *                           Shift-JIS
 * 
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                            main.c
 * 
 *     Version   Date          Design     Log
 *  --------------------------------------------------------------------
 */

#include <kernel.h>
#include <stdio.h>
#include <sys/file.h>
#include <libsd.h>
#include <spucodec.h>

/* ���p���� SPU2 �� DMA �`�����l�� */
#define DMA_CH 0

/* �����X���b�h�̃X���b�h�D��x */
#define BASE_priority  60

/* ���ɃG���R�[�h���ꂽ�g�`�f�[�^�̏�� */
#define VAG_DATA_SIZE	(229296 + 16) /* 64 �o�C�g�P�ʂɐ؂�グ */
#define VAG_FILENAME	"host1:/usr/local/sce/data/sound/wave/knot.vag"
#define VAG_HEADER_SIZE 48
#define VAG_ADDR	0x5010

/* �G���R�[�h�����̌��f�[�^�Ƃ��Ďg�p���� PCM �g�`�f�[�^�̏�� */
#define RAW_DATA_SIZE 802444
#define RAW_FILENAME	"host1:/usr/local/sce/data/sound/wave/knot_l.raw"
#define RAW_ADDR	(VAG_ADDR + VAG_DATA_SIZE)

/* �����Ɏg�p����{�C�X�̃{�C�X�ԍ� */
#define VOICE_VAG 0		/* ���ɃG���R�[�h���ꂽ�g�`�f�[�^�̔����p */
#define VOICE_RAW 1		/* �G���R�[�h�����ŏo�͂��ꂽ�g�`�f�[�^�̔����p */

/* �f�o�b�O�p */
#ifdef DEBUG
#define PRINTF(x) printf x
#else
#define PRINTF(x)
#endif

/* ---------------------------------------------------------------- 	*/
/*
 * set_data()		�̈���m�ۂ��A���̗̈�Ƀt�@�C���̓��e��ǂݍ���
 *	����:
 *		fname:	�ǂݍ��ރt�@�C���̖��O
 *		size:	�ǂݍ��ރt�@�C���̃T�C�Y
 *	�Ԃ�l:
 *		NULL      ... �ُ�I��
 *		NULL �ȊO ... �t�@�C���̓��e���ǂݍ��܂ꂽ�̈��
 *			      �擪�A�h���X
 */
/* ---------------------------------------------------------------- 	*/
char *
set_data (char *fname, int *size)
{
    int fd;
    char *buffer;
    int oldstat;

    printf ("    fname = [%s]\n", fname);

    /* �t�@�C�����J�� */
    if ((fd = open (fname, O_RDONLY)) < 0) {
	printf ("\nfile open failed. %s\n", fname);
	return NULL;
    };
    *size = lseek (fd, 0, 2);	/* �I�[�܂ňړ����ăt�@�C���T�C�Y�𓾂� */
    if (*size <= 0) {
	printf ("\nCan't load file to iop heap\n");
	return NULL;
    }
    lseek (fd, 0, 0);

    /* �t�@�C���̓��e��ǂݍ��ޗ̈���m�� */
    PRINTF (("allocate IOP heap memory - "));
    CpuSuspendIntr (&oldstat);
    buffer = AllocSysMemory (0, *size, NULL);
    CpuResumeIntr (oldstat);
    if (buffer == NULL) {
	printf ("\nCan't alloc heap \n");
	return NULL;
    }
    PRINTF (("alloced 0x%x  \n", (int)buffer));

    /* �t�@�C���̓��e��ǂݍ��� */
    read (fd, buffer, *size);

    /* �t�@�C������� */
    close (fd);

    return buffer;
}

/* ---------------------------------------------------------------- 	*/
/*
 * rawdata_encode_and_play()	PCM �f�[�^���G���R�[�h���ĉ��t
 *	����:	�Ȃ�
 *	�Ԃ�l:
 *		0 ... ����I��
 *	       -1 ... set_data() ���ُ�I�������ꍇ�A���邢��
 *		      �G���R�[�h���ʗp�̗̈悪�m�ۂł��Ȃ������ꍇ
 */
/* ---------------------------------------------------------------- 	*/
int
rawdata_encode_and_play (void)
{
    char *addr, *adpcm;
    int size;
    int encode_size;
    int oldstat;
    sceSpuEncodeEnv env;

    printf ("  Playing the encoded RAW data ...\n");

    /* PCM �g�`�f�[�^�̓ǂݍ��� */
    if ((addr = set_data (RAW_FILENAME, &size)) == NULL){
	return -1;
    }

    /* PCM �f�[�^�̃T�C�Y�̔����̗̈���m��
     *  ... PCM �f�[�^�̃G���R�[�h���ʂ͖� 1/3 �����A�����傫�߂� */ 
    CpuSuspendIntr (&oldstat);
    adpcm = AllocSysMemory (0, size / 2, NULL);
    CpuResumeIntr (oldstat);
    if (adpcm == NULL) {
	printf ("\nCan't alloc heap \n");
	return (-1);
    }

    /* �g�`�f�[�^�G���R�[�h�����̐ݒ� */
    env.src        = (short *) addr;
    env.dest       = (short *) adpcm;
    env.size       = RAW_DATA_SIZE;
    env.loop_start = 0;				    /* ���[�v�J�n�|�C���g */
    env.loop       = SPUCODEC_ENCODE_NO_LOOP;	    /* ���[�v���� */
    env.byte_swap  = SPUCODEC_ENCODE_ENDIAN_LITTLE; /* �G���f�B�A������ */
    env.proceed    = SPUCODEC_ENCODE_WHOLE;	    /* �S��G���R�[�h or �i���w�� */
    env.quality    = SPUCODEC_ENCODE_MIDDLE_QULITY; /* �������x�� */
    env.work       = (short *) 0; // NULL ... temporary reserved.

    /* �G���R�[�h */
    printf ("    encoding ...\n");
    encode_size = sceSpuCodecEncode (&env);
    printf ("    encoding ... finished: encoded data size = %d [0x%x]\n",
	    encode_size, encode_size);

    /* �G���R�[�h���ʂ� SPU2 ���[�J���������ɓ]�� */
    sceSdVoiceTrans (DMA_CH,
		     SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
		     (u_char *) adpcm,
		     RAW_ADDR, (u_int) encode_size);
    PRINTF (("    TSA = %x \n", sceSdGetAddr (DMA_CH | SD_A_TSA)));

    /* �]���I����҂� */
    PRINTF (("    Wait for transferring ...\n"));
    sceSdVoiceTransStatus (DMA_CH, SD_TRANS_STATUS_WAIT);
    PRINTF (("    Transfer is done ...\n"));

    /* �G���R�[�h���ʂ����ۂɉ��t */
    sceSdSetAddr   (SD_CORE_0 | (VOICE_RAW << 1) | SD_VA_SSA, RAW_ADDR);
    printf ("    and playing ...\n");
    sceSdSetSwitch (SD_CORE_0 | SD_S_KON,  1 << VOICE_RAW); /* ���� */

    /* 10 �b�҂� ... �g�`�f�[�^�����t�����܂� */
    DelayThread (10 * 1000 * 1000); // 10 sec.

    /* ���� */
    sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, 1 << VOICE_RAW);

    /* �G���R�[�h���ʂ̗̈����� */
    CpuSuspendIntr (&oldstat);
    FreeSysMemory (addr);
    CpuResumeIntr (oldstat);

    printf ("  Playing the encoded RAW data ... completed...\n");

    return 0;
}

/* ---------------------------------------------------------------- 	*/
/*
 * vag_play()	���O�ɃG���R�[�h���ꂽ�g�`�f�[�^�����t
 *	����:	�Ȃ�
 *	�Ԃ�l:
 *		0 ... ����I��
 *	       -1 ... set_data() ���ُ�I�������ꍇ
 */
/* ---------------------------------------------------------------- 	*/
int
vag_play (void)
{
    char *addr;
    int size;
    int oldstat;

    printf ("  Playing the normal VAG ...\n");

    /* �g�`�f�[�^�̓ǂݍ��� */
    if ((addr = set_data (VAG_FILENAME, &size)) == NULL){
	return -1;
    }

    PRINTF (("    Data transfer ...\n"));

    /* �g�`�f�[�^�� SPU2 ���[�J���������ɓ]�� */
    sceSdVoiceTrans (DMA_CH,
		     SD_TRANS_MODE_WRITE | SD_TRANS_BY_DMA,
		     (u_char *) addr,
		     VAG_ADDR, (u_int) VAG_DATA_SIZE);
    PRINTF (("    TSA = %x \n", sceSdGetAddr (DMA_CH | SD_A_TSA)));

    /* �]���I����҂� */
    PRINTF (("    Wait for transferring ...\n"));
    sceSdVoiceTransStatus (DMA_CH, SD_TRANS_STATUS_WAIT);
    PRINTF (("    Transfer is done ...\n"));

    /* �g�`�f�[�^�����t */
    sceSdSetAddr  (SD_CORE_0 | (VOICE_VAG << 1) | SD_VA_SSA, VAG_ADDR);
    sceSdSetSwitch (SD_CORE_0 | SD_S_KON,  1 << VOICE_VAG); /* ���� */

    /* 10 �b�҂� ... �g�`�f�[�^�����t�����܂� */
    DelayThread (10 * 1000 * 1000); // 10 sec.

    /* ���� */
    sceSdSetSwitch (SD_CORE_0 | SD_S_KOFF, 1 << VOICE_VAG);

    /* �g�`�f�[�^�̗̈����� */
    CpuSuspendIntr (&oldstat);
    FreeSysMemory (addr);
    CpuResumeIntr (oldstat);

    printf ("  Playing the normal VAG ... completed.\n");

    return 0;
}

/* ---------------------------------------------------------------- 	*/
/*
 * encode_test()	���C������
 *	����:	�Ȃ�
 *	�Ԃ�l:	�K�� 0
 */
/* ---------------------------------------------------------------- 	*/
int
encode_test (void)
{
    int core, v;
    int i;

    printf ("enadpcm start ================\n");

    /* �჌�x���T�E���h���C�u�����̏����� */
    sceSdInit (0);

    PRINTF (("  Set all attributes ...\n"));

    /*
     * SPU2 ��T�^�I�Ȓl�ŏ�����
     */
    for (i = 0; i < 2; i++) {
	sceSdSetParam (i | SD_P_MVOLL, 0x3fff);
	sceSdSetParam (i | SD_P_MVOLR, 0x3fff);
    }
    for (core = 0; core < 2; core ++) {
	for (v = 0; v < 24; v ++) {
	    sceSdSetParam (core | (v << 1) | SD_VP_VOLL, 0x3fff);
	    sceSdSetParam (core | (v << 1) | SD_VP_VOLR, 0x3fff);
	    sceSdSetParam (core | (v << 1) | SD_VP_PITCH, 0x1000);
	    sceSdSetParam (core | (v << 1) | SD_VP_ADSR1, 
			   SD_ADSR1 (SD_ADSR_A_LINEAR, 0, 0, 0xf));
	    sceSdSetParam (core | (v << 1) | SD_VP_ADSR2,
			   SD_ADSR2 (SD_ADSR_S_EXP_INC, 0, SD_ADSR_R_EXP, 0));
	}
    }

    /* �G�t�F�N�g�����͗��p���Ȃ� *//* effect disable */
    sceSdSetCoreAttr (SD_CORE_0 | SD_C_EFFECT_ENABLE, 0);

    /*
     *		---- ���C������ ----
     */

    /* �܂��A���O�ɃG���R�[�h���ꂽ�g�`�f�[�^�����t */
    vag_play ();

    /* ���ɁA�����g�`�f�[�^�� PCM �f�[�^���G���R�[�h�������ĉ��t */
    rawdata_encode_and_play ();

    printf ("enadpcm completed ============\n");

    return 0;
}

/* ----------------------------------------------------------------
 * start
 * ---------------------------------------------------------------- */

int
start (int argc, char *argv [])
{
    struct ThreadParam param;
    int	thid;

    /* �����X���b�h�̍쐬 *//* Create main thread */
    param.attr         = TH_C;
    param.entry        = encode_test;
    param.initPriority = BASE_priority;
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread(&param);
    if (thid > 0) {
	StartThread (thid, 0);
	/* �t�@�C���̓��e���S�ēǂݍ��܂��܂őҋ@ */
	DelayThread (25 * 1000 * 1000); /* for file service (dsifilesv) */
	return RESIDENT_END;	/* program is resident */
    } else {
	return NO_RESIDENT_END;	/* program is deleted */
    }
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
