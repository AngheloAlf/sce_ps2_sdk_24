/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *                     I/O Processor Library
 *                          Version 2.0
 *                           Shift-JIS
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                         sqsong - main.c
 *                      SONG Sequencer Sample
 *
 *     Version   Date          Design   Log
 *  --------------------------------------------------------------------
 *     1.00      Jun. 2001     kaol     first checked in.
 */

#include <kernel.h>
#include <stdio.h>

#include <libsd.h>
#include <csl.h>
#include <cslmidi.h>
#include <modmidi.h>
#include <modhsyn.h>

#define MIDI_CHUNK 0
#define SONG_CHUNK 1

/* --- PLAY_CHUNK --- */
/* SONG ���t�̏ꍇ�ASONG_CHUNK ���w�肷��B
 *  MIDI ���t�̏ꍇ�AMIDI_CHUNK ���w�肷��B
 *  ... �ʏ�A������ MIDI Block �ɂ� SONG ���`������邽�߁AMIDI_CHUNK ��
 *      �w�肵���ꍇ�A�g�p�����f�[�^�ɂ���Ă͋Ȃ̈ꕔ�������t����Ȃ� */
#define PLAY_CHUNK SONG_CHUNK

/* ================================================================ */
/*  Constant macros / variables					    */
/* �萔�}�N��/�ϐ�						    */
/* ================================================================ */

/* ----------------------------------------------------------------
 * size / memory allocation
 * ---------------------------------------------------------------- */
// SQ0/HD0/BD0
#define SQ0_ADDRESS	0x101000
#define HD0_ADDRESS	((void *) 0x120000)
#define BD0_ADDRESS	((void *) 0x130000)
#define BD0_SIZE	(474944+0) // 64 �̔{����
// BD top address in SPU2 local memory
#define SPU_ADDR0	((void *) 0x5010)

volatile int main_sema;		/* �T�E���h������҂Z�}�t�H */

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
 * modmidi
 * ---------------------------------------------------------------- */
/* input port */
#define  IN_PORT_0   0		// For SQ0
#define  IN_PORT_NUM 1		// ���̓|�[�g�̑���

/* output port */
#define OUT_PORT_0   0		// SQ0 �̑S�Ă� MIDI ch �͂����ɏo��
#define OUT_PORT_NUM 1		// �o�̓|�[�g�̑���

/* CSL context */
static sceCslCtx midiCtx;

/* CSL buffer group */
static sceCslBuffGrp midiGrp [2]; // ��ɗv�f���� 2 ... 0/����, 1/�o��

/* CSL buffer context */
static sceCslBuffCtx midiIn  [IN_PORT_NUM * 2]; // ���̓|�[�g�̑��� x 2
static sceCslBuffCtx midiOut [OUT_PORT_NUM];    // �o�̓|�[�g�̑���

/* midi-stream buffer */
#define STREAMBUF_SIZE 1024	// �o�b�t�@�T�C�Y
#define STREAMBUF_SIZE_ALL (sizeof (sceCslMidiStream) + STREAMBUF_SIZE) // �o�b�t�@ + ����

// �e���o�̓|�[�g���g���o�b�t�@: ���̓|�[�g�̑���
static char streamBf [IN_PORT_NUM][STREAMBUF_SIZE_ALL];
// ���\����: ���̓|�[�g�̑���
static sceMidiEnv midiEnv [IN_PORT_NUM];

/* ----------------------------------------------------------------
 * modhsyn
 * ---------------------------------------------------------------- */
/* input port */
#define HS_PORT_0 0		// OUT_PORT_0 �̏o�͂����
#define HS_PORT_NUM 1		// ���̓|�[�g�̑���
#define HS_BANK_0 0		// HD0/BD0 �� HS_PORT_0 �ł̃o���N�ԍ�

/* CSL context */
static sceCslCtx synthCtx;

/* CSL buffer group */
static sceCslBuffGrp synthGrp [1];	// ��ɗv�f���� 1

/* CSL buffer context */
static sceCslBuffCtx synthIn [HS_PORT_NUM * 2];	 // ���̓|�[�g�̑��� x 2
static sceHSynEnv    synthEnv [HS_PORT_NUM];     // ���̓|�[�g�̑���

