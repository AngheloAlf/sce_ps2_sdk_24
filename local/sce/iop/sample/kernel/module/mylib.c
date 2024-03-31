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
 *                         mylib.c
 *                         develop library
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       2.3.0          May,19,2001     isii
 */

#include <stdio.h>
#include <kernel.h>

/* �������ɏ풓���郂�W���[����, �ȉ��̂悤�Ƀ��W���[������
 * ���W���[���o�[�W������t���Ă����ƕ֗��ł��B
 */
#define MYNAME "mylib"
ModuleInfo Module = { MYNAME, 0x0101 };

/* ================================================================
 * 	�풓���C�u�����Ƃ��Ă̏������G���g��
 * ================================================================ */

int MyLibInit()
{
    int err, oldei;
    /* mylib.tbl����, ���[�e�B���e�B loplibgen �ɂ���ăG���g���e�[�u����
     * ��������܂��B�G���g���e�[�u���̃��x�����ɂ�, '���C�u������_entry' ��
     *  �����܂��B
     */
    extern libhead mylib_entry; /* ���C�u������_entry ���Q�� */

    printf("'%s' Start\n", MYNAME);
    /* ���W���[���̏풓�̂��߂̏������A�o�^�Ȃǂ��s���B*/
    CpuSuspendIntr(&oldei);
    err = RegisterLibraryEntries(&mylib_entry);
    CpuResumeIntr(oldei);
    if( err == KE_LIBRARY_FOUND ) {
	/* ���ɓ����̏풓���C�u����������̂œo�^�Ɏ��s */
	printf("'%s' already exist. no resident\n", MYNAME);
	return NO_RESIDENT_END; /* �I�����ă���������ދ� */
    } else if( err != KE_OK ) {
	printf("'%s' What happen ?\n", MYNAME);
	return NO_RESIDENT_END; /* �I�����ă���������ދ� */
    }
    printf("'%s' resident\n", MYNAME);
    return RESIDENT_END; /* �I�����ď풓���� */
}

/* ================================================================
 * 	�풓���C�u�����̊e�G���g���̒�`
 * ================================================================ */

/*
   ���W���[���́A���ꂼ��Ǝ��� GP���W�X�^�̒l�������܂��B
   �Ƃ��낪�A���郂�W���[�����瑼�̃��W���[���̏풓���C�u������
   �Ăяo���ƁAGP���W�X�^�͌Ăяo�������W���[���� GP�l��ێ������܂�
   �Ăяo����܂��B
   �]���ď풓���C�u�������Ŏ����W���[�����̃O���[�o���f�[�^��
   �A�N�Z�X����Ƃ���肪�����܂��B
   ���̖�������邽�߂ɁA�ȉ��̂ǂ��炩�̕��@�ŏ풓���C�u������
   �쐬���Ă��������B
    1) �R���p�C���̃I�v�V���� -G 0 �ŏ풓���C�u�������R���p�C������B
       ����ɂ���ď풓���C�u������ GP ���W�X�^���g�p���Ȃ��R�[�h�ɂȂ�܂��B
       ���������̕��@�͏풓���C�u�������኱�傫���Ȃ���s���x���x���Ȃ�܂��B
    2) �풓���C�u�����̊e�G���g���֐����A���L�̂悤�ɁAGP���W�X�^��Ҕ�����
       �����W���[���� GP ���W�X�^��ݒ肷��悤�ɃR�[�f�B���O����B
      (���L�̃R�[�f�B���O�� gcc�n���̃R���p�C�����g�p�����ꍇ�̗�ł��B)
 */

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
