/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *                          Version 2.0
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         sqhard - main.c
 *                      MIDI Sequencer Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     2.0.6     Nov.,2000     kaol     New timer functions are used.
 *     2.0       Jun.28,2000   kaol     Two SQ/HD/BD can be handled,
 *                                       and ATick is handled by thread.
 *               Jan.27,2000   kaol     gWaitFlag was not initialized,
 *                                        and modified in intr. routine.
 *     1.20      Nov.28.1999   morita   remove MMIX_SINE setting because
 *                                      it has been default.
 *     0.60      Oct.14.1999   morita   first checked in.
 */

#include <kernel.h>
#include <stdio.h>

#include <libsd.h>
#include <csl.h>
#include <cslmidi.h>
#include <modmidi.h>
#include <modhsyn.h>

#define True 1

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

/* ----------------
 * CallBack �̗�
 * ---------------- */
int
metaMsgCB (unsigned char metaNo, unsigned char *bf, unsigned int len, unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;

#if 0
    // ���^�C�x���g��M
    printf ("META %02x\n", (int) metaNo);
#endif
    return True;
}

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
/* ----------------
 * �T�C�Y/�������z�u
 * ---------------- */
// 1 �߂̉��t�f�[�^: SQ0/HD0/BD0
#define SQ0_ADDR	0x101000
#define SQ0_BLOCK0	0	// ���t�u���b�N�ԍ�: sakana.sq �ɂ� 1 �����Ȃ�
#define HD0_ADDRESS	((void *) 0x120000)
#define BD0_ADDRESS	((void *) 0x130000)
#define BD0_SIZE	(335520+32) // 64 �̔{����
// BD �� SPU2 ���[�J�����������擪�A�h���X
#define SPU_ADDR0	((void *) 0x5010)

// 2 �߂̉��t�f�[�^: SQ1/HD1/BD1
#define SQ1_ADDR	0x104000
#define SQ1_BLOCK0	0	// ���t�u���b�N�ԍ�: ___.sq �ɂ� 1 �����Ȃ�
#define HD1_ADDRESS	((void *) 0x128000)
#define BD1_ADDRESS	((void *) 0x190000)
#define BD1_SIZE	(474944+0) // 64 �̔{����
#define SPU_ADDR1	((void *) (0x5010 + BD0_SIZE))

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
 * modmidi �֘A
 * ---------------------------------------------------------------- */
// ���̓|�[�g
#define  IN_PORT_0   0		// SQ0 �p
#define  IN_PORT_1   1		// SQ1 �p
#define  IN_PORT_NUM 2		// ���̓|�[�g�̑���

// �o�̓|�[�g
#define OUT_PORT_0   0		// SQ0 �̑S�Ă� MIDI ch �͂����ɏo��
#define OUT_PORT_1   1		// SQ1 �̑S�Ă� MIDI ch �͂����ɏo��
#define OUT_PORT_NUM 2		// �o�̓|�[�g�̑���

/* CSL context */
static sceCslCtx midiCtx;

/* CSL buffer group */
static sceCslBuffGrp midiGrp [2]; // ��ɗv�f���� 2 ... 0/����, 1/�o��

/* CSL buffer context */
static sceCslBuffCtx mInBfCtx  [IN_PORT_NUM * 2]; // ���̓|�[�g�̑��� x 2
static sceCslBuffCtx mOutBfCtx [OUT_PORT_NUM];    // �o�̓|�[�g�̑���

/*
 * midi-stream buffer
 */
#define STREAMBUF_SIZE 1024	// �o�b�t�@�T�C�Y
#define STREAMBUF_SIZE_ALL (sizeof (sceCslMidiStream) + STREAMBUF_SIZE) // �o�b�t�@ + ����

// �e���o�̓|�[�g���g���o�b�t�@: ���̓|�[�g�̑���
static char streamBf [IN_PORT_NUM][STREAMBUF_SIZE_ALL];
// ���\����: ���̓|�[�g�̑���
static sceMidiEnv midiEnv [IN_PORT_NUM];

#define SQ_TOP 0

/* ----------------------------------------------------------------
 * modhsyn �֘A
 * ---------------------------------------------------------------- */
