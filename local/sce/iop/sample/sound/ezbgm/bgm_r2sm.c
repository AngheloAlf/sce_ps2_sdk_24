/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                  I/O Proseccor sample program
 *                          Version 1.20
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                      ezbgm.irx - bgm_r2sm.c
 *                  raw to spu pcm (mono version)
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   1.20      Nov.23.1999     morita    modify for EzBGM
 */


#include <kernel.h>

#define SPU_BLOCK 512

// Scratch Pad を使えばもっと速くなるが…

/* モノラル指定時の波形データ変換ルーチン */
void
_BgmRaw2SpuMono (unsigned int *src, unsigned int *dst, unsigned int block)
{
    int i;

    for (i = 0; i < block; i++) {
	memcpy ((void *)((int)dst + i * SPU_BLOCK * 2),
		(void *)((int)src + i * SPU_BLOCK),
		SPU_BLOCK);
	memcpy ((void *)((int)dst + i * SPU_BLOCK * 2 + SPU_BLOCK),
		(void *)((int)src + i * SPU_BLOCK),
		SPU_BLOCK);
    }

    return;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* This file ends here, DON'T ADD STUFF AFTER THIS */
