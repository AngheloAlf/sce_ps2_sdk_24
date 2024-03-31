/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: reflect.c,v 1.3 2002/05/20 02:07:23 kaneko Exp $	*/
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
} Reflect;

#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)

static const sceHiType micropt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_MICRO, SCE_HIG_PLUGIN_STATUS, SCE_HIP_MICRO_PLUG, SCE_HIP_REVISION };

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
    "HiP Reflect : can't allocate memory from HiG Heap\n",
    "HiP Reflect : can't find micro-plug\n",
    "HiP Reflect : can't find micro-data\n",
    "HiP Reflect : plug stack is null.\n"
};

/* error handling */
static sceHiErr _hip_refle_err(_local_err err)
{
    sceHiErrState.mes = _refle_err_mes[err];
    return _local2higerr[err];
}

/************************************************/
/*		Reflect Calc			*/
/************************************************/
static void ReflectCalc(Reflect *reflect, sceHiPlug *plug)
{
    sceVu0FVECTOR	*camera_pos;
    sceVu0FVECTOR	*camera_zdir;
    sceVu0FVECTOR	*camera_up;
    sceVu0FVECTOR	vertical;
    sceVu0FMATRIX	world_view;
    sceHiPlugReflectPreArg_t	*args;

    args = (sceHiPlugReflectPreArg_t *)plug->args;
    camera_pos = args->camera_pos;
    camera_zdir = args->camera_zdir;
    camera_up = args->camera_up;

    sceVu0ScaleVector(vertical, *camera_up, -1.0f);
    sceVu0CameraMatrix(world_view, *camera_pos, *camera_zdir, vertical);
    sceVu0CopyMatrix(reflect->microD->mtx.wview, world_view);
    reflect->microD->zoom = args->zoom;
    reflect->microD->zshift = args->z_shift;
}

/************************************************/
/*		Reflect Init			*/
/************************************************/
static sceHiErr ReflectInit(Reflect *reflect, sceHiPlug *plug)
{
    sceHiErr	err;
    sceHiPlug	*microP;

    err = sceHiGetInsPlug(plug, &microP, micropt);
    if (err != SCE_HIG_NO_ERR)
	return _hip_refle_err(_NO_MICRO_PLUG);

    reflect->microD = sceHiPlugMicroGetData(microP);
    if (reflect->microD == NULL)
	return _hip_refle_err(_NO_MICRO_DATA);
    return SCE_HIG_NO_ERR;
}

/************************************************/
/*		Reflect Plug			*/
/************************************************/
sceHiErr sceHiPlugReflect(sceHiPlug *plug, int process)
{
    sceHiErr err;
    Reflect *reflect;

    switch(process){
      case SCE_HIG_INIT_PROCESS:
	  reflect = (Reflect *)sceHiMemAlign(16, sizeof(Reflect));
	  if(reflect == NULL)
	      return _hip_refle_err(_NO_HEAP);
	  err = ReflectInit(reflect, plug);
	  if(err != SCE_HIG_NO_ERR){
	     sceHiMemFree((u_int *)Paddr(reflect));
	     return err;
	  }
	  plug->stack = (u_int)reflect;
	  plug->args = NULL;
	  break;

      case SCE_HIG_PRE_PROCESS:
	  reflect = (Reflect *)plug->stack;
	  if(reflect == NULL)
	      return _hip_refle_err(_STACK_NULL);
	  if(plug->args != NULL){
	      ReflectCalc(reflect, plug);
	  }
	  plug->args = NULL;
	  break;

      case SCE_HIG_POST_PROCESS:
	  break;

      case SCE_HIG_END_PROCESS:
	  reflect=(Reflect *)plug->stack;
	  if(reflect == NULL) return _hip_refle_err(_STACK_NULL);
	  sceHiMemFree((u_int *)Paddr(reflect));
	  plug->stack = NULL;
	  plug->args = NULL;
	  break;
    }

    return SCE_HIG_NO_ERR;
}


