/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/* 
 *
 *      Copyright (C) 1998-2002 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                       libsdr - sdr_i.h
 *                    internal headder for libsdr
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   0.60      Oct.12.1999     morita    first checked in
 */
#ifndef _SDRDRV_I_H_
#define _SDRDRV_I_H_


/* ----------------------------------------------
   moduke ID number
   �ŏ�ʃr�b�g��1��ID��SCE R&D��p�B
   �J�X�^�}�C�Y�����ꍇ�͕ύX���邱�ƁB
  -----------------------------------------------*/
#define sce_SDR_DEV   0x80000701
#define sce_SDRST_CB  0x80000704

#ifdef SCE_OBSOLETE
extern int _sce_sdrDMA0CallBackProc (void *common);
extern int _sce_sdrDMA1CallBackProc (void *common);
extern int _sce_sdrIRQCallBackProc (void *common);
#endif
extern int _sce_sdrDMA0IntrHandler (int core, void *common);
extern int _sce_sdrDMA1IntrHandler (int core, void *common);
extern int _sce_sdrSpu2IntrHandler (int core_bit, void *common);

/* �R�[���o�b�N�̃X�e�[�^�X�󂯓n���p */
typedef struct {
    int		mode;	
    int		voice_bit;
    int		status;	
    int		opt;
    int         pad[12];  /* �L���b�V�����C���T�C�Y(64Byte)�ɂȂ�悤�� */
} SdrEECBData;

extern int initial_priority_main;	/* sdr module main thread priority */
extern int initial_priority_cb;		/* sdr callback thread priority */

#if 0
#define PRINTF(x) printf x
#else
#define PRINTF(x) 
#endif

#define PRIORITY_MODULE_SDRDRV 24

#endif /* _SDRDRV_I_H_ */
