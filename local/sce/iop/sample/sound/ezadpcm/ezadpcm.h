/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                  I/O Processor sample program
 *                          Version 0.11
 *                           Shift-JIS
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                      ezadpcm.irx - ezadpcm.h
 *			 header file for EzADPCM
 *
 *	Version		Date		Design	Log
 *  --------------------------------------------------------------------
 *	0.10		Feb. 3, 2000	kaol	stolen from ezbgm/bgm_i.h
 *						  and modified
 */

// RPC command:
//   ビット 15 = 1: 値を返す
#define EzADPCM_COMMAND_MASK	0xfff0
#define EzADPCM_CH_MASK		0x000f

#define EzADPCM_INIT		0x8000
#define EzADPCM_QUIT		0x0010
#define EzADPCM_OPEN		0x8020
#define EzADPCM_CLOSE		0x0030
#define EzADPCM_PRELOAD		0x8040
#define EzADPCM_START		0x0050
#define EzADPCM_PRELOADSTART	0x8060
#define EzADPCM_STOP		0x8070
#define EzADPCM_SETVOICE	0x0080
#define EzADPCM_SETADDR		0x0090
#define EzADPCM_SETSIZE		0x00a0
#define EzADPCM_SETVOL		0x00b0
#define EzADPCM_SETVOLDIRECT	0x00c0
#define EzADPCM_SETMASTERVOL	0x00d0
#define EzADPCM_GETSTATUS	0x80e0
#define EzADPCM_SEEK		0x00f0
#define EzADPCM_SDINIT		0x7ff0

// for GETSTATUS
#define EzADPCM_STATUS			0x0001f000
#define EzADPCM_STATUS_IDLE		0x00000000
#define EzADPCM_STATUS_PRELOAD		0x00001000
#define EzADPCM_STATUS_PRELOADING	0x00002000
#define EzADPCM_STATUS_PRELOADED	0x00003000
#define EzADPCM_STATUS_RUNNABLE		0x00004000
#define EzADPCM_STATUS_RUNNING		0x00005000
#define EzADPCM_STATUS_TERMINATE	0x00006000

// Module ID number
//   サンプルなのでユーザー用番号。適宜変更のこと。
#define EzADPCM_DEV 0x00012345

#if 0
#define PRINTF(x) printf x
#else
#define PRINTF(x) 
#endif
#define ERROR(x) printf x
#define xPRINTF(x) 

#define BASE_priority  32

#define OLDLIB 0
#define TRANS_CH  0

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
