/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                    I/O Processor Library
 *
 *                           - ftp -
 *
 *                        Version 1.0.0
 *                          Shift-JIS
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 * hddpart.h - header file for hddpart.c
 *
 * $Id: hddpart.h,v 1.3 2001/06/19 00:34:02 ywashizu Exp $
 * $Author: ywashizu $
 *
 */

#ifndef _HDDPART_H
#define _HDDPART_H

#include <ctype.h>
#include <kernel.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <dirent.h>
#include <stdlib.h>

#define MAX_PATH                1024
#define SECTOR_SIZE             512

int showHddDir(char *dir);

void releaseHdd(char* path);

int setupHdd(char* path, char* cwd, int mode);

void updateCwd(char *p, char* cwd);



#endif 