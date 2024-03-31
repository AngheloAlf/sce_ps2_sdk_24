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


/* ----------------------------------------------
   moduke ID number
   最上位ビットが1のIDはSCE R&D専用。
   カスタマイズした場合は変更すること。
  -----------------------------------------------*/
#define sce_SDR_DEV   0x80000701
#define sce_SDRST_CB  0x80000704

extern int (*_sce_sdr_gDMA0CB)(void);
extern int (*_sce_sdr_gDMA1CB)(void);
extern int (*_sce_sdr_gIRQCB)(void);

extern sceSdTransIntrHandler _sce_sdr_transIntr0Hdr;
extern sceSdTransIntrHandler _sce_sdr_transIntr1Hdr;
extern sceSdSpu2IntrHandler  _sce_sdr_spu2IntrHdr;

extern void *_sce_sdr_transIntr0Arg;
extern void *_sce_sdr_transIntr1Arg;
extern void *_sce_sdr_spu2IntrArg;

//--- コールバックのステータス受け渡し用
typedef struct {
    int		mode;	
    int		voice_bit;
    int		status;	
    int		opt;
    int         pad [12];  // キャッシュラインサイズ(64Byte)になるように
} SdrEECBData;

#ifdef __DEBUG
#define PRINTF(x) printf x
#else
#define PRINTF(x) 
#endif
