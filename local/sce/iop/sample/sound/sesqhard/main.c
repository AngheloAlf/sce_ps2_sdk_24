/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *                          Version 2.0
 *                           Shift-JIS
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         sesqhard - main.c
 *                        SE Sequence Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.0       Nov.,2000     kaol     for release
 */

#include <kernel.h>
#include <stdio.h>

#include <libsd.h>
#include <csl.h>
#include <cslse.h>
#include <modsesq.h>
#include <modhsyn.h>

volatile int main_sema;		// �T�E���h������҂Z�}�t�H
sceHSyn_VoiceStat vstat;	// modhsyn ��ԃ��j�^�o�b�t�@

/* ----------------------------------------------------------------
 * �^�C�}�[
 * ---------------------------------------------------------------- */

// 4167 micro sec. = 1000 * 1000 / 240 = 1/240 sec
#define ONE_240TH 4167

typedef struct TimerCtx {
    int thread_id;
    int timer_id;
    int count;
} TimerCtx;

/* ----------------
 * ���荞�݃n���h��
 * ---------------- */
unsigned int
timer_handler (void *common)
{
    TimerCtx *tc = (TimerCtx *) common;

    iWakeupThread (tc->thread_id); // wakeup ATick()

    return (tc->count); // �V���Ȕ�r�l��ݒ肵�J�E���g�𑱍s
}

/* ----------------
 * �^�C�}�[�ݒ�
 * ---------------- */
int
set_timer (TimerCtx *timer)
{
    struct SysClock clock;
    int timer_id;

    // 1/240 sec = ??? sysclock
    USec2SysClock (ONE_240TH, & clock);
    timer->count = clock.low;	/* within 32-bit */

    // Use sysclock timer
    if ((timer_id = AllocHardTimer (TC_SYSCLOCK, 32, 1)) <= 0) {
	printf ("Can NOT allocate hard timer ...\n");
	return (-1);
    }
    timer->timer_id = timer_id;

    if (SetTimerHandler (timer_id, timer->count,
			 timer_handler, (void *) timer) != KE_OK) {
	printf ("Can NOT set timeup timer handler ...\n");
	return (-1);
    }

    if (SetupHardTimer (timer_id, TC_SYSCLOCK, TM_NO_GATE, 1) != KE_OK) {
	printf ("Can NOT setup hard timer ...\n");
	return (-1);
    }

    if (StartHardTimer (timer_id) != KE_OK) {
	printf ("Can NOT start hard timer ...\n");
	return (-1);
    }

    return 0;
}

/* ----------------
 * �^�C�}�[�폜
 * ---------------- */
int
clear_timer (TimerCtx *timer)
{
    int ret;

    ret = StopHardTimer (timer->timer_id);
    if (! (ret == KE_OK || ret == KE_TIMER_NOT_INUSE)) {
	printf ("Can NOT stop hard timer ...\n");
	return (-1);
    }

    if (FreeHardTimer (timer->timer_id) != KE_OK) {
	printf ("Can NOT free hard timer ...\n");
	return (-1);
    }
    return 0;
}

/* ----------------------------------------------------------------
 * �萔�}�N��/�ϐ�
 * ---------------------------------------------------------------- */
/* ----------------
 * �T�C�Y/�������z�u
 * ---------------- */
#define SQ_ADDR		0x101000
#define SQ_TOP		0
#define SQ_BLOCK0	0	// ���t�u���b�N�ԍ�: sakana.sq �ɂ� 1 �����Ȃ�

/* ----------------
 * �g�`�f�[�^���
 * ---------------- */
#define HD0_ADDRESS	((void *) 0x120000) // IOP ��
#define BD0_ADDRESS	((void *) 0x130000) // IOP ��
#define BD0_SIZE	(43008+0) // 64 �̔{����

// BD �� SPU2 ���[�J�����������擪�A�h���X
#define SPU_ADDR0	((void *) 0x5010)

/* ----------------
 * CSL �֘A
 * ---------------- */
