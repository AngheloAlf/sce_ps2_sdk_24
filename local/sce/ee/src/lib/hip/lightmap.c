/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: lightmap.c,v 1.5 2002/05/20 02:07:23 kaneko Exp $	*/
#include <eekernel.h>
#include <stdlib.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>
#include <math.h>

/***************************************************
 * Local Errors
 ***************************************************/
typedef enum {
    _NO_HEAP,
    _NO_MICRO_PLUG,
    _NO_MICRO_DATA,
    _NO_STACK,
    _NO_ARGS,
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_VALUE,
};

static const char *_err_mes[] = {
    "HiP LightMap : can't allocate memory\n",
    "HiP LightMap : can't find micro plug\n",
    "HiP LightMap : can't find micro data\n",
    "HiP LightMap : plug stack is null",
    "HiP LightMap : plug args is null\n",
};

/* error handling */
static sceHiErr _hip_err(_local_err err)
{
    sceHiErrState.mes = _err_mes[err];
    return _local2higerr[err];
}

/*******************************************************
 *	LightMap Structure
 *******************************************************/
typedef struct {
    int			width;
    int			height;
    int			fov;	/* TRUE:width , FALSE:height */
    sceHiPlugMicroData_t		*mdata;
}LightMap;

enum {DIR, POINT, SPOT};

static const sceHiType micropt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_MICRO, SCE_HIG_PLUGIN_STATUS, SCE_HIP_MICRO_PLUG, SCE_HIP_REVISION };

/********************************************************/
/*	LightMap View					*/
/********************************************************/
static void lightmap_view(LightMap *lightmap)
{
    sceVu0FMATRIX m,tm;
    sceVu0FVECTOR yd={0.0f,-1.0f,0.0f,0.0f};
    sceVu0FVECTOR vec;
    float ax,ay;
    float angle;
    int i;
 
    angle = acosf(sqrtf(lightmap->mdata->light[DIR].pos[0][3]));	/* spot angle */
 
    if(lightmap->fov){
        ax = 1.0f;
        ay = (float)lightmap->width/(float)lightmap->height;
    }
    else{
        ax = (float)lightmap->height/(float)lightmap->width;
        ay = 1.0f;
    }
 
 
    for(i=0;i<4;i++){
	vec[i] = lightmap->mdata->light[DIR].dir[i][0];	/* spot vector */
    }
    sceVu0ScaleVector(vec,vec,-1.0f);
 
 
    sceVu0CopyVector(lightmap->mdata->light[DIR].dir[0], vec);
    sceVu0UnitMatrix(m);
 
    if ((vec[0]-0.00002f<0.0f) && (vec[2]-0.00002f<0.0f)) {yd[0]=-vec[1]; yd[1]=0.0f; yd[2]=0.0f;}
    sceVu0OuterProduct(yd, vec,yd);
    sceVu0Normalize(yd, yd);
    sceVu0OuterProduct(yd, vec, yd);
 
 
 
    sceVu0CameraMatrix(m, lightmap->mdata->light[DIR].pos[0], vec, yd);
    sceVu0UnitMatrix(tm);
 
    tm[0][0] = ax*0.5f / tanf(angle);
    tm[1][1] = ay*0.5f / tanf(angle);
 
 
    tm[2][0] = 0.5f;
    tm[2][1] = 0.5f;
    tm[2][2] = 0.0f;
    tm[2][3] = 1.0f;
    tm[3][2] = 0.0f;
    tm[3][3] = 0.0f;
 
    sceVu0MulMatrix(tm, tm, m);
 
    sceVu0CopyMatrix(lightmap->mdata->mtx.wview, tm);                          /* projtex matrix */
}                                                                            
/*******************************************************
 *	LightMap Plug		       
 *******************************************************/
sceHiErr sceHiPlugLightMap(sceHiPlug *plug, int process)
{
    sceHiErr	err;
    sceHiPlug	*micro;
    LightMap	*lightmap;

    switch(process){
      case SCE_HIG_INIT_PROCESS:
	err = sceHiGetInsPlug(plug, &micro, micropt);
	if(err != SCE_HIG_NO_ERR)
	    return _hip_err(_NO_MICRO_PLUG);
	
	lightmap = (LightMap *)sceHiMemAlign(16, sizeof(LightMap));
	if(lightmap == NULL) return _hip_err(_NO_HEAP);

	lightmap->mdata = sceHiPlugMicroGetData(micro);
	if (lightmap->mdata == NULL)
	    return _hip_err(_NO_MICRO_DATA);

	if(plug->args != NULL){
	    lightmap->width = ((sceHiPlugLightMapInitArg_t *)plug->args)->width;
	    lightmap->height = ((sceHiPlugLightMapInitArg_t *)plug->args)->height;
	    lightmap->fov = ((sceHiPlugLightMapInitArg_t *)plug->args)->fov;
	} else {
	    sceHiMemFree((u_int *)((u_int)lightmap & 0x0fffffff));
	    return _hip_err(_NO_ARGS);
	}

	plug->args = NULL;
	plug->stack = (u_int)lightmap;
	break;

      case SCE_HIG_PRE_PROCESS:
	lightmap = (LightMap *)plug->stack;
	if (lightmap == NULL)	return _hip_err(_NO_STACK);
	lightmap_view(lightmap);
	break;
	
      case SCE_HIG_POST_PROCESS:
	break;
	
      case SCE_HIG_END_PROCESS:
	lightmap = (LightMap *)plug->stack;
	if (lightmap == NULL)	return _hip_err(_NO_STACK);
	sceHiMemFree((u_int *)((u_int)lightmap & 0x0fffffff));
	plug->stack = NULL;
	plug->args = NULL;
	break;
	
      default:
	break;
    }
    
    return SCE_HIG_NO_ERR;
}
