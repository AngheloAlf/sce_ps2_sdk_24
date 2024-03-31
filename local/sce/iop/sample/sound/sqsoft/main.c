/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *                          Version 1.20
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         sqhard - main.c
 *                      MIDI Sequencer Sample
 *
 *     Version   Date          Design     Log
 *  --------------------------------------------------------------------
 *     1.20      Nov.28.1999   morita     changed for LoadModule from EE
 *     0.60      Oct.14.1999   morita     first checked in.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <kernel.h>
#include <stdio.h>
#include <sif.h>
#include <csl.h>
#include <cslmidi.h>
#include <modmidi.h>
#include <modssyn.h>
#include <libsd.h>
#include <timerman.h>
#include <sys/file.h>

/* ----------------------------------------------------------------
 * �萔�}�N��/�ϐ�
 * ---------------------------------------------------------------- */
#define True 1

/* ----------------------------------------------------------------
 * modmidi �֘A
 * ---------------------------------------------------------------- */
// ���̓|�[�g
#define MIDI_IN_PORT 0

/* CSL context */
static sceCslCtx midiCtx;

/* CSL buffer group */
static sceCslBuffGrp midiGrp [2];   // ��ɗv�f���� 2 ... 0/����, 1/�o��
static sceCslBuffCtx mInBfCtx  [2]; // ���̓|�[�g�̑��� x 2
static sceCslBuffCtx mOutBfCtx [1]; // �o�̓|�[�g�̑���

/*
 * midi-stream buffer
 */
#define STREAMBUF_SIZE 1024
#define STREAMBUF_SIZE_ALL (sizeof (sceCslMidiStream) + STREAMBUF_SIZE) // �o�b�t�@ + ����

// �e���o�̓|�[�g���g���o�b�t�@
static char streamBf [STREAMBUF_SIZE+sizeof(sceCslMidiStream)];
// ���\����
static sceMidiEnv midiEnv;

/* ----------------------------------------------------------------
 * modhsyn �֘A
 * ---------------------------------------------------------------- */

/* CSL context */
static sceCslCtx synthCtx;

/* CSL buffer group */
static sceCslBuffGrp synthGrp;

/* CSL buffer context */
static sceCslBuffCtx sInBfCtx [2]; // ���̓|�[�g�̑��� x 2
static sceSSynEnv    synthEnv;     // ���̓|�[�g�̑���

char gFilename [64]; // SQ �t�@�C����

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

    iWakeupThread (tc->thread_id); // wakeup atick()

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

    return 0;
}

int
start_timer (TimerCtx* timer)
{
    if (StartHardTimer (timer->timer_id) != KE_OK) {
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
    if (FreeHardTimer (timer->timer_id) != KE_OK) {
	printf ("Can NOT free hard timer ...\n");
	return (-1);
    }

    return 0;
}

int
stop_timer (TimerCtx* timer)
{
    int ret;

    ret = StopHardTimer (timer->timer_id);
    if (! (ret == KE_OK || ret == KE_TIMER_NOT_INUSE)) {
	printf ("Can NOT stop hard timer ...\n");
	return (-1);
    }

    return 0;
}

/* ----------------------------------------------------------------
 * 1 tick ���̏���
 * ---------------------------------------------------------------- */
static void
atick (void)
{
    sceMidi_ATick (&midiCtx);
    sceSSyn_ATick (&synthCtx);
}


/* ----------------------------------------------------------------
 * CallBack �̗�
 * ---------------------------------------------------------------- */
/* ----------------
 * ���^�C�x���g��M
 * ---------------- */
int
callback_meta_msg (unsigned char metaNo, unsigned char *bf, unsigned int len, unsigned int private)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *)private;

#if 0
    printf ("META %02x\n", (int)metaNo);
#endif
    return True;
}

/* ----------------
 * �J��Ԃ�����
 * ---------------- */
int
callback_repeat (sceMidiLoopInfo *pInf, unsigned int private)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream*)private;