#define   IN_BUF 0		// Buffer Group 0: ���� / ��(�I�v�V����)
#define  OUT_BUF 1		// Buffer Group 1: �o��
#define DATA_BUF 0		// context (N*2):   N �Ԃ̃o�b�t�@
#define  ENV_BUF 1		// context (N*2)+1: N �Ԃ̃o�b�t�@�ɑΉ������

/* ----------------
 * ���W���[���֘A
 * ---------------- */
/* CSL context */
static sceCslCtx sesqCtx;	// modsesq
static sceCslCtx synthCtx;	// modhsyn

/* CSL buffer group */
static sceCslBuffGrp sesqGrp [2]; // modsesq: ��ɗv�f���� 2 ... 0/����, 1/�o��
static sceCslBuffGrp synthGrp;	  // modhsyn: ��ɗv�f���� 1 ... 0/����

#define SESQ_INPUT_NUM 1
#define SESQ_OUTPUT_NUM 1
#define HSYN_INPUT_NUM 1

/* CSL buffer context */
static sceCslBuffCtx sqinBfCtx  [SESQ_INPUT_NUM * 2];	// modsesq: ���̓o�b�t�@
static sceCslBuffCtx sqoutBfCtx [SESQ_OUTPUT_NUM];	// modsesq: �o�̓|�[�g
static sceSESqEnv    sesqEnv    [SESQ_INPUT_NUM];	// modsesq: ���̓|�[�g��
static sceCslBuffCtx synthBfCtx [HSYN_INPUT_NUM * 2];	// modhsyn: ���̓o�b�t�@
static sceHSynEnv    synthEnv   [HSYN_INPUT_NUM];	// modhsyn: ���̓|�[�g��

/* SE stream buffer */
// �o�b�t�@���T�C�Y
#define STREAMBUF_SIZE 1024
// �S�̃T�C�Y: ���� + �o�b�t�@
#define STREAMBUF_SIZE_ALL (sizeof (sceCslSeStream) + STREAMBUF_SIZE)
// �e���o�̓|�[�g���g���o�b�t�@
static char streamBf [SESQ_OUTPUT_NUM][STREAMBUF_SIZE_ALL];

/* ----------------------------------------------------------------
 * �e Mudule Context �̐ݒ�
 * ---------------------------------------------------------------- */
