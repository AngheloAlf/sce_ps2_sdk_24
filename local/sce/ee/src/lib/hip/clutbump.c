/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: clutbump.c,v 1.3 2002/05/20 02:07:22 kaneko Exp $	*/
#include <eekernel.h>
#include <stdlib.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

#define	NTABLE	256

/*******************************************************
 *	ClutBump Structure
 *******************************************************/
typedef struct{
    sceVu0FMATRIX *matrix;
    sceVu0FVECTOR *normal_table;
    u_int         *clut_table;
    u_int	  padding;
} ClutGroup;

typedef struct{
    u_int	nclut;
    ClutGroup	*clutgroup;
    u_int	padding[2];
} ClutBump;

static const sceHiType clutt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_BUMP, SCE_HIG_DATA_STATUS, SCE_HIP_CLUTBUMP_DATA, SCE_HIP_REVISION };
static const sceHiType normt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_BUMP, SCE_HIG_DATA_STATUS, SCE_HIP_CLUTBUMP_NORMAL, SCE_HIP_REVISION };
static const sceHiType bmatt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHAPE, SCE_HIG_DATA_STATUS, SCE_HIP_BASEMATRIX, SCE_HIP_REVISION };
static const sceHiType tex2dt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_TEX2D, SCE_HIG_DATA_STATUS, SCE_HIP_TEX2D_DATA, SCE_HIP_REVISION };

/***************************************************
 * Local Errors
 ***************************************************/
typedef enum {
    _NO_HEAP,
    _CLUTBUMP_DATA_ERR,
    _TEX2D_DATA_ERR,
    _BASEMATRIX_DATA_ERR,
    _NORMAL_DATA_ERR,
    _PLGBLK_STACK_BROKEN
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
};

static const char *_clutbump_err_mes[] = {
    "HiP Clut-bump : can't allocate memory\n",
    "HiP Clut-bump : CLUTBUMP_DATA error\n",
    "HiP Clut-bump : TEX2D_DATA error\n",
    "HiP Clut-bump : BASEMATRIX error\n",
    "HiP Clut-bump : NORMAL_DATA error\n",
    "HiP Clut-bump : the plugin stack is null\n"
};

/* error handling */
static sceHiErr _hip_clutbump_err(_local_err err)
{
    sceHiErrState.mes = _clutbump_err_mes[err];
    return _local2higerr[err];
}


/*******************************************************
 *	ClutBump PreCalc
 *******************************************************/
static void calc_clut(ClutBump *clutbump, sceHiPlugClutBumpPreArg_t *arg)
{
    int i,j,k,factor;
    u_char  *pwrite;
    sceVu0FVECTOR local_light;
    sceVu0FMATRIX world_local;
    sceVu0FVECTOR *normal_p;
    float result;
    sceVu0IVECTOR color;
    float alpha;
    float specular;
    sceHiPlugClutBumpPreArg_t  arg_pre = {
	{0.0f,0.0f,1.0f,0.0f},{0.0f,0.5f,0.0f,0.0f}
    };

    if(arg != NULL){
	sceVu0CopyVector(arg_pre.light_dir,arg->light_dir);
	sceVu0CopyVector(arg_pre.shading, arg->shading);
    }

    for (i=0;i<clutbump->nclut;i++) {
	sceVu0TransposeMatrix(world_local,*(clutbump->clutgroup[i].matrix));
	sceVu0ApplyMatrix(local_light,world_local,arg_pre.light_dir);
	sceVu0Normalize(local_light,local_light); 
	sceVu0ScaleVector(local_light,local_light,-1.0);
	pwrite=(u_char *)clutbump->clutgroup[i].clut_table;
	normal_p=(sceVu0FVECTOR *)  clutbump->clutgroup[i].normal_table;
	for (j=0;j<NTABLE;j++){
	    result=sceVu0InnerProduct(local_light,*normal_p);
	    if(result<0) result=0.0;
	    if(arg_pre.shading[2] != 0.0f) {
		specular=result;
		factor=(int)arg_pre.shading[3];
		for(k=0;k<factor;k++){
		    specular=specular*specular;
		}
		//alpha=arg_pre.shading[0]  + arg_pre.shading[1] * result *255 + arg_pre.shading[2] * pow(result,arg_pre.shading[3])*255;
		alpha=arg_pre.shading[0]  + arg_pre.shading[1] * result *255 + arg_pre.shading[2] * specular*255;
	    } else {
		alpha=arg_pre.shading[0]  + arg_pre.shading[1] * result*255;
	    }
	    if (alpha>255) alpha=255;
	    color[3]=(int) alpha;
	    if (color[3]> 255) color[3]=255;
	    if(color[3]<0) color[3]=0;
	    pwrite[3]=(u_char) color[3];
	    pwrite+=4;
	    normal_p++;
	}
    }
}

/*******************************************************
 *	ClutBump Init
 *******************************************************/
#define QWSIZE 4
#define NUMIDX 3

