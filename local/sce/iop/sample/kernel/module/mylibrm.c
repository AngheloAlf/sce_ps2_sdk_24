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
 *                         mylibrm.c
 *                         develop library
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       2.3.0          May,19,2001     isii
 */

#include <stdio.h>
#include <kernel.h>

#define MYNAME "mylibrm"
ModuleInfo Module = { MYNAME, 0x0101 };

int module_start(int argc, char *argv[]);
int module_stop(int argc, char *argv[]);

/* ================================================================
 * 	削除可能モジュールとしてのエントリポイント
 * ================================================================ */

int MyLibInit(int argc, char *argv[])
{
    if( argc >= 0 ) 	return module_start(argc,argv);
    else		return module_stop(-argc,argv);
}

/* ================================================================
 * 	常駐ライブラリとしての初期化エントリ
 * ================================================================ */

int module_start(int argc, char *argv[])
{
    int err, oldei;
    extern libhead mylib_entry;

    printf("'%s' Start\n", MYNAME);
    /* モジュールの常駐のための初期化、登録などを行う。*/
    CpuSuspendIntr(&oldei);
    err = RegisterLibraryEntries(&mylib_entry);
    CpuResumeIntr(oldei);
    if( err != KE_OK ) {
	printf("'%s' no resident\n", MYNAME);
	return NO_RESIDENT_END;
    }
    printf("'%s' removable resident\n", MYNAME);
    return REMOVABLE_RESIDENT_END; /* 削除可能の常駐を通知する */
}

/* ================================================================
 * 	常駐ライブラリとしての終了エントリ
 * ================================================================ */

int module_stop(int argc, char *argv[])
{
    int err, oldei;
    extern libhead mylib_entry;

    printf("'%s' Stop\n", MYNAME);
    CpuSuspendIntr(&oldei);
    err = ReleaseLibraryEntries(&mylib_entry);
    CpuResumeIntr(oldei);
    if( err == KE_OK  || err == KE_LIBRARY_NOTFOUND ) {
	/* モジュールの常駐終了のための後処理、登録の解除などを行う。*/
	printf("'%s' stopped\n", MYNAME);
	return NO_RESIDENT_END; /* 停止完了を通知する */
    } else {
	if( err == KE_LIBRARY_INUSE ) {
	    printf("'%s' library inuse\n", MYNAME);
	}
	/* 常駐し続ける為になんらかの処理/登録が必要ならおこなう。*/
	printf("'%s' can't stop. removable resident end\n", MYNAME);
	return REMOVABLE_RESIDENT_END; /* 停止不能を通知する */
    }
}

void libentry1(char *name)
{
    unsigned long oldgp;
    asm volatile( "  move %0, $gp; la  $gp, _gp" : "=r" (oldgp));

    printf("        %s --> %s:libentry1()\n", name, MYNAME);

    asm volatile( "  move $gp, %0"  : : "r" (oldgp));
}

void internal_libentry2(char *name)
{
    unsigned long oldgp;
    asm volatile( "  move %0, $gp; la  $gp, _gp" : "=r" (oldgp));

    printf("        %s --> %s:libentry2()\n", name, MYNAME);

    asm volatile( "  move $gp, %0"  : : "r" (oldgp));
}
