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
 *                       ezadpcm.irx - entry.c
 *                           entry function
 *
 *	Version		Date		Design	Log
 *  --------------------------------------------------------------------
 *	0.10		Feb. 3, 2000	kaol		
 *	0.11		Feb.27, 2000	kaol	entire waveform data is
 *                                               read at once.
 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include "ezadpcm.h"

ModuleInfo Module = {"ezadpcm_driver", 0x0102};

// in command.c
extern int sce_adpcm_loop (void);

// スタートエントリ
int
start (void)
{
    struct ThreadParam param;
    int th;

    sceSifInitRpc (0);

    printf ("EzADPCM driver version 0.12\n");

    param.attr         = TH_C;
    param.entry        = sce_adpcm_loop;
    param.initPriority = BASE_priority - 2;
    param.stackSize    = 0x800;
    param.option       = 0;
    th = CreateThread (&param);
    if (th > 0) {
	StartThread (th, 0);
	printf (" Exit EzADPCM loader thread \n");
	return 0;
    }else{
	return 1;
    }
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
