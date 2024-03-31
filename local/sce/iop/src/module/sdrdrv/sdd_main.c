/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/* 
 *                   I/O Proseccor sample Library
 *                          Version 0.50
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-2002 Sony Computer Entertainment Inc.
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
 *             May.14.2000     kaol      Thread priority changed.
 *   2.0.0     Jul.24.2000     kaol      Thread priority can be changed
 *                                        dynamically from outside.
 */

#include <kernel.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>		/* atoi() */
#include <ctype.h>		/* isdigit() */
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <sdrcmd.h>
#include "sdr_i.h"

ModuleInfo Module = {"sdr_driver", 0x0401 };

extern int sce_sdr_loop();	/* sdd_com.c */

int initial_priority_main;	/* ���C���X���b�h�̃v���C�I���e�B�����l */
int initial_priority_cb;	/* �R�[���o�b�N�X���b�h�̃v���C�I���e�B�����l */

int thid_main;			/* ���C���X���b�h�̃X���b�h ID */
extern volatile int gStThid;	/* �R�[���o�b�N�X���b�h�̃X���b�h ID (sdd_cb.c) */

int
start (int argc, char *argv [])
{
    /* sdrdrv.tbl����, ���[�e�B���e�B ioplibgen �ɂ���ăG���g���e�[�u����
     * ��������܂��B�G���g���e�[�u���̃��x�����ɂ�, '���C�u������_entry' ��
     * �����܂��B
     */
    extern libhead sdrdrv_entry; /* ���C�u������_entry ���Q�� */
    int code, oldei;		/* for Cpu{Suspend,Resume}Intr() */
    struct ThreadParam param;
    int i, pval;
    char *p;

    FlushDcache ();

    /*
     * ���W���[���̏풓�󋵂��`�F�b�N
     */
    CpuSuspendIntr (&oldei);
    code = RegisterLibraryEntries (&sdrdrv_entry);
    CpuResumeIntr (oldei);
    if (code != KE_OK) {
	/* ���ɓ����̏풓���C�u����������̂œo�^�Ɏ��s */
	return NO_RESIDENT_END; /* �I�����ă���������ދ� */
    }

    printf ("SDR driver version 4.0.1 (C) SCEI\n");

    /*
     * �����I�v�V����: �e�X���b�h�̃v���C�I���e�B�����l
     */
    initial_priority_main = PRIORITY_MODULE_SDRDRV;
    initial_priority_cb   = PRIORITY_MODULE_SDRDRV;
    thid_main = 0;		/* ���C���X���b�h���쐬 */
    gStThid   = 0;		/* �R�[���o�b�N�X���b�h���쐬 */

    /*
     * �����I�v�V�����̉��
     */
    for (i = 1; i < argc; i ++) {
	if (! strncmp ("thpri=", argv [i], 6)) {
	    p = argv [i] + 6; /* strlen("thpri=") == 6 */
	    if (isdigit (*p)) {
		/* ���C���X���b�h�̃v���C�I���e�B */
		pval = atoi (p);
		if (pval < USER_HIGHEST_PRIORITY ||
		    pval > USER_LOWEST_PRIORITY) {
		    printf (" SDR driver error: invalid priority %d\n", pval);
		    pval = PRIORITY_MODULE_SDRDRV;
		}
		initial_priority_main = pval;
	    }
	    while (isdigit (*p)) p ++;
	    if (*p == ',' && isdigit (*(p + 1))) {
		/* �R�[���o�b�N�X���b�h�̃v���C�I���e�B */
		p ++;
		pval = atoi (p);
		if (pval < USER_HIGHEST_PRIORITY ||
		    pval > USER_LOWEST_PRIORITY) {
		    printf (" SDR driver error: invalid priority %d\n", pval);
		    pval = PRIORITY_MODULE_SDRDRV;
		}
		initial_priority_cb = pval;
	    }
	    /* �c��̕�����͖�������� */

	    /* �w�肳�ꂽ�v���C�I���e�B�̃`�F�b�N */
	    if (initial_priority_main > initial_priority_cb) {
		/* �R�[���o�b�N�X���b�h�̃v���C�I���e�B��
		 * ���C���X���b�h�̃v���C�I���e�B���A
		 * �������Ⴍ (�l�Ƃ��Ă͑傫��) �Ȃ���΂����Ȃ� */
		printf (" SDR driver ERROR:\n");
		printf ("   callback th. priority is higher than main th. priority.\n");
		initial_priority_cb = initial_priority_main;
	    }
	    printf (" SDR driver: thread priority: main=%d, callback=%d\n",
		    initial_priority_main, initial_priority_cb);
	}
	/* else { �s���ȃI�v�V�����͒P�ɖ������� } */
    }
	
    /* sdr thread start */
    param.attr         = TH_C;
    param.entry        = sce_sdr_loop;
    param.initPriority = initial_priority_main;
    param.stackSize    = 0x800;
    param.option       = 0;
    thid_main = CreateThread (&param);
    if (thid_main > 0) {
	StartThread (thid_main, 0);	/* ���C���X���b�h���s */
	printf (" Exit rsd_main \n");
	return RESIDENT_END;		/* �������ɏ풓 */
    } else {
	return NO_RESIDENT_END;		/* �I�����ă���������ދ� */
    }
}