void
set_module_context (void)
{
    /*
     * modsesq
     */
    // CSL context
    sesqCtx.extmod     = NULL;
    sesqCtx.callBack   = NULL;
    sesqCtx.buffGrpNum = 2;	// modsesq �͏�� 2
    sesqCtx.buffGrp    = sesqGrp;
      // CSL buffer group: ����
      sesqGrp [ IN_BUF].buffNum = SESQ_INPUT_NUM * 2;	// ���̓|�[�g�� x 2
      sesqGrp [ IN_BUF].buffCtx = sqinBfCtx;
        // CSL buffer context
        sqinBfCtx  [0 * 2 + DATA_BUF].sema = 0;
	sqinBfCtx  [0 * 2 + DATA_BUF].buff = NULL;	// set_sesq() �ɂĐݒ�
	sqinBfCtx  [0 * 2 +  ENV_BUF].sema = 0;
	sqinBfCtx  [0 * 2 +  ENV_BUF].buff = sesqEnv;
      // CSL buffer group: �o��
      sesqGrp [OUT_BUF].buffNum = SESQ_OUTPUT_NUM;	// �o�̓|�[�g��
      sesqGrp [OUT_BUF].buffCtx = sqoutBfCtx;
        // �o�̓|�[�g 0 ... �S�Ẵ��b�Z�[�W�͂����ɏo��
	sqoutBfCtx [0].sema = 0;
	sqoutBfCtx [0].buff = &(streamBf [0]); // modhsyn �Ƌ��L

    /*
     * modhsyn
     */
    // CSL context
    synthCtx.extmod   	= NULL;
    synthCtx.callBack 	= NULL;
    synthCtx.buffGrpNum = 1;	// modhsyn �͏�� 1
    synthCtx.buffGrp    = &synthGrp;
      // CSL buffer group: ����
      synthGrp.buffNum = HSYN_INPUT_NUM * 2;		// ���̓|�[�g�� x 2
      synthGrp.buffCtx = synthBfCtx;
        // modsesq �p
        synthBfCtx [0 * 2 + DATA_BUF].sema = 0;
	synthBfCtx [0 * 2 + DATA_BUF].buff = &(streamBf [0]); // modsesq �Ƌ��L
	synthBfCtx [0 * 2 +  ENV_BUF].sema = 0;
	synthBfCtx [0 * 2 +  ENV_BUF].buff = &(synthEnv [0]);

    /*
     * SE Sequence stream buffer
     */
    ((sceCslSeStream *) &(streamBf [0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslSeStream *) &(streamBf [0]))->validsize = 0;

    return;
}

/* ----------------------------------------------------------------
 * Hardware Synthesizer �Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
int
set_hsyn (sceHSyn_VoiceStat* pVstat)
{
    // modhsyn �S�̂̏�����
    if (sceHSyn_Init (&synthCtx, ONE_240TH) != sceHSynNoError) { // 1/240sec
	printf ("sceHSyn_Init Error\n");
	return (-1);
    }

    /* ���̓|�[�g�� HD/BD �̐ݒ�
     * ---------------------------------------------------------------- */

    // modhsyn ���̓|�[�g�̐ݒ�
    synthEnv [0].priority       = 0;
    synthEnv [0].portMode       = sceHSynModeSESyn;	// SE ���[�h
    synthEnv [0].waveType       = sceHSynTypeTimbre;	// Timbre Chunk �̎w��
    synthEnv [0].lfoWaveNum 	= 0;
    synthEnv [0].lfoWaveTbl 	= NULL;
    synthEnv [0].velocityMapNum = 0;
    synthEnv [0].velocityMapTbl = NULL;

    // BD0 �� SPU2 ���[�J���������ɓ]��
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD0_ADDRESS, SPU_ADDR0, BD0_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n");
	return (-1);
    }

    // HD0 �� BD0 ���u���̓|�[�g 0 �̃o���N�ԍ� 0�v�Ƃ��ēo�^
    if (sceHSyn_Load (&synthCtx, 0, SPU_ADDR0, HD0_ADDRESS, 0) != sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", 0);
	return (-1);
    }

    // modhsyn �̏�ԃ��j�^�o�b�t�@��ݒ�
    sceHSyn_SetVoiceStatBuffer (pVstat);

    // SE �{�C�X�g�p��
    sceHSyn_SESetMaxVoices (12);

    return 0;
}

/* ----------------------------------------------------------------
 * SE Sequencer �Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
int
set_sesq (void)
{
    int ret;

    // modsesq �S�̂̏�����
    if ((ret = sceSESq_Init (&sesqCtx, ONE_240TH)) != sceSESqNoError) { // 1/240sec
	printf ("sceSESq_Init Error\n");
	return (-1);
    }

    /* modsesq ���̓|�[�g�̐ݒ�
     * ---------------------------------------------------------------- */

    /* SQ ����̓|�[�g 0 �ɓo�^ */
    // sesqCtx.buffGrp [IN_BUF].buffCtx [0 * 2 + DATA_BUF].buff = (void *) SQ_ADDR;
    sqinBfCtx  [0 * 2 + DATA_BUF].buff = (void *) SQ_ADDR;
    if (sceSESq_Load (&sesqCtx, 0) != sceSESqNoError) {
	printf ("sceSESq_Load (%d) error\n", 0);
	return (-1);
    }

    /* �o�̓|�[�g�̎w�� */
    // �f�t�H���g�̏o�̓|�[�g
    sesqEnv [0].defaultOutPort = 0;
    // outPort [0].port ���ȉ��̒l�̏ꍇ�A���蓖�Ă𒲍����Ȃ�
    sesqEnv [0].outPort [0].port = sceSESqEnv_NoOutPortAssignment;

    /* ��{�����̐ݒ� */
    sesqEnv [0].masterVolume    = sceSESq_Volume0db;
    sesqEnv [0].masterPanpot    = sceSESq_PanpotCenter;
    sesqEnv [0].masterTimeScale = sceSESq_BaseTimeScale;

    return 0;
}

