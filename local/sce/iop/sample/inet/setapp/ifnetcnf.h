/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4.2
 */
/*
 *      Inet Setting Application Sample
 *
 *                          Version 1.4
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                       All Rights Reserved.
 *
 *                          ifnetcnf.h
 *
 *        Version       Date             Design     Log
 *  --------------------------------------------------------------------
 *        1.1           2000.12.22       tetsu      Initial
 *        1.2           2001.01.31       tetsu      Change Final Flow
 *        1.3           2001.03.11       tetsu      Change for HDD
 *        1.4           2001.03.16       tetsu      Change for PPPoE
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <kernel.h>
#include <sys/file.h>
#include <memory.h>
#include <sif.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <netcnf.h>
#include <inet/inet.h>
#include <inet/inetctl.h>
#include <inet/netdev.h>
#include <usb.h>
#include <usbd.h>
#include <usbmload.h>

#include "/usr/local/sce/ee/sample/inet/setapp/common.h" /* 絶対 PATH */
//#include "../../../../ee/sample/inet/setapp/common.h" /* 相対 PATH */

/****************/
/* 外部参照領域 */
/****************/
extern sceNetCnfEnv_t env;
extern u_char mem_area[0x1000];

/********************/
/* 関数プロトタイプ */
/********************/
int read_env(int, int, char *, char *, int);
int write_env(int, int, char *, int);
int update_env(int, int);
#ifdef DEBUG
void dump_data(sceNetCnfEnvData_t *);
void dump_env(sceNetCnfEnv_t *);
void list_dump(sceNetCnfList2_t *, int);
#endif /* DEBUG */