int
sceSdrChangeThreadPriority (int priority_main, /* main thread */
			    int priority_cb)   /* callback thread */
{
    struct ThreadInfo mythinfo;
    int ret;
    
    /*
     * �����ŗ^����ꂽ�v���C�I���e�B�l�̑Ó����𒲂ׂ�
     */
    if (priority_main < USER_HIGHEST_PRIORITY ||
	priority_main > USER_LOWEST_PRIORITY) {
	return KE_ILLEGAL_PRIORITY;
    }
    if (priority_cb < USER_HIGHEST_PRIORITY ||
	priority_cb > USER_LOWEST_PRIORITY) {
	return KE_ILLEGAL_PRIORITY;
    }
    if (priority_main > priority_cb) {
	/* �R�[���o�b�N�X���b�h�̃v���C�I���e�B�̓��C���X���b�h��
	 *  �v���C�I���e�B���A�������Ⴍ (�l�Ƃ��Ă͑傫��) �Ȃ����
	 *  �����Ȃ� */
	printf (" SDR driver ERROR:\n");
	printf ("   callback th. priority is higher than main th. priority.\n");
	priority_cb = priority_main;
    }
#if 0
    printf (" SDR driver: change thread priority: main=%d, callback=%d\n",
	    priority_main, priority_cb);
#endif

    /*
     * ���̃X���b�h�Ɏ��s����D���Ȃ��悤�ɁA��U���X���b�h��
     * �v���C�I���e�B���L�^������ɍō��ɏグ�Ă����B
     */
    ReferThreadStatus (TH_SELF, &mythinfo);
    ChangeThreadPriority (TH_SELF, MODULE_INIT_PRIORITY);
        
    /*
     * ���C�u�����̊Ǘ����̃X���b�h�Q�̃v���C�I���e�B��K�؂ȏ��Ԃ�
     * ChangeThreadPriority() ����B
     */
    /* ���C���X���b�h */
    if (thid_main > 0) {
	if ((ret = ChangeThreadPriority (thid_main, priority_main)) < 0) {
	    return ret;
	}
    } /* else { �K���쐬����Ă��� } */
    /* �R�[���o�b�N�X���b�h */
    if (gStThid > 0) {
	if ((ret = ChangeThreadPriority (gStThid, priority_cb)) < 0) {
	    return ret;
	}
    } else {
	initial_priority_cb = priority_cb; /* �쐬���̏����l�� */
    }
    
    /* ���X���b�h�̃v���C�I���e�B�����ɖ߂� */
    ChangeThreadPriority (TH_SELF, mythinfo.currentPriority);
    
    return KE_OK;
}

/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
