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
 *                         sebasic - main.c
 *                        SE Stream Basic Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.0       Sep.??,2000   kaol     for release
 */

#include <kernel.h>
#include <stdio.h>

#include <libsd.h>
#include <csl.h>
#include <cslse.h>
#include <modhsyn.h>

#include <modsein.h>

#define USE_MAKEMSG

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
// �g�`�f�[�^���
#define HD0_ADDRESS	((void *) 0x120000)
#define BD0_ADDRESS	((void *) 0x130000)
#define BD0_SIZE	(127536+16) // 64 �̔{����

// BD �� SPU2 ���[�J�����������擪�A�h���X
#define SPU_ADDR0	((void *) 0x5010)

/* ----------------------------------------------------------------
 * CSL �֘A
 * ---------------------------------------------------------------- */
// CSL buffer group �w��
#define   IN_BUF 0		// Buffer Group 0: ���� / ��(�I�v�V����)
#define  OUT_BUF 1		// Buffer Group 1: �o��
// CSL buffer context �w��
#define DATA_BUF 0		// context (N*2):   N �Ԃ̃o�b�t�@
#define  ENV_BUF 1		// context (N*2)+1: N �Ԃ̃o�b�t�@�ɑΉ������

/* ----------------------------------------------------------------
 * modsein �֘A
 * ---------------------------------------------------------------- */
// �o�̓|�[�g
#define OUT_PORT_0   0		// �S�Ă� SE �X�g���[���͂����ɏo��
#define OUT_PORT_NUM 1		// �o�̓|�[�g�̑���

/* CSL context */
static sceCslCtx seinCtx;

/* CSL buffer group */
static sceCslBuffGrp seinGrp [2]; // ��ɗv�f���� 2 ... 0/����, 1/�o��

/* CSL buffer context */
static sceCslBuffCtx seinBfCtx [OUT_PORT_NUM];// �o�̓|�[�g�̑���

/*
 * SE stream buffer
 */
#define STREAMBUF_SIZE 1024	// �o�b�t�@�T�C�Y
#define STREAMBUF_SIZE_ALL (sizeof (sceCslSeStream) + STREAMBUF_SIZE) // ���� + �o�b�t�@

// �e���o�̓|�[�g���g���o�b�t�@: ���̓|�[�g�̑���
static char streamBf [OUT_PORT_NUM][STREAMBUF_SIZE_ALL];

/* ----------------------------------------------------------------
 * modhsyn �֘A
 * ---------------------------------------------------------------- */
// ���̓|�[�g
#define HS_PORT_0   0		// OUT_PORT_0 �̏o�͂����
#define HS_PORT_NUM 1		// ���̓|�[�g�̑���
#define HS_BANK_0   15		// HD0/BD0 �� HS_PORT_0 �ł̃o���N�ԍ�

/* CSL context */
static sceCslCtx synthCtx;

/* CSL buffer group */
static sceCslBuffGrp synthGrp;

/* CSL buffer context */
static sceCslBuffCtx sInBfCtx [HS_PORT_NUM * 2]; // ���̓|�[�g�̑��� x 2
static sceHSynEnv    synthEnv [HS_PORT_NUM];     // ���̓|�[�g�̑���

/* ----------------------------------------------------------------
 * �e Mudule Context �̐ݒ�
 * ---------------------------------------------------------------- */
void
set_module_context (void)
{
    /*
     * modsein
     */
    // CSL context
    seinCtx.extmod     = NULL;
    seinCtx.callBack   = NULL;
    seinCtx.buffGrpNum = 2;	// modsein �͏�� 2
    seinCtx.buffGrp    = seinGrp;
      // CSL buffer group: ����
      seinGrp [ IN_BUF].buffNum = 0;
      seinGrp [ IN_BUF].buffCtx = NULL;
      // CSL buffer group: �o��
      seinGrp [OUT_BUF].buffNum = OUT_PORT_NUM;	// �o�̓|�[�g��
      seinGrp [OUT_BUF].buffCtx = seinBfCtx;
        // �o�̓|�[�g 0
	seinBfCtx [OUT_PORT_0].sema = 0;
	seinBfCtx [OUT_PORT_0].buff = &(streamBf [OUT_PORT_0]); // modhsyn �Ƌ��L

    /*
     * modhsyn
     */
    // CSL context
    synthCtx.extmod   	= NULL;
    synthCtx.callBack 	= NULL;
    synthCtx.buffGrpNum = 1;	// modhsyn �͏�� 1
    synthCtx.buffGrp    = &synthGrp;
      // CSL buffer group: ����
      synthGrp.buffNum = HS_PORT_NUM * 2;	// ���̓|�[�g�� x 2
      synthGrp.buffCtx = sInBfCtx;
        // OUT_PORT_0 �p
        sInBfCtx [HS_PORT_0 * 2 + DATA_BUF].sema = 0;
	sInBfCtx [HS_PORT_0 * 2 + DATA_BUF].buff = &(streamBf [OUT_PORT_0]); // modmidi�Ƌ��L
	sInBfCtx [HS_PORT_0 * 2 +  ENV_BUF].sema = 0;
	sInBfCtx [HS_PORT_0 * 2 +  ENV_BUF].buff = &(synthEnv [HS_PORT_0]);

    /*
     * SE-stream buffer
     */
    ((sceCslSeStream *) &(streamBf [OUT_PORT_0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslSeStream *) &(streamBf [OUT_PORT_0]))->validsize = 0;

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

    /* ���̓|�[�g 0 �� HD0/BD0 �̐ݒ�
     * ---------------------------------------------------------------- */

    // modhsyn ���̓|�[�g 0 �̐ݒ�
    synthEnv [HS_PORT_0].priority       = 0;
    synthEnv [HS_PORT_0].portMode       = sceHSynModeSESyn; // �|�[�g�� SE ���[�h
    synthEnv [HS_PORT_0].waveType       = sceHSynTypeHSyn;  // �g�`�f�[�^�͏]���ʂ�
    synthEnv [HS_PORT_0].lfoWaveNum 	= 0;
    synthEnv [HS_PORT_0].lfoWaveTbl 	= NULL;
    synthEnv [HS_PORT_0].velocityMapNum = 0;
    synthEnv [HS_PORT_0].velocityMapTbl = NULL;

    // BD0 �� SPU2 ���[�J���������ɓ]��
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD0_ADDRESS, SPU_ADDR0, BD0_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n");
	return (-1);
    }

    // HD0 �� BD0 ���u���̓|�[�g 0 �̃o���N�ԍ� HS_BANK_0 (= 0)�v�Ƃ��ēo�^
    if (sceHSyn_Load (&synthCtx, HS_PORT_0, SPU_ADDR0, HD0_ADDRESS, HS_BANK_0)
	!= sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", HS_PORT_0);
	return (-1);
    }

    // modhsyn �̏�ԃ��j�^�o�b�t�@��ݒ�
    sceHSyn_SetVoiceStatBuffer (pVstat);

    return 0;
}