#if 0
    printf ("Repeat %s %d %d %x\n",
	    (pInf->type == sceMidiLoopInfoType_Midi) ? "Midi" : "Song",
	    (int)pInf->loopTimes, (int)pInf->loopCount, pInf->loopId);
#endif
    return True;
}

/* ----------------------------------------------------------------
 * �e Mudule Context �̐ݒ�
 * ---------------------------------------------------------------- */
void
set_module_context (void)
{
    /*
     * modmidi
     */
    // CSL context
    midiCtx.extmod     = NULL;
    midiCtx.callBack   = NULL;
    midiCtx.buffGrpNum = 2;	// modmidi �͏�� 2
    midiCtx.buffGrp    = midiGrp;
      // CSL buffer group: ����
      midiGrp [0].buffNum = 2;
      midiGrp [0].buffCtx = mInBfCtx;
	mInBfCtx [0].sema = 0;
	mInBfCtx [0].buff = NULL;
	mInBfCtx [1].sema = 0;
	mInBfCtx [1].buff = &midiEnv;
      // CSL buffer group: �o��
      midiGrp [1].buffNum = 1;
      midiGrp [1].buffCtx = mOutBfCtx;
	mOutBfCtx [0].sema = 0;
	mOutBfCtx [0].buff = streamBf;

    /*
     * modhsyn
     */
    // CSL context
    synthCtx.extmod     = NULL;
    synthCtx.callBack   = NULL;
    synthCtx.buffGrpNum = 1;	// modhsyn �͏�� 1
    synthCtx.buffGrp    = &synthGrp;
      // CSL buffer group: ����
      synthGrp.buffNum = 2;
      synthGrp.buffCtx = sInBfCtx;
        sInBfCtx [0].sema = 0;
	sInBfCtx [0].buff = streamBf; // modmidi �Ƌ��L
	sInBfCtx [1].sema = 0;
	sInBfCtx [1].buff = &synthEnv;

    /*
     * midi-stream buffer
     */
    ((sceCslMidiStream *)streamBf)->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslMidiStream *)streamBf)->validsize = 0;

    return;
}

/* ----------------------------------------------------------------
 * Software Synthesizer �Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
int
set_ssyn (void)
{
    // modssyn �S�̂̏�����
    if (sceSSyn_Init (&synthCtx, ONE_240TH) != sceSSynNoError) { // 1/240sec
	printf("sceSSyn_Init Error\n");
	return 1;
    }

    return 0;
}


/* ----------------------------------------------------------------
 * Midi Sequencer �Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
#define ALLOC_SIZE (128 * 1024)

int
set_midi (void)
{
    void* sq;
    int fd, i;
    int oldstat;

    /* SQ �t�@�C���̓ǂݍ���
     * ---------------------------------------------------------------- */
    CpuSuspendIntr (&oldstat);
    sq = AllocSysMemory (0, ALLOC_SIZE, NULL);
    CpuResumeIntr (oldstat);
    if ((fd = open (gFilename, O_RDONLY)) < 0) {
	printf ("file open failed. %s\n", gFilename);
	return -1;
    }
    if (read (fd, sq, ALLOC_SIZE) < 0){
	printf ("file read failed. %s\n", gFilename);
	return -1;
    }

    // modmidi �S�̂̏�����
    if (sceMidi_Init (&midiCtx, ONE_240TH) != sceMidiNoError) { // 1/240sec
	printf ("sceMidi_Init Error\n");
	return 1;
    }

    /* modmidi ���̓|�[�g 0 �̐ݒ�
     * ---------------------------------------------------------------- */

    /* SQ ����̓|�[�g 0 �ɓo�^ */
    midiCtx.buffGrp [0].buffCtx [0].buff = sq;
    if (sceMidi_Load (&midiCtx, MIDI_IN_PORT) != sceMidiNoError) {
	printf ("sceMidi_Load(%d) error\n", i);
	return 1;
    }

    /* ���̓|�[�g 0 �̐ݒ� */

    // ... �S�ẴR�[���o�b�N�p�f�[�^�̓o�b�t�@�ɒu�����
    midiEnv.chMsgCallBackPrivateData   = (unsigned int)streamBf;
    midiEnv.metaMsgCallBack            = callback_meta_msg;
    midiEnv.metaMsgCallBackPrivateData = (unsigned int)streamBf;
    midiEnv.repeatCallBack             = callback_repeat;
    midiEnv.repeatCallBackPrivateData  = (unsigned int)streamBf;
    midiEnv.excOutPort = 1 << 0; // �G�N�X�N���[�V�u�͏o�̓|�[�g 0 �ɏo��

    // ���̓|�[�g 0 �� MIDI �f�[�^�� MIDI ch �Ɋւ�炸�o�̓|�[�g 0 �ɏo��
    for (i = 0; i < sceMidiNumMidiCh; i++) {
	midiEnv.outPort [i] =  1 << 0;
    }

    // ���̓|�[�g 0 �̉��t�ΏۂƂ��āA�o�^���ꂽ SQ �̃u���b�N�ԍ� 0 ���w��
    if (sceMidi_SelectMidi (&midiCtx, MIDI_IN_PORT, 0) != sceMidiNoError) {
	printf("sceMidi_SelectMidi Error\n");
	return 1;
    }
    // ���̓|�[�g 0 �̉��t�J�n�ʒu��擪��
    if (sceMidi_MidiSetLocation (&midiCtx, MIDI_IN_PORT, 0) != sceMidiNoError) {
	printf("sceMidi_MidiSetLocation Error\n");
	return 1;
    }
    printf("LOCATION = %d\n", sceMidi_GetEnv(&midiCtx, 1)->position);
	
    /* ���t
     * ---------------------------------------------------------------- */

    if (sceMidi_MidiPlaySwitch (&midiCtx, MIDI_IN_PORT, sceMidi_MidiPlayStart)
	!= sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n");
	return 1;
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
	r_attr.mode = SD_REV_MODE_STUDIO_A | SD_REV_MODE_CLEAR_WA;
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

    // core0 out �� core1 ��WET���ɂ͓���Ȃ�
    sceSdSetParam (1 | SD_P_MMIX, ~(SD_MMIX_SINEL | SD_MMIX_SINER));

    return;
}

/*-------------------
  ���C��
  -------------------*/
int
my_main (void)
{
    TimerCtx timer;
    int count = 0;

    // initialize hardware
#if 0
    // SPU�̏�������EE����sceSdRemoteInit�ōs�Ȃ��Ă���
    sceSdInit (0);
#endif
    set_spu2 ();

    timer.thread_id = GetThreadId ();
    set_timer (&timer);

    // initialize modules
    set_module_context ();

    set_ssyn ();
    set_midi ();

    // timer start
    start_timer (&timer);

    // main loop
    while (sceMidi_isInPlay (&midiCtx, MIDI_IN_PORT)) {
	atick ();
	SleepThread (); // �d���̖������͐Q��
    }
	
    printf ("/////// count %d ///////", ++count);

    // timer stop
    stop_timer (&timer);
    clear_timer (&timer);

    printf ("EXIT............\n");

    return 0;
}

/*-------------------------
  main���X���b�h�Ƃ��č��B
  -------------------------*/
// sdrdrv.irx�Ɠ����ɓ��삷��̂ŁAsdrdrv.irx�̃v���C�I���e�B(���݂̂Ƃ�
//   ��34�E�ύX�̉\������)�����Ⴍ���Ă����K�v������B

int
start (int argc, char *argv [])
{
    struct ThreadParam param;
    int thid;

    strcpy (gFilename, argv [1]); // EE����arg���󂯎���Ă���
    printf ("filename = %s\n", gFilename);

    param.attr         = TH_C;
    param.entry        = my_main;
    param.initPriority = 36;     // sdrdrv.irx�����Ⴍ����
    param.stackSize    = 0x800;
    param.option       = 0;
    thid = CreateThread (&param);
    if (thid > 0) {
	StartThread (thid,0);
	return RESIDENT_END;	/* 0: program is resident */
    } else {
	return NO_RESIDENT_END;	/* 1: program is deleted */
    }
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
