/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: fisheye.c,v 1.3 2002/05/20 02:07:22 kaneko Exp $	*/
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
    u_int			padding[3];
} FishEye;

#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)

static const sceHiType micropt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_MICRO, SCE_HIG_PLUGIN_STATUS, SCE_HIP_MICRO_PLUG, SCE_HIP_REVISION };

/***************************************************
 * Local Errors
 ***************************************************/
typedef enum {
    _NO_HEAP,
    _NO_MICRO,
    _MICRO_DATA_ERR,
    _PLGBLK_STACK_BROKEN
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
};

static const char *_fish_err_mes[] = {
    "HiP FishEye : can't allocate memory from HiG Heap\n",
    "HiP FishEye : can't find micro-plug",
    "HiP FishEye : can't find micro-data",
    "HiP FishEye : plug stack is null\n"
};

/* error handling */
static sceHiErr _hip_fish_err(_local_err err)
{
    sceHiErrState.mes = _fish_err_mes[err];
    return _local2higerr[err];
}
/************************************************/
/*		FishEye Calc			*/
/************************************************/
static void FishEyeCalc(FishEye *fisheye, sceHiPlug *plug)
{
    sceVu0FVECTOR               *camera_pos;
    sceVu0FVECTOR               *camera_zdir;
    sceVu0FVECTOR               *camera_up;
    sceVu0FMATRIX               world_view;
    float	tex_size;

    camera_pos=((sceHiPlugFishEyePreArg_t *)plug->args)->camera_pos; 
    camera_zdir=((sceHiPlugFishEyePreArg_t *)plug->args)->camera_zdir;
    camera_up=((sceHiPlugFishEyePreArg_t *)plug->args)->camera_up; 
    tex_size=((sceHiPlugFishEyePreArg_t *)plug->args)->tex_size;
    sceVu0CameraMatrix(world_view, *camera_pos, *camera_zdir, *camera_up);
    sceVu0CopyMatrix(fisheye->microD->mtx.wview, world_view);

    sceVu0CopyVector(*(sceVu0FVECTOR *)&fisheye->microD->camx, *camera_pos);
    fisheye->microD->clp.fisheye.texsize = tex_size / 2.0f;
}

/************************************************/
/*		FishEye Init			*/
/************************************************/
static sceHiErr FishEyeInit(FishEye *fisheye, sceHiPlug *plug)
{
    sceHiErr	err;
    sceHiPlug	*microp;
    float	rmin, rmax, near_, far_;
    u_int	zdepth;

    err = sceHiGetInsPlug(plug, &microp, micropt);
    if (err != SCE_HIG_NO_ERR)
	return _hip_fish_err(_NO_MICRO);
    fisheye->microD = sceHiPlugMicroGetData(microp);
    if (fisheye->microD == NULL)
	return _hip_fish_err(_NO_MICRO);

    if (plug->args != NULL) {
	zdepth = ((sceHiPlugFishEyeInitArg_t *)plug->args)->zdepth;
	rmax = ((sceHiPlugFishEyeInitArg_t *)plug->args)->rmax;
	rmin = ((sceHiPlugFishEyeInitArg_t *)plug->args)->rmin;
	switch (zdepth) {
	  case SCE_GS_PSMZ32:
	    far_ = 4294967296;
	    break;
	  case SCE_GS_PSMZ24:
	    far_ = 16777216;
	    break;
	  case SCE_GS_PSMZ16:
	  case SCE_GS_PSMZ16S:
	    far_ = 65536;
	    break;
	  default:
	    far_ = 16777216;
	    break;
	}
    } else {
	far_ = 16777216;
	rmax = 16000000;
	rmin = 0.0;
    }

    near_ = 1;
    fisheye->microD->clp.fisheye.ZA = (rmax * near_ - rmin * far_) / (near_ - far_);
    fisheye->microD->clp.fisheye.ZB = -(rmax * near_ * far_ - rmin * near_ * far_) / (near_ - far_);

    return SCE_HIG_NO_ERR;
}

/************************************************/
/*		FishEye Plug			*/
/************************************************/
sceHiErr sceHiPlugFishEye(sceHiPlug *plug, int process)
{
    sceHiErr err;
    FishEye *fisheye;

    switch(process){
    case SCE_HIG_INIT_PROCESS:
      fisheye = (FishEye *)sceHiMemAlign(16, sizeof(FishEye));
      if(fisheye == NULL) { return _hip_fish_err(_NO_HEAP); }
      err = FishEyeInit(fisheye, plug);
      if(err != SCE_HIG_NO_ERR){
	  sceHiMemFree((u_int *)Paddr(fisheye));
	  return err;
      }
      plug->args = NULL;
      plug->stack = (u_int)fisheye;

      break;
    case SCE_HIG_PRE_PROCESS:
      fisheye=(FishEye *)plug->stack;
      if(fisheye == NULL) return _hip_fish_err(_PLGBLK_STACK_BROKEN);
      if(plug->args != NULL){
	  FishEyeCalc(fisheye, plug);
      }
      plug->args=NULL;
      break;
  
    case SCE_HIG_POST_PROCESS:
      break;
        
    case SCE_HIG_END_PROCESS:
      fisheye=(FishEye *)plug->stack;
      if(fisheye == NULL) return _hip_fish_err(_PLGBLK_STACK_BROKEN);
      sceHiMemFree((u_int *)Paddr(fisheye));
      plug->args=NULL;
      plug->stack = NULL;
     break;
    }
    
    return SCE_HIG_NO_ERR;
}


