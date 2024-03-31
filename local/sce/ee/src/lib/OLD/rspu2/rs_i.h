/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                       librspu2 - rspu2cmd.h
 *                    internal headder for librspu2
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   0.30      Jun.17.1999     morita    first checked in
 */

typedef struct {
    int		mode;	
    int		voice_bit;
    int		status;	
    int		opt;
    int         pad[12];  // キャッシュラインサイズ(64Byte)になるように
} SpuEECBData;

//--- magic number
#define sce_SPU_DEV   0x80000601
#define sce_SPUST_DEV 0x80000602
#define sce_SPUST_CB  0x80000603

//#define PRINTF(x) printf x
#define PRINTF(x) 
