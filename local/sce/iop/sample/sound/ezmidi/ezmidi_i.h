/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                       EzMIDI - ezmidi_i.h
 *                       headder for EzMIDI
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   1.30      Dec.12.1999     morita    first checked in
 */

//--- 0x8000ビットは戻り値アリを表す
//--- 0x1000ビットは引数が構造体であること表す
//--- 0x0001ビットはポート指定（関係あるもののみ）

#define EzMIDI_START          0x0000
#define EzMIDI_INIT           0x8010
#define EzMIDI_STOP           0x0020
#define EzMIDI_SEEK           0x0030
#define EzMIDI_SETSQ          0x0040
#define EzMIDI_SETHD_P0       0x9050
#define EzMIDI_SETHD_P1       0x9051
#define EzMIDI_TRANSBDPACKET  0x9060
#define EzMIDI_TRANSBD        0x0070
#define EzMIDI_GETFILELENGTH  0x9080
#define EzMIDI_GETSTATUS      0x8090
#define EzMIDI_SETATTR_P0     0x00a0
#define EzMIDI_SETATTR_P1     0x00a1
#define EzMIDI_SETVOL_P0      0x00b0
#define EzMIDI_SETVOL_P1      0x00b1

#define MSIN_BUF_SIZE  256

typedef struct {
    unsigned int     hdAddr;
    unsigned int     bdAddr;
    unsigned int     bdSize;
    unsigned int     spuAddr;
    char             bdName [64-4*4];
} EZMIDI_BANK;

#define PORTATTR_MASK_PRIORITY  0x00FF
#define PORTATTR_MASK_MAXNOTE   0xFF00

/* ----------------------------------------------
   module ID number
   サンプルなのでユーザー用番号。適宜変更のこと。
  -----------------------------------------------*/
#define EZMIDI_DEV   0x00012346

//#define PRINTF(x) printf x
#define PRINTF(x) 

#define ERROR(x) printf x
//#define ERROR(x) 

#define BASE_priority  32

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
