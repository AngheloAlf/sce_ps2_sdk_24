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
 * 	�폜�\���W���[���Ƃ��ẴG���g���|�C���g
 * ================================================================ */

int MyLibInit(int argc, char *argv[])
{
    if( argc >= 0 ) 	return module_start(argc,argv);
    else		return module_stop(-argc,argv);
}

/* ================================================================
 * 	�풓���C�u�����Ƃ��Ă̏������G���g��
 * ================================================================ */

int module_start(int argc, char *argv[])
{
    int err, oldei;
    extern libhead mylib_entry;

    printf("'%s' Start\n", MYNAME);
    /* ���W���[���̏풓�̂��߂̏������A�o�^�Ȃǂ��s���B*/
    CpuSuspendIntr(&oldei);
    err = RegisterLibraryEntries(&mylib_entry);
    CpuResumeIntr(oldei);
    if( err != KE_OK ) {
	printf("'%s' no resident\n", MYNAME);
	return NO_RESIDENT_END;
    }
    printf("'%s' removable resident\n", MYNAME);
    return REMOVABLE_RESIDENT_END; /* �폜�\�̏풓��ʒm���� */
}

/* ================================================================
 * 	�풓���C�u�����Ƃ��Ă̏I���G���g��
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
	/* ���W���[���̏풓�I���̂��߂̌㏈���A�o�^�̉����Ȃǂ��s���B*/
	printf("'%s' stopped\n", MYNAME);
	return NO_RESIDENT_END; /* ��~������ʒm���� */
    } else {
	if( err == KE_LIBRARY_INUSE ) {
	    printf("'%s' library inuse\n", MYNAME);
	}
	/* �풓��������ׂɂȂ�炩�̏���/�o�^���K�v�Ȃ炨���Ȃ��B*/
	printf("'%s' can't stop. removable resident end\n", MYNAME);
	return REMOVABLE_RESIDENT_END; /* ��~�s�\��ʒm���� */
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