int
play_sq (int *sesq_id, int set_no, int seq_no)
{

    // ���̓|�[�g 0 �̉��t�ΏۂƂ��āA�o�^���ꂽ SQ �� SE Sequence Set 
    // �ԍ��� SE Sequence �ԍ����w��
    if ((*sesq_id = sceSESq_SelectSeq (&sesqCtx, 0, set_no, seq_no))
	== sceSESqError) {
	printf ("sceSESq_SelectSeq Error\n");
	return (-1);
    }

    /* ���t
     * ---------------------------------------------------------------- */
    // ���̓|�[�g 0
    if (sceSESq_SeqPlaySwitch (&sesqCtx, 0, *sesq_id, sceSESq_SeqPlayStart)
	!= sceSESqNoError) {
	printf ("sceSESq_SeqPlaySwitch Error\n");
	return (-1);
    }

    return 0;
}

int
stop_sq (int *sesq_id)
{
    if (sceSESq_SeqPlaySwitch (&sesqCtx, 0, *sesq_id, sceSESq_SeqPlayStop)
	!= sceSESqNoError) {
	printf ("sceSESq_SeqPlaySwitch Error\n");
	return (-1);
    }

    if (sceSESq_UnselectSeq (&sesqCtx, 0, *sesq_id) != sceSESqNoError) {
	printf ("sceSESq_UnselectSeq Error\n");
	return (-1);
    }

    return 0;
}

/* ----------------------------------------------------------------
 * 1 tick ���̏���
 * ---------------------------------------------------------------- */
static int
atick (void)
{
    int count;
    int sesq_id;

    count = 0;
    while (1) {
	SleepThread ();

	// Tick ����
	sceSESq_ATick (&sesqCtx);	// modsesq
	sceHSyn_ATick (&synthCtx);	// modhsyn

	count ++;

	switch (count) {
	case 1:
	    printf ("Play --- Alarm whistle\n");
	    play_sq (&sesq_id, 0, 0);
	    break;
	case 400:
	    printf ("Stop --- Alarm whistle\n");
	    stop_sq (&sesq_id);
	    break;

	case 500:
	    printf ("Play --- Clash!\n");
	    play_sq (&sesq_id, 0, 1);
	    break;
	case 900:
	    printf ("Stop --- Clash!\n");
	    stop_sq (&sesq_id);
	    break;

	case 1000:
	    printf ("Play --- Alarm whistle .. (0.5 sec.) .. Clash!\n");
	    play_sq (&sesq_id, 0, 2);
	    break;
	case 1400:
	    printf ("Stop --- Alarm whistle .. (0.5 sec.) .. Clash!\n");
	    stop_sq (&sesq_id);
	    break;

	case 1500:
	    printf ("Play --- (blank: 1 sec.) .. Alarm whistle .. (0.5 sec.) .. Clash!\n");
	    play_sq (&sesq_id, 0, 3);
	    break;
	case 2200:
	    printf ("Stop --- (blank: 1 sec.) .. Alarm whistle .. (0.5 sec.) .. Clash!\n");
	    stop_sq (&sesq_id);
	    break;

	case 2300:
	    printf ("Play --- strings (loop) .. (2 sec.) .. Off\n");
	    play_sq (&sesq_id, 1, 0);
	    break;
	case 3000:
	    printf ("Stop --- strings (loop) .. (2 sec.) .. Off\n");
	    stop_sq (&sesq_id);
	    break;

	case 3100:
	    printf ("Play --- strings (loop) .. (up / 2 sec. .. down / 2 sec.) .. (1 sec.) .. Off\n");
	    play_sq (&sesq_id, 1, 1);
	    break;
	case 4500:
	    printf ("Stop --- strings (loop) .. (up / 2 sec. .. down / 2 sec.) .. (1 sec.) .. Off\n");
	    stop_sq (&sesq_id);
	    break;
	}
	    
#if 0
	if ((count % 240) == 0) {
	    printf (".");
	}
#endif
	if (count > 4600) {
	    break;		/* ���t�I�� */
	}
    }
    SignalSema (main_sema); // start �ɏ�����Ԃ�

    return 0;
}

