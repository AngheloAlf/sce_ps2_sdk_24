/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/* 
 *                      I/O Processor Library
 *                          Version 1.07
 *                           Shift-JIS
 *
 *      Copyright (C) 2000-2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                        inet - inetctl.h
 *                        INET Control I/F
 *
 * $Id: inetctl.h,v 1.9 2002/03/27 05:11:13 xokano Exp $
 */

#if !defined(_INETCTL_H)
#define _INETCTL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <netcnf.h>

typedef struct sceInetCtlEventHandlers {
	struct sceInetCtlEventHandlers *forw, *back;
	void (*func)(int id, int type);
	int gp;
} sceInetCtlEventHandlers_t;

/* type for event handler of sceInetCtlRegisterEventHandler() */
#define sceINETCTL_IEV_Attach	1	/* I/F attach */
#define sceINETCTL_IEV_Detach	2	/* I/F detach */
#define sceINETCTL_IEV_Start	3	/* I/F start (Running=1)*/
#define sceINETCTL_IEV_Stop	4	/* I/F stop  (Running=0) */
#define sceINETCTL_IEV_Error	5	/* I/F error (Error=1) */
#define sceINETCTL_IEV_Conf	6	/* Configuration found */
#define sceINETCTL_IEV_NoConf	7	/* Configuration not found */
#define sceINETCTL_IEV_Up	8	/* User Up */
#define sceINETCTL_IEV_Down	9	/* User Down */
#define sceINETCTL_IEV_Retry	10	/* Retry (redial) */

/* state code for sceInetCtlGetState() */
#define sceINETCTL_S_DETACHED	0	/* Detached */
#define sceINETCTL_S_STARTING	1	/* Starting */
#define sceINETCTL_S_RETRYING	2	/* Retrying */
#define sceINETCTL_S_STARTED	3	/* Started */
#define sceINETCTL_S_STOPPING	4	/* Stopping */
#define sceINETCTL_S_STOPPED	5	/* Stopped */

int sceInetCtlSetConfiguration(sceNetCnfEnv_t *e);
sceNetCnfEnv_t *sceInetCtlGetConfiguration(void);
int sceInetCtlUpInterface(int id);
int sceInetCtlDownInterface(int id);
int sceInetCtlSetAutoMode(int f_on);
int sceInetCtlRegisterEventHandler(struct sceInetCtlEventHandlers *eh);
int sceInetCtlUnregisterEventHandler(struct sceInetCtlEventHandlers *eh);
int sceInetCtlGetState(int id, int *pstate);

#if defined(__cplusplus)
}
#endif

#endif	/* _INETCTL_H */