// ���̓|�[�g
#define HS_PORT_0 0		// OUT_PORT_0 �̏o�͂����
#define HS_PORT_1 1		// OUT_PORT_1 �̏o�͂����
#define HS_PORT_NUM 2		// ���̓|�[�g�̑���
#define HS_BANK_0 0		// HD0/BD0 �� HS_PORT_0 �ł̃o���N�ԍ�
#define HS_BANK_1 0		// HD1/BD1 �� HS_PORT_1 �ł̃o���N�ԍ�

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
     * modmidi
     */
    // CSL context
    midiCtx.extmod     = NULL;
    midiCtx.callBack   = NULL;
    midiCtx.buffGrpNum = 2;	// modmidi �͏�� 2
    midiCtx.buffGrp    = midiGrp;
      // CSL buffer group: ����
      midiGrp [ IN_BUF].buffNum = IN_PORT_NUM * 2;	// ���̓|�[�g�� x 2
      midiGrp [ IN_BUF].buffCtx = mInBfCtx;
        // CSL buffer context
        // SQ0 �p
        mInBfCtx  [IN_PORT_0 * 2 + DATA_BUF].sema = 0;
	mInBfCtx  [IN_PORT_0 * 2 + DATA_BUF].buff = NULL; // set_midi() �ɂĐݒ�
	mInBfCtx  [IN_PORT_0 * 2 +  ENV_BUF].sema = 0;
	mInBfCtx  [IN_PORT_0 * 2 +  ENV_BUF].buff = &(midiEnv [IN_PORT_0]);
        // SQ1 �p
        mInBfCtx  [IN_PORT_1 * 2 + DATA_BUF].sema = 0;
	mInBfCtx  [IN_PORT_1 * 2 + DATA_BUF].buff = NULL; // set_midi() �ɂĐݒ�
	mInBfCtx  [IN_PORT_1 * 2 +  ENV_BUF].sema = 0;
	mInBfCtx  [IN_PORT_1 * 2 +  ENV_BUF].buff = &(midiEnv [IN_PORT_1]);
      // CSL buffer group: �o��
      midiGrp [OUT_BUF].buffNum = OUT_PORT_NUM;		// �o�̓|�[�g��
      midiGrp [OUT_BUF].buffCtx = mOutBfCtx;
        // �o�̓|�[�g 0 ... SQ0 �̑S�Ă� MIDI ch �͂����ɏo��
	mOutBfCtx [OUT_PORT_0              ].sema = 0;
	mOutBfCtx [OUT_PORT_0              ].buff = &(streamBf [OUT_PORT_0]); // modhsyn �Ƌ��L
        // �o�̓|�[�g 1 ... SQ1 �̑S�Ă� MIDI ch �͂����ɏo��
	mOutBfCtx [OUT_PORT_1              ].sema = 0;
	mOutBfCtx [OUT_PORT_1              ].buff = &(streamBf [OUT_PORT_1]); // modhsyn �Ƌ��L

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
        // OUT_PORT_1 �p
        sInBfCtx [HS_PORT_1 * 2 + DATA_BUF].sema = 0;
	sInBfCtx [HS_PORT_1 * 2 + DATA_BUF].buff = &(streamBf [OUT_PORT_1]); // modmidi�Ƌ��L
	sInBfCtx [HS_PORT_1 * 2 +  ENV_BUF].sema = 0;
	sInBfCtx [HS_PORT_1 * 2 +  ENV_BUF].buff = &(synthEnv [HS_PORT_1]);

    /*
     * midi-stream buffer
     */
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_0]))->validsize = 0;
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_1]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_1]))->validsize = 0;

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

    /* ���̓|�[�g 1 �� HD1/BD1 �̐ݒ�
     * ---------------------------------------------------------------- */

    // modhsyn ���̓|�[�g 1 �̐ݒ�
    synthEnv [HS_PORT_1].priority       = 0;
    synthEnv [HS_PORT_1].lfoWaveNum 	= 0;
    synthEnv [HS_PORT_1].lfoWaveTbl 	= NULL;
    synthEnv [HS_PORT_1].velocityMapNum = 0;
    synthEnv [HS_PORT_1].velocityMapTbl = NULL;

    // BD1 �� SPU2 ���[�J���������ɓ]��
    if (sceHSyn_VoiceTrans (SD_CORE_0, BD1_ADDRESS, SPU_ADDR1, BD1_SIZE)
	!= sceHSynNoError) {
	printf ("sceHSyn_VoiceTrans Error\n");
	return (-1);
    }

    // HD1 �� BD1 ���u���̓|�[�g 1 �̃o���N�ԍ� HS_BANK_1 (= 0)�v�Ƃ��ēo�^
    if (sceHSyn_Load (&synthCtx, HS_PORT_1, SPU_ADDR1, HD1_ADDRESS, HS_BANK_1)
	!= sceHSynNoError) {
	printf ("sceHSyn_Load (%d) error\n", HS_PORT_1);
	return (-1);
    }

    // modhsyn �̏�ԃ��j�^�o�b�t�@��ݒ�
    sceHSyn_SetVoiceStatBuffer (pVstat);

    return 0;
}