static sceHSyn_VoiceStat vstat;	// modhsyn ��ԃ��j�^�o�b�t�@

/* ================================================================
 * Timer
 * ================================================================ */
#include "timer.c"

/* ================================================================ */
/*  The example of `modmidi' callback functions			    */
/* `modmidi' �R�[���o�b�N�֐��̗�				    */
/* ================================================================ */

/* �S�Ẵ`�����l�����b�Z�[�W�ɑ΂��ČĂ΂��
 * ---------------------------------------------------------------- */
unsigned int
chMsgCB (unsigned int msg, unsigned int private_data)
{
    /* `msg' format:
       bit  0- 7: MIDI status
       bit  8-15: MIDI 1st data
       bit 16-23: MIDI 2nd data		*/
#if 0
    Kprintf ("Channel message: %02x %02x %02x\n",
	     (msg & 0x0000ff),
	     (msg & 0x007f00) >>  8,
	     (msg & 0x7f0000) >> 16);
#endif

    /* �Ԃ�l�̗� */

    /* �n���Ă��� ch message �����̊֐����ŏ������Amodmidi ���ł�
     *  �����𖳌��ɂ������ꍇ�� sceMidi_ChMsgNoData ��Ԃ��B*/
    // return (sceMidi_ChMsgNoData);

    /* �K�� MIDI ch = 0 �ɂ��ĕԂ� */
    // return (msg & 0x7f7ff0);

    /* �ʏ�͂��̂܂ܕԂ� (���邢�̓R�[���o�b�N�֐����̂��̂�ݒ肵�Ȃ�)�B*/
    return (msg);
}

/* meta event �ɑ΂��ČĂ΂��
 * ---------------------------------------------------------------- */
int
metaMsgCB (unsigned char metaNo, unsigned char *current_point, unsigned int len,
	   unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;

#if 0
    /* ���^�C�x���g��M */
    printf ("META %02x\n", (int) metaNo);
#endif

    /* �Ԃ�l�̗� */

    /* 0 ��Ԃ��� modmidi ���� current_point �̈ʒu���璷�� len ��
     * �f�[�^���ǂݔ�΂����
     * �� �����̃f�[�^�͂��̃R�[���o�b�N�֐����ŏ������ꂽ���̂�
     *    ���Ĉ����� */
    // return 0;

    /* �ʏ�� 0 �ȊO�̒l��Ԃ��Amodmidi ���ŏ���������
     * (���邢�̓R�[���o�b�N���̂��̂�ݒ肵�Ȃ�)�B*/
    return (NEXT_ENABLE);
}

/* exclusive message �ɑ΂��ČĂ΂��
 * ---------------------------------------------------------------- */
int
exMsgCB (unsigned char *current_point, unsigned int len, unsigned int private_data)
{
    /* ... */	/* exclusive message �ɉ����ď������s�� */

    /* �Ԃ�l�̗� */

    /* 0 ��Ԃ��� modmidi ���� current_point �̈ʒu���璷�� len ��
     * �f�[�^���ǂݔ�΂���� */
    // return 0;

    /* �ʏ�� 0 �ȊO�̒l��Ԃ��Amodmidi ���ŏ���������
     * (���邢�̓R�[���o�b�N���̂��̂�ݒ肵�Ȃ�)�B*/
    return (NEXT_ENABLE);
}

/* �g�� MIDI ���b�Z�[�W: repeat �ɑ΂��ČĂ΂��
 * ---------------------------------------------------------------- */
int
repeatCB (sceMidiLoopInfo *pLoopInfo, unsigned int private_data)
{
    // sceCslMidiStream *pHdr = (sceCslMidiStream *) private_data;

#if 0
    printf ("Repeat %s %d %d %x\n",
	    (pLoopInfo->type == sceMidiLoopInfoType_Midi) ? "Midi" : "Song",
	    (int) pLoopInfo->loopTimes,
	    (int) pLoopInfo->loopCount, pLoopInfo->loopId);
#endif

    /* �Ԃ�l�̗� */

    /* 0 ��Ԃ��ƃ��s�[�g���s���Ȃ� */
    // return 0;

    /* �ʏ�� 0 �ȊO�̒l��Ԃ��A���s�[�g�𑱍s������
     * (���邢�̓R�[���o�b�N���̂��̂�ݒ肵�Ȃ�)�B*/
    return (NEXT_ENABLE);
}

