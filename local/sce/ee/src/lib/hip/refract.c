/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: refract.c,v 1.3 2002/05/20 02:07:23 kaneko Exp $	*/
#include <eekernel.h>
#include <stdlib.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>
#include <math.h>


typedef struct{
  sceHiPlugMicroData_t         *microD;
  int zdepth;
  int color_format;
  int tex_size;
} Refract;

#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)

static const sceHiType micropt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_MICRO, SCE_HIG_PLUGIN_STATUS, SCE_HIP_MICRO, SCE_HIP_REVISION };

/***************************************************
 * Local Errors
 ***************************************************/
typedef enum {
    _NO_HEAP,
    _NO_MICRO_PLUG,
    _NO_MICRO_DATA,
    _STACK_NULL
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
};

static const char *_refle_err_mes[] = {
    "HiP Refract : can't allocate memory from HiG Heap\n",
    "HiP Refract : can't find micro-plug\n",
    "HiP Refract : can't find micro-data\n",
    "HiP Refract : plug stack is null\n"
};

/* error handling */
static sceHiErr _hip_refle_err(_local_err err)
{
    sceHiErrState.mes = _refle_err_mes[err];
    return _local2higerr[err];
}

/************************************************/
/*		Refract Calc			*/
/************************************************/
static void RefractCalc(Refract *refract, sceHiPlug *plug)
{
    sceVu0FVECTOR	*camera_pos;
    sceVu0FVECTOR	*camera_zdir;
    sceVu0FVECTOR	*camera_up;
    sceVu0FVECTOR	vertical;
    sceVu0FMATRIX	world_view;
    sceHiPlugRefractPreArg_t *args;

    args = (sceHiPlugRefractPreArg_t *)plug->args;
    camera_pos = args->camera_pos; 
    camera_zdir = args->camera_zdir;
    camera_up = args->camera_up;
    sceVu0ScaleVector(vertical, *camera_up, -1.0f);
    sceVu0CameraMatrix(world_view, *camera_pos, *camera_zdir, vertical);
    sceVu0CopyMatrix(refract->microD->mtx.wview, world_view);

    refract->microD->refidx = args->refract_index;
    refract->microD->zoom = args->zoom;
    refract->microD->zshift = args->z_shift;
}

/************************************************/
/*		Refract Init			*/
/************************************************/
static sceHiErr RefractInit(Refract *refract, sceHiPlug *plug)
{
    sceHiErr	err;
    sceHiPlug	*microP;

    err = sceHiGetInsPlug(plug, &microP, micropt);
    if(err != SCE_HIG_NO_ERR)
	return _hip_refle_err(_NO_MICRO_PLUG);

    refract->microD = sceHiPlugMicroGetData(microP);
    if (refract->microD == NULL)
	return _hip_refle_err(_NO_MICRO_DATA);

    return SCE_HIG_NO_ERR;
}

/************************************************/
/*		Refract Plug			*/
/************************************************/
sceHiErr sceHiPlugRefract(sceHiPlug *plug, int process)
{
    sceHiErr err;
    Refract *refract;

    switch(process){
      case SCE_HIG_INIT_PROCESS:
	  refract = (Refract *)sceHiMemAlign(16, sizeof(Refract));
	  if(refract == NULL) { return _hip_refle_err(_NO_HEAP); }
	  err = RefractInit(refract, plug);
	  if(err != SCE_HIG_NO_ERR){
	     sceHiMemFree((u_int *)Paddr(refract));
	     return err;
	  }
	  plug->stack = (u_int)refract;
	  plug->args = NULL;
	  break;

      case SCE_HIG_PRE_PROCESS:
	  refract=(Refract *)plug->stack;
	  if(refract == NULL) return _hip_refle_err(_STACK_NULL);
	  if(plug->args != NULL){
	      RefractCalc(refract, plug);
	  }
	  plug->args = NULL;
	  break;

      case SCE_HIG_POST_PROCESS:
	  break;

      case SCE_HIG_END_PROCESS:
	  refract=(Refract *)plug->stack;
	  if(refract == NULL) return _hip_refle_err(_STACK_NULL);
	  sceHiMemFree((u_int *)Paddr(refract));
	  plug->stack = NULL;
	  plug->args = NULL;
	  break;
    }

    return SCE_HIG_NO_ERR;
}


