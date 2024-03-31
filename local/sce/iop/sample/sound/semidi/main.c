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
 *                         semidply - main.c
 *                      SE & MIDI Sequence Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.0       Sep.??,2000   kaol     for release
 */

#include <kernel.h>
#include <stdio.h>
#include <libsd.h>
#include <csl.h>
#include <cslmidi.h>
#include <cslse.h>
#include <modmidi.h>
#include <modhsyn.h>

// #define sceSEInCommandVersion_0
#include <modsein.h>

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
 *  CallBack�̗�
 * ---------------------------------------------------------------- */
/* ----------------
 * Meta Event
 * ---------------- */
int
metaMsgCB (unsigned char metaNo, unsigned char *bf, unsigned int len, unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;
#if 0
    printf ("META %02x\n", (int) metaNo);
#endif
    return True;
}

/* ----------------
 * Repeat
 * ---------------- */
int
repeatCB (sceMidiLoopInfo *pInf, unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;
#if 0
    printf ("Repeat %s %d %d %x\n",
	    (pInf->type == sceMidiLoopInfoType_Midi) ? "Midi" : "Song",
	    (int) pInf->loopTimes, (int) pInf->loopCount, pInf->loopId);
#endif
    return True;
}

/* ----------------------------------------------------------------
 * �萔�}�N��/�ϐ�
 * ---------------------------------------------------------------- */
#define SQ_ADDR		0x101000
#define SQ_TOP		0
#define SQ_BLOCK0	0	// ���t�u���b�N�ԍ�: sakana.sq �ɂ� 1 �����Ȃ�

/* ----------------
 * �g�`�f�[�^���
 * ---------------- */
#define HD0_ADDRESS	((void *) 0x120000) // IOP ��
#define BD0_ADDRESS	((void *) 0x130000) // IOP ��
#define BD0_SIZE	(335520+32) // 64 �̔{����
#define HD1_ADDRESS	((void *) 0x128000) // IOP ��
#define BD1_ADDRESS	((void *) 0x190000) // IOP ��
#define BD1_SIZE	(127536+16) // 64 �̔{����
// BD �� SPU2 ���[�J�����������擪�A�h���X
#define SPU_ADDR0	((void *) 0x5010)
#define SPU_ADDR1	((void *) (0x5010 + BD0_SIZE))

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
static sceCslCtx midiCtx;	// modmidi
static sceCslCtx seinCtx;	// modsein
static sceCslCtx synthCtx;	// modhsyn

/* CSL buffer group */
static sceCslBuffGrp midiGrp [2]; // modmidi: ��ɗv�f���� 2 ... 0/����, 1/�o��
static sceCslBuffGrp seinGrp [2]; // modsein: ��ɗv�f���� 2 ... 0/����, 1/�o��
static sceCslBuffGrp synthGrp;	  // modhsyn: ��ɗv�f���� 1 ... 0/����

/* CSL buffer context */
static sceCslBuffCtx minBfCtx   [1 * 2];	// modmidi: ���̓o�b�t�@
static sceCslBuffCtx moutBfCtx  [1];		// modmidi: �o�̓|�[�g
static sceMidiEnv    midiEnv    [1];		// modmidi: ���̓|�[�g��
static sceCslBuffCtx seinBfCtx  [1];		// modsein: �o�̓|�[�g
static sceCslBuffCtx synthBfCtx [2 * 2];	// modhsyn: ���̓o�b�t�@
static sceHSynEnv    synthEnv   [2];		// modhsyn: ���̓|�[�g��

/* SE stream buffer */
// �o�b�t�@���T�C�Y
#define STREAMBUF_SIZE 1024
// �S�̃T�C�Y: ���� + �o�b�t�@
#define STREAMBUF_SIZE_ALL (sizeof (sceCslSeStream) + STREAMBUF_SIZE)
// �e���o�̓|�[�g���g���o�b�t�@
static char streamBf [2][STREAMBUF_SIZE_ALL];

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
      midiGrp [ IN_BUF].buffNum = 1 * 2;	// ���̓|�[�g�� x 2
      midiGrp [ IN_BUF].buffCtx = minBfCtx;
        // CSL buffer context
        minBfCtx  [0 * 2 + DATA_BUF].sema = 0;
	minBfCtx  [0 * 2 + DATA_BUF].buff = NULL; // set_midi() �ɂĐݒ�
	minBfCtx  [0 * 2 +  ENV_BUF].sema = 0;
	minBfCtx  [0 * 2 +  ENV_BUF].buff = midiEnv;
      // CSL buffer group: �o��
      midiGrp [OUT_BUF].buffNum = 1;		// �o�̓|�[�g��
      midiGrp [OUT_BUF].buffCtx = moutBfCtx;
        // �o�̓|�[�g 0 ... SQ0 �̑S�Ă� MIDI ch �͂����ɏo��
	moutBfCtx [0].sema = 0;
	moutBfCtx [0].buff = &(streamBf [0]); // modhsyn �Ƌ��L

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
      seinGrp [OUT_BUF].buffNum = 1;	// �o�̓|�[�g��
      seinGrp [OUT_BUF].buffCtx = seinBfCtx;
        // �o�̓|�[�g 0
	seinBfCtx [0].sema = 0;
	seinBfCtx [0].buff = &(streamBf [1]); // modhsyn �Ƌ��L

    /*
     * modhsyn
     */
    // CSL context
    synthCtx.extmod   	= NULL;
    synthCtx.callBack 	= NULL;
    synthCtx.buffGrpNum = 1;	// modhsyn �͏�� 1
    synthCtx.buffGrp    = &synthGrp;
      // CSL buffer group: ����
      synthGrp.buffNum = 2 * 2;	// ���̓|�[�g�� x 2
      synthGrp.buffCtx = synthBfCtx;
        // modmidi �p
        synthBfCtx [0 * 2 + DATA_BUF].sema = 0;
	synthBfCtx [0 * 2 + DATA_BUF].buff = &(streamBf [0]); // modmidi �Ƌ��L
	synthBfCtx [0 * 2 +  ENV_BUF].sema = 0;
	synthBfCtx [0 * 2 +  ENV_BUF].buff = &(synthEnv [0]);
        // modsein �p
        synthBfCtx [1 * 2 + DATA_BUF].sema = 0;
	synthBfCtx [1 * 2 + DATA_BUF].buff = &(streamBf [1]); // modsein �Ƌ��L
	synthBfCtx [1 * 2 +  ENV_BUF].sema = 0;
	synthBfCtx [1 * 2 +  ENV_BUF].buff = &(synthEnv [1]);

    /*
     * MIDI/SE stream buffer
     */
    ((sceCslSeStream *) &(streamBf [0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslSeStream *) &(streamBf [0]))->validsize = 0;
    ((sceCslSeStream *) &(streamBf [1]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslSeStream *) &(streamBf [1]))->validsize = 0;

    return;
}