/* ================================================================ */
/*  Set up each Mudule Context					    */
/* �e Mudule Context �̐ݒ�					    */
/* ================================================================ */

/*
================================================================
CSL context / buffer group / buffer context / stream buffer
================================================================

                                                  IOP memory
 +- midiCtx -----------+                           |            |
 |       [CSL context] |                  0x101000 |------------| <=+
 |       conf: NULL    |                           | SQ0        |   |
 |   callBack: NULL    |                           |            |   |
 |     extmod: NULL    |                           |            |   |
 | buffGrpNum: 2       |                           |------------|   |
 |                +- midiGrp ------------------+   |            |   |
 |    buffGrp: ==>|         [CSL buffer group] |                    |
 +----------------| 0 / input                  |                    |
                  |  buffNum: IN_PORT_NUM x 2  |                    |
                  |                   +- midiIn -------------+      |
                  |  buffCtx: =======>| [CSL buffer context] |      |
                  |                   | 0 / data             |      |
                  |                   |  sema: 0             |      |
                  |                   |  buff: =====================+
                  |                   +- - - - - - - - - - - +
                  |                   | 1 / environment      |
                  |                   |  sema: 0       +- midiEnv -----+
                  |                   |  buff: =======>| [environment] |
                  |                   +----------------|               |
                  +- - - - - - - - - - - - - - +       +---------------+
                  | 1 / output                 |
                  |  buffNum: OUT_PORT_NUM = 1 |
                  |                   +- midiOut ------------+
                  |  buffCtx: =======>| [CSL buffer context] |
                  +-------------------| 0 / data             |
                                      |  sema: 0             |
                                      |  buff: ===========================+
                                      +----------------------+            |
                                                                          |
                                              +- streamBf ------------+   |
                                              | [midi-stream buffer]  | <=+
                                              | buffsize:             | <=+
                                              |    STREAMBUF_SIZE_ALL |   |
 +- synthCtx ----------+                      | validsize: 0          |   |
 |       [CSL context] |                      |- - - - - - - - - - - -+   |
 |       conf: NULL    |                      | data:                 |   |
 |   callBack: NULL    |                      |                       |   |
 |     extmod: NULL    |                      +-----------------------+   |
 | buffGrpNum: 1       |                                                  |
 |                +- synthGrp -----------------+                          |
 |    buffGrp: ==>|         [CSL buffer group] |                          |
 +----------------| 0 / input                  |                          |
                  |  buffNum: HS_PORT_NUM * 2  |                          |
                  |                   +- synthIn ------------+            |
                  |  buffCtx: =======>| [CSL buffer context] |            |
                  |                   | 0 / data             |            |
                  |                   |  sema: 0             |            |
                  |                   |  buff: ===========================+
                  |                   +- - - - - - - - - - - +
                  |                   | 1 / environment      |
                  |                   |  sema: 0       +- synthEnv ----+
                  |                   |  buff: =======>| [environment] |<=+
                  |                   +----------------|               |  |
                  +----------------------------+       +---------------+  |
                                         IOP memory                       |
                                         |           |                    |
                                0x120000 |-----------|          bank #: 0 |
                                         |HD0        | ===================+
                                         |           |                    |
                                         |      SPU2 local memory         |
                                         |------|           |             |
                                         |      |-----------|0x5010       |
                                                |BD0        | ============+
                                                |           |
                                                |           |
                                                |-----------|
                                                |           |
*/

