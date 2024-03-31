/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *                   I/O Proseccor sample Library
 *                          Version 0.50
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                       rs2drv.irx - rsd_main.c
 *                           entry function
 *
 *   Version   Date            Design    Log
 *  --------------------------------------------------------------------
 *   0.30      Jun.17.1999     morita    first checked in
 *   0.50      Aug.18.1999     morita    SpuStSetCore etc. added.
 *                                       rewrite for new siflib.
 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>

ModuleInfo Module = {"rspu2_driver", 0x0102 };

#define BASE_priority  32

extern int sce_spu2_loop();
extern volatile int gStThid;

int create_th();

int start ()
{
	struct ThreadParam param;
	int th;

	FlushDcache();
	CpuEnableIntr();

	printf("rspu2 driver module version 1.2.1 (C)SCEI\n");

	/* SPU2 thread start */
	param.attr         = TH_C;
	param.entry        = sce_spu2_loop;
	param.initPriority = BASE_priority - 8; // 24
	param.stackSize    = 0x800;
	param.option       = 0;
	th = CreateThread(&param);
	if (th > 0) {
		StartThread(th,0);
		printf(" Exit rsd_main_main \n");
		return 0;
	}else{
		return 1;
	}
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */

