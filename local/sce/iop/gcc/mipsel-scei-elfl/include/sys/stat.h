/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5
 */
/* $Id: stat.h,v 1.1.34.1 2002/02/19 10:34:45 xokano Exp $ */

/*
 *                     I/O Processor System Services
 *
 *      Copyright (C) 1998-2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                         stat.h
 *                         IO manager interface
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       1.00           2000/10/18      koji
 */

#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus) ||defined(c_plusplus)
extern "C" {
#endif

/* Filetypes and Protection bits for pfs */
#define SCE_STM_FMT		(0xf  <<  12)
#define SCE_STM_FLNK		(0x4  <<  12)	/* symbolic link */
#define SCE_STM_FREG		(0x2  <<  12)	/* regular file */
#define SCE_STM_FDIR		(0x1  <<  12)	/* directory */
#define SCE_STM_ISLNK(m)	(((m) & SCE_STM_FMT) == SCE_STM_FLNK)
#define SCE_STM_ISREG(m)	(((m) & SCE_STM_FMT) == SCE_STM_FREG)
#define SCE_STM_ISDIR(m)	(((m) & SCE_STM_FMT) == SCE_STM_FDIR)

#define SCE_STM_SUID		04000		/* set uid bit */
#define SCE_STM_SGID		02000		/* set gid bit */
#define SCE_STM_SVTX		01000		/* sticky bit  */

#define SCE_STM_RWXU		00700
#define SCE_STM_RUSR		00400
#define SCE_STM_WUSR		00200
#define SCE_STM_XUSR		00100

#define SCE_STM_RWXG		00070
#define SCE_STM_RGRP		00040
#define SCE_STM_WGRP		00020
#define SCE_STM_XGRP		00010

#define SCE_STM_RWXO		00007
#define SCE_STM_ROTH		00004
#define SCE_STM_WOTH		00002
#define SCE_STM_XOTH		00001

#define SCE_STM_ALLUGO	\
	(SCE_STM_SUID|SCE_STM_SGID|SCE_STM_SVTX|SCE_STM_RWXUGO)
#define SCE_STM_RWXUGO		(SCE_STM_RWXU|SCE_STM_RWXG|SCE_STM_RWXO)
#define SCE_STM_RUGO		(SCE_STM_RUSR|SCE_STM_RGRP|SCE_STM_ROTH)
#define SCE_STM_WUGO		(SCE_STM_WUSR|SCE_STM_WGRP|SCE_STM_WOTH)
#define SCE_STM_XUGO		(SCE_STM_XUSR|SCE_STM_XGRP|SCE_STM_XOTH)


/* Filetypes and Protection bits for memory card */
#define SCE_STM_R	0x0001
#define SCE_STM_W	0x0002
#define SCE_STM_X	0x0004
#define SCE_STM_C	0x0008
#define SCE_STM_F	0x0010
#define SCE_STM_D	0x0020


/* for chstat cbit */
#define SCE_CST_MODE	0x0001
#define SCE_CST_ATTR	0x0002
#define SCE_CST_SIZE	0x0004
#define SCE_CST_CT	0x0008
#define SCE_CST_AT	0x0010
#define SCE_CST_MT	0x0020
#define SCE_CST_PRVT	0x0040


struct sce_stat {
	unsigned int	st_mode;	/* �t�@�C���̎��(file/dir) */
					/* �ƃ��[�h(R/W/X) */
	unsigned int	st_attr;	/* �f�o�C�X�ˑ��̑��� */
	unsigned int	st_size;	/* �t�@�C���T�C�Y ���� 32 bit */
	unsigned char	st_ctime[8];	/* �쐬���� */
	unsigned char	st_atime[8];	/* �ŏI�Q�Ǝ��� */
	unsigned char	st_mtime[8];	/* �ŏI�ύX���� */
	unsigned int	st_hisize;	/* �t�@�C���T�C�Y ��� 32bit */
	unsigned int	st_private[6];	/* ���̑� */
};


extern int getstat(const char *name, struct sce_stat *buf);
extern int chstat(const char *name, struct sce_stat *buf, unsigned int cbit);

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus) ||defined(c_plusplus)
}
#endif

#endif /* _SYS_STAT_H */

/* End of File */