static sceHiErr ClutBumpInit(ClutBump *clutbump, sceHiPlug *plug)
{
    sceHiPlugClutBumpHead_t	*clut_head;
    sceHiPlugClutBumpHead_t	*norm_head;
    sceHiPlugShapeHead_t	*bmat_head;
    sceHiPlugTex2DHead_t	*tex2d_head;
    sceHiPlugClutBumpData_t	*clut_data;
    sceHiPlugShapeMatrix_t	*bmat_data;
    sceHiPlugTex2DData_t	*tex2d_data;
    int i, j;

    clut_head = sceHiPlugClutBumpGetHead(plug, clutt);
    if (clut_head == NULL) return _hip_clutbump_err(_CLUTBUMP_DATA_ERR);
    norm_head = sceHiPlugClutBumpGetHead(plug, normt);
    if (norm_head == NULL) return _hip_clutbump_err(_NORMAL_DATA_ERR);
    bmat_head = sceHiPlugShapeGetHead(plug, bmatt);
    if (bmat_head == NULL) return _hip_clutbump_err(_BASEMATRIX_DATA_ERR);
    tex2d_head = sceHiPlugTex2DGetHead(plug, tex2dt);
    if (tex2d_head == NULL) return _hip_clutbump_err(_TEX2D_DATA_ERR);

    clutbump->nclut = clut_head->num;
    if (clutbump->nclut <= 0)
	return _hip_clutbump_err(_CLUTBUMP_DATA_ERR);
    clutbump->clutgroup = (ClutGroup *)sceHiMemAlign(16, sizeof(ClutGroup) * clutbump->nclut);
    if (clutbump->clutgroup == NULL)
	return _hip_clutbump_err(_NO_HEAP);

    /* search light matrix */
    for (i = 0; i < clutbump->nclut; i++) {
	clut_data = sceHiPlugClutBumpGetData(clut_head, i);
	if (clut_data == NULL) return _hip_clutbump_err(_CLUTBUMP_DATA_ERR);
	for (j = 0; j < bmat_head->top.num; j++) {
	    bmat_data = sceHiPlugShapeGetMatrix(bmat_head, j);
	    if (bmat_data == NULL) return _hip_clutbump_err(_BASEMATRIX_DATA_ERR);
	    if (clut_data->shape == bmat_data->shape) {
		clutbump->clutgroup[i].matrix = &bmat_data->light;
		break;
	    }
	}
	if (j == bmat_head->top.num)
	    return _hip_clutbump_err(_CLUTBUMP_DATA_ERR);
    }

    /* search clut address */
    for (i = 0; i < clutbump->nclut; i++) {
	clut_data = sceHiPlugClutBumpGetData(clut_head, i);
	if (clut_data == NULL) return _hip_clutbump_err(_CLUTBUMP_DATA_ERR);
	if (clut_data->tex2d >= tex2d_head->num)
	    return _hip_clutbump_err(_CLUTBUMP_DATA_ERR);
	tex2d_data = sceHiPlugTex2DGetData(tex2d_head, clut_data->tex2d);
	if (tex2d_data == NULL) return _hip_clutbump_err(_TEX2D_DATA_ERR);
	clutbump->clutgroup[i].clut_table = sceHiPlugTex2DGetClut(tex2d_data);
	if (clutbump->clutgroup[i].clut_table == NULL)
	    return _hip_clutbump_err(_TEX2D_DATA_ERR);
    }

    /* search normal table */
    for (i = 0; i < clutbump->nclut; i++) {
	clut_data = sceHiPlugClutBumpGetData(clut_head, i);
	if (clut_data == NULL) return _hip_clutbump_err(_CLUTBUMP_DATA_ERR);
	if (clut_data->normal >= norm_head->num)
	    return _hip_clutbump_err(_CLUTBUMP_DATA_ERR);
	clutbump->clutgroup[i].normal_table = sceHiPlugClutBumpGetNormal(norm_head, clut_data->normal);
	if (clutbump->clutgroup[i].normal_table == NULL)
	    return _hip_clutbump_err(_NORMAL_DATA_ERR);
    }

    return SCE_HIG_NO_ERR;
}

/*******************************************************
 *	ClutBump Plug
 *******************************************************/
sceHiErr sceHiPlugClutBump(sceHiPlug *plug, int process)
{
    sceHiErr	err;
    ClutBump	*clutbump;

    switch(process){
      case SCE_HIG_INIT_PROCESS:
	  clutbump = (ClutBump *)sceHiMemAlign(16, sizeof(ClutBump));
	  if(clutbump == NULL) { return _hip_clutbump_err(_NO_HEAP);}
	  clutbump->clutgroup = NULL;
	  err = ClutBumpInit(clutbump, plug);
	  if(err != SCE_HIG_NO_ERR){
	      if(clutbump->clutgroup != NULL) sceHiMemFree((u_int *)((u_int)(clutbump->clutgroup) & 0x0fffffff));
	      sceHiMemFree((u_int *)((u_int)clutbump & 0x0fffffff));
	      return err;
	  }

	  plug->args = NULL;
	  plug->stack = (u_int)clutbump;
	  break;

      case SCE_HIG_PRE_PROCESS:
	  clutbump = (ClutBump *)plug->stack;
	  if(clutbump == NULL) return _hip_clutbump_err(_PLGBLK_STACK_BROKEN);
	  calc_clut(clutbump, (sceHiPlugClutBumpPreArg_t *)plug->args);
	  plug->args = NULL;
	  break;

      case SCE_HIG_POST_PROCESS:
	  break;

      case SCE_HIG_END_PROCESS:
	  clutbump = (ClutBump *)plug->stack;
	  if(clutbump == NULL) return _hip_clutbump_err(_PLGBLK_STACK_BROKEN);
	  sceHiMemFree((u_int *)((u_int)clutbump & 0x0fffffff));
	  plug->stack = NULL;
	  plug->args = NULL;
	  break;

      default:
	  break;
    }

    return SCE_HIG_NO_ERR;
}
