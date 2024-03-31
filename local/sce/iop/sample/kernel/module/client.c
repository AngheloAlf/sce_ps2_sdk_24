/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *		I/O Processor Library Sample Program
 *
 *			-- Module --
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                         client.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       2.3.0          May,19,2001     isii
 */

#include <stdio.h>
#include <kernel.h>

/* メモリに常駐しなくとも, 以下のようにモジュール名と
 * モジュールバージョンを付けることは可能です。
 */
#define MYNAME "client"
ModuleInfo Module = { MYNAME, 0x0101 };

extern void libentry1(char *name);
extern void libentry2(char *name);

int start(int argc, char *argv[])
{
    printf("'%s' Start\n", MYNAME);
    libentry1(MYNAME);
    libentry2(MYNAME);
    printf("'%s' end\n", MYNAME);
    return NO_RESIDENT_END;
}