void
set_module_context (void)
{
    /* modmidi
     * ---------------------------------------------------------------- */

    // CSL context: modmidi �͏�� 2 �� buffer group
    midiCtx.extmod     = NULL;
    midiCtx.callBack   = NULL;
    midiCtx.buffGrpNum = 2;				// modmidi �͏�� 2
    midiCtx.buffGrp    = midiGrp;

    /* ... input ... */

    // CSL buffer group: ���̓|�[�g���� IN_PORT_NUM
    midiGrp [IN_BUF].buffNum = IN_PORT_NUM * 2;		// ���̓|�[�g�� x 2
    midiGrp [IN_BUF].buffCtx = midiIn;

    // CSL buffer context:
    //   ���̓|�[�g IN_PORT_0:  SQ �f�[�^�̃A�h���X�͌�Őݒ� �� NULL ��
    midiIn [IN_PORT_0 * 2 + DATA_BUF].sema = 0;
    midiIn [IN_PORT_0 * 2 + DATA_BUF].buff = NULL;	// set_midi() �ɂĐݒ�
    midiIn [IN_PORT_0 * 2 +  ENV_BUF].sema = 0;
    midiIn [IN_PORT_0 * 2 +  ENV_BUF].buff = &(midiEnv [IN_PORT_0]);

    /* ... output ... */

    // CSL buffer group: �o�̓|�[�g���� OUT_PORT_NUM
    midiGrp [OUT_BUF].buffNum = OUT_PORT_NUM;		// �o�̓|�[�g��
    midiGrp [OUT_BUF].buffCtx = midiOut;

    // CSL buffer context:
    //   �o�̓|�[�g OUT_PORT_0
    //   ... ���̓|�[�g IN_PORT_0 �̑S�Ă� MIDI ch �͂����ɏo��
    midiOut [OUT_PORT_0].sema = 0;
    midiOut [OUT_PORT_0].buff = &(streamBf [OUT_PORT_0]); // modhsyn �Ƌ��L

    /* modhsyn
     * ---------------------------------------------------------------- */

    // CSL context: modhsyn �͏�� 1
    synthCtx.extmod   	= NULL;
    synthCtx.callBack 	= NULL;
    synthCtx.buffGrpNum = 1;				// modhsyn �͏�� 1
    synthCtx.buffGrp    = synthGrp;

    /* ... input ... */

    // CSL buffer group: ���̓|�[�g���� HS_PORT_NUM
    synthGrp [IN_BUF].buffNum = HS_PORT_NUM * 2;	// ���̓|�[�g�� x 2
    synthGrp [IN_BUF].buffCtx = synthIn;

    // CSL buffer context:
    //   ���̓|�[�g HS_PORT_0 �́AstreamBf[OUT_PORT_0] �� modmidi �Ƌ��L
    synthIn [HS_PORT_0 * 2 + DATA_BUF].sema = 0;
    synthIn [HS_PORT_0 * 2 + DATA_BUF].buff = &(streamBf [OUT_PORT_0]); // modmidi�Ƌ��L
    synthIn [HS_PORT_0 * 2 +  ENV_BUF].sema = 0;
    synthIn [HS_PORT_0 * 2 +  ENV_BUF].buff = &(synthEnv [HS_PORT_0]);

    /* midi-stream buffer
     * ---------------------------------------------------------------- */

    // �T�C�Y�� STREAMBUF_SIZE_ALL
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_0]))->buffsize  = STREAMBUF_SIZE_ALL;
    ((sceCslMidiStream *) &(streamBf [OUT_PORT_0]))->validsize = 0;

    return;
}

/* ----------------------------------------------------------------
 * Hardware Synthesizer �Z�b�g�A�b�v
 * ---------------------------------------------------------------- */
int
set_hsyn (sceHSyn_VoiceStat* pVstat)
{
    /* modhsyn �S�̂̏�����
     * ---------------------------------------------------------------- */
    if (sceHSyn_Init (&synthCtx, ONE_240TH) != sceHSynNoError) { // 1/240sec
	printf ("sceHSyn_Init Error\n");
	return (-1);
    }

    /* ���̓|�[�g HS_PORT_0 �� HD0/BD0 �̐ݒ�
     * ---------------------------------------------------------------- */

    // modhsyn ���̓|�[�g HS_PORT_0 �̐ݒ�
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

    /* modmidi �S�̂̏�����
     * ---------------------------------------------------------------- */
    if (sceMidi_Init (&midiCtx, ONE_240TH) != sceMidiNoError) { // 1/240sec
	printf ("sceMidi_Init Error\n");
	return (-1);
    }

    /* modmidi ���̓|�[�g IN_PORT_0 �̐ݒ�
     * ---------------------------------------------------------------- */

    // SQ0 ����̓|�[�g IN_PORT_0 �ɓo�^
    midiIn  [IN_PORT_0 * 2 + DATA_BUF].buff = (void *) SQ0_ADDRESS;

    if (sceMidi_Load (&midiCtx, IN_PORT_0) != sceHSynNoError) {
	printf ("sceMidi_Load (%d) error\n", IN_PORT_0);
	return (-1);
    }

    // �R�[���o�b�N�֐��ɓn��v���C�x�[�g�f�[�^�͑S�Ẵo�b�t�@�ɐݒ�
    midiEnv [IN_PORT_0].chMsgCallBack              = chMsgCB;
    midiEnv [IN_PORT_0].chMsgCallBackPrivateData   = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].metaMsgCallBack            = metaMsgCB;
    midiEnv [IN_PORT_0].metaMsgCallBackPrivateData = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].repeatCallBack             = repeatCB;
    midiEnv [IN_PORT_0].repeatCallBackPrivateData  = (unsigned int) &(streamBf [IN_PORT_0]);
    midiEnv [IN_PORT_0].excMsgCallBack             = exMsgCB;
    midiEnv [IN_PORT_0].excMsgCallBackPrivateData  = (unsigned int) &(streamBf [IN_PORT_0]);

    // �G�N�X�N���[�V�u�f�[�^�͏o�̓|�[�g OUT_PORT_0 �ɏo��
    midiEnv [IN_PORT_0].excOutPort = 1 << OUT_PORT_0;

    // ���̓|�[�g IN_PORT_0 �� MIDI �f�[�^�́A
    // MIDI ch �Ɋւ�炸�o�̓|�[�g OUT_PORT_0 �ɏo��
    for (i = 0; i < sceMidiNumMidiCh; i++) { // i: MIDI ch
	midiEnv [IN_PORT_0].outPort [i] =  1 << OUT_PORT_0;
    }

    // ���̓|�[�g 0 �̉��t�Ώۂ�ݒ�
#if PLAY_CHUNK == SONG_CHUNK
    // �o�^���ꂽ SQ (SQ0) �� SONG table 0 �Ԃ��w��
    if (sceMidi_SelectSong (&midiCtx, IN_PORT_0, 0) != sceMidiNoError) {
	printf ("sceMidi_SelectSong Error\n");
	return (-1);
    }
#elif PLAY_CHUNK == MIDI_CHUNK
    // �o�^���ꂽ SQ (SQ0) �� MIDI Data Block 0 �Ԃ��w��
    if (sceMidi_SelectMidi (&midiCtx, IN_PORT_0, 0) != sceMidiNoError) {
	printf ("sceMidi_SelectMidi Error\n");
	return (-1);
    }
#endif

    // ���̓|�[�g 0 �̉��t�J�n�ʒu��擪��
#if PLAY_CHUNK == SONG_CHUNK
    if (sceMidi_SongSetLocation (&midiCtx, IN_PORT_0, 0, sceMidi_SSL_WithPreCommand)
	!= sceMidiNoError) {
	printf ("sceMidi_SongSetLocation Error\n");
	return (-1);
    }
#elif PLAY_CHUNK == MIDI_CHUNK
    if (sceMidi_MidiSetLocation (&midiCtx, IN_PORT_0, 0) != sceMidiNoError) {
	printf ("sceMidi_MidiSetLocation Error\n");
	return (-1);
    }
#endif

    /* ���t
     * ---------------------------------------------------------------- */

    // ���̓|�[�g IN_PORT_0 �̃f�[�^�����t
#if PLAY_CHUNK == SONG_CHUNK
    if (sceMidi_SongPlaySwitch (&midiCtx, IN_PORT_0, sceMidi_SongPlayStart)
	!= sceMidiNoError) {
	printf ("sceMidi_SongPlaySwitch Error\n");
	return (-1);
    }
#elif PLAY_CHUNK == MIDI_CHUNK
    if (sceMidi_MidiPlaySwitch (&midiCtx, IN_PORT_0, sceMidi_MidiPlayStart)
	!= sceMidiNoError) {
	printf ("sceMidi_MidiPlaySwitch Error\n");
	return (-1);
    }
#endif

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

#if PLAY_CHUNK == SONG_CHUNK
	if (! ((sceMidi_GetEnv (&midiCtx, IN_PORT_0)->status) & sceMidiSongStat_inPlay)) {
#elif PLAY_CHUNK == MIDI_CHUNK
	if (! (sceMidi_isInPlay (&midiCtx, IN_PORT_0))) {
#endif
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