/* ----------------------------------------------------------------
 * Midi Sequencer �Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
int
set_midi (void)
{
    int i;

    // modmidi �S�̂̏�����
    if (sceMidi_Init (&midiCtx, ONE_240TH) != sceMidiNoError) { // 1/240sec
	printf ("sceMidi_Init Error\n");
	return (-1);
    }

    /* modmidi ���̓|�[�g 0 �̐ݒ�
     * ---------------------------------------------------------------- */

    /* SQ0 ����̓|�[�g 0 �ɓo�^ */
    // midiCtx.buffGrp [IN_BUF].buffCtx [IN_PORT_0 * 2 + DATA_BUF].buff = (void *) SQ0_ADDR;
    mInBfCtx  [IN_PORT_0 * 2 + DATA_BUF].buff = (void *) SQ0_ADDR;
    if (sceMidi_Load (&midiCtx, IN_PORT_0) != sceHSynNoError) {
	printf ("sceMidi_Load (%d) error\n", i);
	return (-1);
    }

    /* ���̓|�[�g 0 �̐ݒ� */

    // ... �S�ẴR�[���o�b�N�p�f�[�^�̓o�b�t�@�ɒu�����
    midiEnv [IN_PORT_0].chMsgCallBackPrivateData   = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].metaMsgCallBack            = metaMsgCB;
    midiEnv [IN_PORT_0].metaMsgCallBackPrivateData = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].repeatCallBack             = repeatCB;
    midiEnv [IN_PORT_0].repeatCallBackPrivateData  = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].excOutPort = 1 << OUT_PORT_0; // �G�N�X�N���[�V�u�͏o�̓|�[�g 0 �ɏo��

    // ���̓|�[�g 0 �� MIDI �f�[�^�� MIDI ch �Ɋւ�炸�o�̓|�[�g 0 �ɏo��
    for (i = 0; i < sceMidiNumMidiCh; i++) { // i: MIDI ch
	midiEnv [IN_PORT_0].outPort [i] =  1 << OUT_PORT_0;
    }

    // ���̓|�[�g 0 �̉��t�ΏۂƂ��āA�o�^���ꂽ SQ (SQ0) �̃u���b�N�ԍ� 0 ���w��
    if (sceMidi_SelectMidi (&midiCtx, IN_PORT_0, SQ0_BLOCK0) != sceMidiNoError) {
	printf ("sceMidi_SelectMidi Error\n");
	return (-1);
    }
    // ���̓|�[�g 0 �̉��t�J�n�ʒu��擪��
    if (sceMidi_MidiSetLocation (&midiCtx, IN_PORT_0, SQ_TOP) != sceMidiNoError) {
	printf ("sceMidi_MidiSetLocation Error\n");
	return (-1);
    }
#if 0
    printf ("LOCATION = %d\n", sceMidi_GetEnv (&midiCtx, IN_PORT_0)->position); // �ʒu�̊m�F