/* ----------------------------------------------------------------
 * Midi Sequencer�Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
int
set_midi (void)
{
    int i;

    // modmidi �S�̂̏�����
    if (sceMidi_Init (&midiCtx, 4167) != sceMidiNoError) { // 4167 = 1/240sec
	printf ("sceMidi_Init Error\n"); return (-1);
    }

    /* modmidi ���̓|�[�g 0 �̐ݒ�
     * ---------------------------------------------------------------- */

    /* SQ ����̓|�[�g 0 �ɓo�^ */
    // midiCtx.buffGrp [IN_BUF].buffCtx [0 * 2 + DATA_BUF].buff = (void *) SQ_ADDR;
    minBfCtx  [0 * 2 + DATA_BUF].buff = (void *) SQ_ADDR;
    if (sceMidi_Load (&midiCtx, 0) != sceHSynNoError) {
	printf ("sceMidi_Load (%d) error\n", 0); return (-1);
    }

    /* ���̓|�[�g 0 �̐ݒ� */
    // ... �S�ẴR�[���o�b�N�p�f�[�^�̓o�b�t�@�ɒu�����
    midiEnv [0].chMsgCallBackPrivateData   = (unsigned int) &(streamBf [0]);
    midiEnv [0].metaMsgCallBack            = metaMsgCB;
    midiEnv [0].metaMsgCallBackPrivateData = (unsigned int) &(streamBf [0]);
    midiEnv [0].repeatCallBack             = repeatCB;
    midiEnv [0].repeatCallBackPrivateData  = (unsigned int) &(streamBf [0]);
    midiEnv [0].excOutPort = 1 << 0; // �G�N�X�N���[�V�u�͏o�̓|�[�g 0 �ɏo��
    for (i = 0; i < sceMidiNumMidiCh; i++) { // i: MIDI ch
	// ���̓|�[�g 0 �� MIDI �f�[�^�� MIDI ch �Ɋւ�炸�o�̓|�[�g 0 �ɏo��
	midiEnv [0].outPort [i] = 1 << 0;
    }

    // ���̓|�[�g 0 �̉��t�ΏۂƂ��āA�o�^���ꂽ SQ (SQ0) �̃u���b�N�ԍ� 0 ���w��
    if (sceMidi_SelectMidi (&midiCtx, 0, SQ_BLOCK0) != sceMidiNoError) {
	printf ("sceMidi_SelectMidi Error\n"); return (-1);
    }
    // ���̓|�[�g 0 �̉��t�J�n�ʒu��擪��
    if (sceMidi_MidiSetLocation (&midiCtx, 0, SQ_TOP) != sceMidiNoError) {
	printf ("sceMidi_MidiSetLocation Error\n"); return (-1);
    }
    // printf ("LOCATION = %d\n", sceMidi_GetEnv (&midiCtx, 0)->position); // �ʒu�̊m�F

    /* ���t
     * ---------------------------------------------------------------- */
    // ���̓|�[�g 0
    if (sceMidi_MidiPlaySwitch (&midiCtx, 0, sceMidi_MidiPlayStart) != sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n"); return (-1);
    }

    return 0;
}

/* ----------------------------------------------------------------
 * Hardware Synthesizer �Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
int
set_hsyn (sceHSyn_VoiceStat* pVstat)
{
    // modhsyn �S�̂̏�����
    if (sceHSyn_Init (&synthCtx, 4167) != sceHSynNoError) { // 4167 = 1/240sec
	printf ("sceHSyn_Init Error\n"); return (-1);
    }

    /* ���̓|�[�g 0 �� HD/BD �̐ݒ�
     * ---------------------------------------------------------------- */

    // modhsyn ���̓|�[�g 0 �̐ݒ�
    synthEnv [0].priority       = 0;
#if 0 /* �f�t�H���g�ł��̐ݒ� */
    synthEnv [0].portMode       = sceHSynModeHSyn;	// �ʏ탂�[�h
    synthEnv [0].waveType       = sceHSynTypeHSyn;	// �g�`�f�[�^�͏]���ʂ�
#endif
    synthEnv [0].lfoWaveNum 	= 0;
    synthEnv [0].lfoWaveTbl 	= NULL;
    synthEnv [0].velocityMapNum = 0;
    synthEnv [0].velocityMapTbl = NULL;

    // BD0 �� SPU2 ���[�J���������ɓ]��
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD0_ADDRESS, SPU_ADDR0, BD0_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n"); return (-1);
    }

    // HD0 �� BD0 ���u���̓|�[�g 0 �̃o���N�ԍ� 0�v�Ƃ��ēo�^
    if (sceHSyn_Load (&synthCtx, 0, SPU_ADDR0, HD0_ADDRESS, 0)
	!= sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", 0); return (-1);
    }

    /* ���̓|�[�g 1 �� HD/BD �̐ݒ�
     * ---------------------------------------------------------------- */

    // modhsyn ���̓|�[�g 0 �̐ݒ�
    synthEnv [1].priority       = 0;
    synthEnv [1].portMode       = sceHSynModeSESyn;	// �|�[�g�� SE �p
    synthEnv [1].waveType       = sceHSynTypeHSyn;	// �g�`�f�[�^�͏]���ʂ�
    synthEnv [1].lfoWaveNum 	= 0;
    synthEnv [1].lfoWaveTbl 	= NULL;
    synthEnv [1].velocityMapNum = 0;
    synthEnv [1].velocityMapTbl = NULL;

    // BD1 �� SPU2 ���[�J���������ɓ]��
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD1_ADDRESS, SPU_ADDR1, BD1_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n"); return (-1);
    }

    // HD1 �� BD1 ���u���̓|�[�g 1 �̃o���N�ԍ� 0�v�Ƃ��ēo�^
    if (sceHSyn_Load (&synthCtx, 1, SPU_ADDR1, HD1_ADDRESS, 0)
	!= sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", 1); return (-1);
    }

    // modhsyn �̏�ԃ��j�^�o�b�t�@��ݒ�
    sceHSyn_SetVoiceStatBuffer (pVstat);

    // SE �{�C�X�g�p��
    sceHSyn_SESetMaxVoices (12);

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
//static char vnum [48];