/* ----------------------------------------------------------------
 * SE Stream input �Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
int
set_sein (void)
{
    // modmidi �S�̂̏�����
    if (sceSEIn_Init (&seinCtx) != sceSEInNoError) {
	printf ("sceSEIn_Init Error\n");
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
    unsigned int count;
    int finalize;

    count = finalize = 0;
    while (1) {
	SleepThread ();

	switch (count) {
	case 100:
	    printf ("Note on --- Comedy.\n");
	    //                              port, id,      bank, prog, note, vel, pan
#ifdef USE_MAKEMSG
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223344, HS_BANK_0, 0, 48, 0x7f, 64); // ver. 1
#else
	    sceSEIn_NoteOn     (&seinCtx, 0, 0x11223344, HS_BANK_0, 0, 48, 0x7f, 64); // ver. 1
#endif
	    break;
	case 500:
	    printf ("Note on --- Cheers!\n");
	    //                              port, id,      bank, prog, note, vel, pan
#ifdef USE_MAKEMSG
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223345, HS_BANK_0, 0, 72, 0x7f, 64); // ver. 1
#else
	    sceSEIn_NoteOn     (&seinCtx, 0, 0x11223345, HS_BANK_0, 0, 72, 0x7f, 64); // ver. 1
#endif
	    break;
	case 1000:
	    printf ("Note on --- Old telephone.\n");
	    //                               port, id,      bank, prog, note, vel, pan, SPU pitch
#ifdef USE_MAKEMSG
	    sceSEIn_MakePitchOn (&seinCtx, 0, 0x11223346, HS_BANK_0, 0, 96, 0x7f, 64, 0x3fff);
#else
	    sceSEIn_PitchOn     (&seinCtx, 0, 0x11223346, HS_BANK_0, 0, 96, 0x7f, 64, 0x3fff);
#endif
	    break;
	case 2000:
	    printf ("Note off.\n");
#ifdef USE_MAKEMSG
	    //                            port, id,      bank, prog, note, v, pan
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223344, HS_BANK_0, 0, 48, 0, 0);
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223345, HS_BANK_0, 0, 72, 0, 0);
	    sceSEIn_MakeNoteOn (&seinCtx, 0, 0x11223346, HS_BANK_0, 0, 96, 0, 0);
#else
	    //                         port, id,      bank, prog, note
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223344, HS_BANK_0, 0, 48);
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223345, HS_BANK_0, 0, 72);
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223346, HS_BANK_0, 0, 96);
#endif
	    break;
	}

	if (count > 2000) {
	    finalize ++;
	}

	// Tick ����
	sceSEIn_ATick (&seinCtx);	// modsein
	sceHSyn_ATick (&synthCtx);	// modhsyn

	if (finalize) {
	    // �����{�C�X�̊m�F
	    if (! (vstat.pendingVoiceCount || vstat.workVoiceCount)) {
		// ��������Ă���{�C�X����
		SignalSema (main_sema); // start() �ɏ�����Ԃ�
	    }
	}

	count ++;
    }

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

    // initialize SPU2 hardware
    sceSdInit (0);
    set_spu2 ();		// Digital effect/master volume

    // initialize modules environment
    set_module_context ();	// CSL module context
    set_hsyn (&vstat);		// modhsyn
    set_sein ();		// modsein

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
	printf ("clear_timer: something wrong ...");
    }

    printf ("Fine ............\n");

    return 0;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