/* ----------------------------------------------------------------
 * SPU2 �ݒ�
 * ---------------------------------------------------------------- */
void
set_spu2 (void)
{
    int i;
    sceSdEffectAttr r_attr;

    // set digital output mode
    sceSdSetCoreAttr (SD_C_SPDIF_MODE, (SD_SPDIF_MEDIA_CD |
					SD_SPDIF_OUT_PCM  |
					SD_SPDIF_COPY_NORMAL));

    for (i = 0; i < 2; i++) {
	/*
	 * �G�t�F�N�g�ݒ�
	 */
	// effect workarea end address �� CORE0 �� CORE1 �Ƃŕʂ̗̈�ɐݒ�
	sceSdSetAddr (i | SD_A_EEA, 0x1fffff - 0x20000 * i);

	// set reverb attribute
	r_attr.depth_L  = 0;
	r_attr.depth_R  = 0;
	r_attr.mode = SD_REV_MODE_HALL | SD_REV_MODE_CLEAR_WA;
	sceSdSetEffectAttr (i, &r_attr);

	// reverb on
	sceSdSetCoreAttr (i | SD_C_EFFECT_ENABLE, 1);
	sceSdSetParam    (i | SD_P_EVOLL, 0x3fff);
	sceSdSetParam    (i | SD_P_EVOLR, 0x3fff);

	/*
	 * �}�X�^�[�{�����[���ݒ�
	 */
	sceSdSetParam (i | SD_P_MVOLL, 0x3fff);
	sceSdSetParam (i | SD_P_MVOLR, 0x3fff);
    }

    return;
}

/* ----------------------------------------------------------------
 * �����I���҂��Z�}�t�H�̍쐬
 * ---------------------------------------------------------------- */
int
make_semaphore (void)
{
    struct SemaParam sema;

    sema.initCount = 0;
    sema.maxCount  = 1;
    sema.attr      = AT_THFIFO;

    /* �Z�}�t�H�쐬 */
    return (CreateSema (&sema));
}

/* ----------------------------------------------------------------
 * tick �����p�X���b�h�̍쐬
 * ---------------------------------------------------------------- */
int
make_thread (void)
{
    struct ThreadParam param;

    param.attr         = TH_C;
    param.entry        = atick;
    param.initPriority = 32 - 3;
    param.stackSize    = 0x800;
    param.option       = 0;

    /* �X���b�h�쐬 */
    return (CreateThread (&param));
}

/* ----------------------------------------------------------------
 * ���C������
 * ---------------------------------------------------------------- */
int
start (int argc, char *argv [])
{
    int thid;
    TimerCtx timer;

    // initialize hardware
    sceSdInit (0);
    set_spu2 ();		// Digital effect/master volume

    // initialize modules environment
    set_module_context ();	// CSL module context
    set_hsyn (&vstat);		// modhsyn
    set_sesq ();		// modsesq

    // semaphore: for finishing the play
    main_sema = make_semaphore ();

    // Thread: ATick() process
    thid = make_thread ();
    StartThread (thid, (unsigned long) NULL);

    // set timer
    timer.thread_id = thid;
    if (set_timer (&timer) < 0) {
	printf ("set_timer: something wrong ...");
    }

    // waiting for finishing the play
    WaitSema (main_sema);

    // timer disable
    if (clear_timer (&timer) < 0) {
	printf ("clearTimer: something wrong ...");
    }

    printf ("Fine ............\n");

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