static int
atick (void)
{
    unsigned int count;
    int finalize;
    //char mnum;

    count = finalize = 0;
    while (1) {
	SleepThread ();

	switch (count) {
	case 100:
#ifdef sceSEInCommandVersion_0
	    sceSEIn_NoteOn (&seinCtx,
			    0,		// modsein port number
			    0x11223344,	// id
			    0,		// bank number
			    0,		// program number
			    48,		// note number
			    0x7f);	// velocity
#else
	    sceSEIn_NoteOn (&seinCtx,
			    0,		// modsein port number
			    0x11223344,	// id
			    0,		// bank number
			    0,		// program number
			    48,		// note number
			    0x7f,	// velocity
			    40);	// panpot
#endif
	    break;
	case 500:
#ifdef sceSEInCommandVersion_0
	    sceSEIn_NoteOn (&seinCtx, 0, 0x11223345, 0, 0, 72, 0x7f);
#else
	    sceSEIn_NoteOn (&seinCtx, 0, 0x11223345, 0, 0, 72, 0x7f, 64);
#endif
	    break;
	case 900:
#ifdef sceSEInCommandVersion_0
	    sceSEIn_NoteOn (&seinCtx, 0, 0x11223346, 0, 0, 96, 0x7f);
#else
	    sceSEIn_NoteOn (&seinCtx, 0, 0x11223346, 0, 0, 96, 0x7f, 64);
#endif
	    break;
	case 1500:
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223344, 0, 0, 48);
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223345, 0, 0, 72);
	    sceSEIn_NoteOff (&seinCtx, 0, 0x11223346, 0, 0, 96);
	    break;
	}

	// Tick ����
	sceMidi_ATick (&midiCtx);	// modmidi
	sceSEIn_ATick (&seinCtx);	// modsein
	sceHSyn_ATick (&synthCtx);	// modhsyn

	/*
	// �w�肵�� SE ID �Ŏg�p����Ă���{�C�X���擾�̗�:
	//     mnum �ɂ͎w�肵�� ID �Ŕ������Ă���{�C�X�� (��� 10)
	//     vnum �ɂ͎w�肵�� ID �Ŕ������Ă���{�C�X�ԍ� (��� 10)
	mnum = sceHSyn_SERetrieveVoiceNumberByID (&synthCtx,
						  1, // modhsyn port
						  0x11223344, // id
						  vnum,
						  10); // max
	// printf ("%02d\n", mnum);
	// ...
	*/					  

	if (! (sceMidi_isInPlay (&midiCtx, 0))) {
	    // �Ȃ��I��
	    if (! finalize) {
		sceSEIn_NoteOff (&seinCtx, 0, 0x11223344, 0, 0, 48);
		sceSEIn_NoteOff (&seinCtx, 0, 0x11223345, 0, 0, 72);
		sceSEIn_NoteOff (&seinCtx, 0, 0x11223346, 0, 0, 96);
		finalize ++;
	    }
	    if (! (vstat.pendingVoiceCount || vstat.workVoiceCount)) {
		// �{�C�X�����I��
		SignalSema (main_sema); // start �ɏ�����Ԃ�
	    }
	}

	count ++;
	if (count > 2000) count = 0;
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
    set_midi ();		// modmidi
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