#endif

    /* modmidi ���̓|�[�g 1 �̐ݒ�
     * ---------------------------------------------------------------- */

    /* SQ1 ����̓|�[�g 0 �ɓo�^ */
    // midiCtx.buffGrp [IN_BUF].buffCtx [IN_PORT_1 * 2 + DATA_BUF].buff = (void *) SQ1_ADDR;
    mInBfCtx  [IN_PORT_1 * 2 + DATA_BUF].buff = (void *) SQ1_ADDR;
    if (sceMidi_Load (&midiCtx, IN_PORT_1) != sceHSynNoError) {
	printf ("sceMidi_Load (%d) error\n", i);
	return (-1);
    }

    /* ���̓|�[�g 1 �̐ݒ� */
    // ... �S�ẴR�[���o�b�N�p�f�[�^�̓o�b�t�@�ɒu�����
    midiEnv [IN_PORT_1].chMsgCallBackPrivateData   = (unsigned int) &(streamBf [IN_PORT_1]);
    midiEnv [IN_PORT_1].metaMsgCallBack            = metaMsgCB;
    midiEnv [IN_PORT_1].metaMsgCallBackPrivateData = (unsigned int) &(streamBf [IN_PORT_1]);
    midiEnv [IN_PORT_1].repeatCallBack             = repeatCB;
    midiEnv [IN_PORT_1].repeatCallBackPrivateData  = (unsigned int) &(streamBf [IN_PORT_1]);
    midiEnv [IN_PORT_1].excOutPort = 1 << OUT_PORT_1; // �G�N�X�N���[�V�u�͏o�̓|�[�g 1 �ɏo��
    for (i = 0; i < sceMidiNumMidiCh; i++) { // i: MIDI ch
	// ���̓|�[�g 1 �� MIDI �f�[�^�� MIDI ch �Ɋւ�炸�o�̓|�[�g 1 �ɏo��
	midiEnv [IN_PORT_1].outPort [i] =  1 << OUT_PORT_1;
    }

    // ���̓|�[�g 1 �̉��t�ΏۂƂ��āA�o�^���ꂽ SQ (SQ1) �̃u���b�N�ԍ� 0 ���w��
    if (sceMidi_SelectMidi (&midiCtx, IN_PORT_1, SQ1_BLOCK0) != sceMidiNoError) {
	printf ("sceMidi_SelectMidi Error\n");
	return (-1);
    }

    // ���̓|�[�g 1 �̉��t�J�n�ʒu��擪��
    if (sceMidi_MidiSetLocation (&midiCtx, IN_PORT_1, SQ_TOP) != sceMidiNoError) {
	printf ("sceMidi_MidiSetLocation Error\n");
	return (-1);
    }
#if 0
    printf ("LOCATION = %d\n", sceMidi_GetEnv (&midiCtx, IN_PORT_1)->position); // �ʒu�̊m�F
#endif

    /* ���t
     * ---------------------------------------------------------------- */
    // ���̓|�[�g 0
    if (sceMidi_MidiPlaySwitch (&midiCtx, IN_PORT_0, sceMidi_MidiPlayStart) != sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n");
	return (-1);
    }
    // ���̓|�[�g 1
    if (sceMidi_MidiPlaySwitch (&midiCtx, IN_PORT_1, sceMidi_MidiPlayStart) != sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n");
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
    while (1) {
	SleepThread ();

	// Tick ����
	sceMidi_ATick (&midiCtx);	// modmidi
	sceHSyn_ATick (&synthCtx);	// modhsyn

	if (! (sceMidi_isInPlay (&midiCtx, IN_PORT_0) ||
	       sceMidi_isInPlay (&midiCtx, IN_PORT_1))) {
	    // �Ȃ��I��
	    if (! (vstat.pendingVoiceCount || vstat.workVoiceCount)) {
		// ��������Ă���{�C�X����
		SignalSema (main_sema); // start() �ɏ�����Ԃ�
	    }
	}
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

    // initialize hardware
    sceSdInit (0);
    set_spu2 ();		// Digital effect/master volume

    // initialize modules environment
    set_module_context ();	// CSL module context
    set_hsyn (&vstat);		// modhsyn
    set_midi ();		// modmidi

    // semaphore: for finishing the play
    main_sema = make_semaphore ();

    // Thread: atick() process
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
