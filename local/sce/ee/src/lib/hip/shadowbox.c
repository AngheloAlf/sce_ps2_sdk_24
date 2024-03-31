/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: shadowbox.c,v 1.3 2002/05/20 02:07:23 kaneko Exp $	*/
#include <eekernel.h>
#include <stdlib.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

/***************************************************
 * Local Errors
 ***************************************************/
typedef enum {
    _NO_HEAP,
    _NO_BOX_DATA,
    _NO_BASEMATRIX,
    _NO_STACK,
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
};

static const char *_err_mes[] = {
    "HiP ShadowBox : can't allocate memory\n",
    "HiP ShadowBox : can't find shadowbox data\n",
    "HiP ShadowBox : can't find basematrix data\n",
    "HiP ShadowBox : plug stack is null\n",
};

/* error handling */
static sceHiErr _hip_err(_local_err err)
{
    sceHiErrState.mes = _err_mes[err];
    return _local2higerr[err];
}

/*******************************************************
 *	ShadowBox Structure
 *******************************************************/
typedef struct {
    sceHiPlugShadowBoxData_t	*box;
    sceVu0FMATRIX		*mat;
    u_int			padding[2];
}ShadowBox;

#define QWSIZE	4
#define NUMIDX	3

static const sceHiType basemt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHAPE, SCE_HIG_DATA_STATUS, SCE_HIP_BASEMATRIX, SCE_HIP_REVISION };

/*******************************************************
 *	ShadowBox Calc
 *******************************************************/
static void shadowbox_calc(ShadowBox *sbox)
{
    int i;
    sceVu0FVECTOR box[8] = {
	{ sbox->box->max[0], sbox->box->min[1], sbox->box->max[2], 1.0f },
	{ sbox->box->max[0], sbox->box->max[1], sbox->box->max[2], 1.0f },
	{ sbox->box->max[0], sbox->box->min[1], sbox->box->min[2], 1.0f },
	{ sbox->box->max[0], sbox->box->max[1], sbox->box->min[2], 1.0f },
	{ sbox->box->min[0], sbox->box->min[1], sbox->box->max[2], 1.0f },
	{ sbox->box->min[0], sbox->box->max[1], sbox->box->max[2], 1.0f },
	{ sbox->box->min[0], sbox->box->min[1], sbox->box->min[2], 1.0f },
	{ sbox->box->min[0], sbox->box->max[1], sbox->box->min[2], 1.0f },
    };

    for(i = 0; i < 8; i++){
	sceVu0ApplyMatrix(sbox->box->box[i], *sbox->mat, box[i]);
    }
}
/*******************************************************
 *	ShadowBox Plug
 *******************************************************/
sceHiErr sceHiPlugShadowBox(sceHiPlug *plug, int process)
{
    ShadowBox	*sbox;
    sceHiPlugShadowBoxData_t	*boxP;
    sceHiPlugShapeHead_t	*matH;
    sceHiPlugShapeMatrix_t	*mat;

    switch(process){
      case SCE_HIG_INIT_PROCESS:
	boxP = sceHiPlugShadowBoxGetData(plug);
	if (boxP == NULL)	return _hip_err(_NO_BOX_DATA);
	matH = sceHiPlugShapeGetHead(plug, basemt);
	if (matH == NULL)	return _hip_err(_NO_BASEMATRIX);

	sbox = (ShadowBox *)sceHiMemAlign(16, sizeof(ShadowBox));
	if(sbox == NULL)
	    return _hip_err(_NO_HEAP);

	mat = sceHiPlugShapeGetMatrix(matH, 0);
	if (mat == NULL)	return _hip_err(_NO_BASEMATRIX);
	sbox->box = boxP;
	sbox->mat = &(mat->local); /* only root matrix */

	plug->args = NULL;
	plug->stack = (u_int)sbox;
	break;

      case SCE_HIG_PRE_PROCESS:
	  sbox = (ShadowBox *)plug->stack;
	  if(sbox == NULL)		return _hip_err(_NO_STACK);
	  shadowbox_calc(sbox);
	  break;

      case SCE_HIG_END_PROCESS:
	  sbox = (ShadowBox *)plug->stack;
	  if(sbox == NULL)		return _hip_err(_NO_STACK);
	  sceHiMemFree((u_int *)((u_int)sbox & 0x0fffffff));
	  plug->stack = NULL;
	  plug->args = NULL;
	  break;

      default:
	  break;
    }

    return SCE_HIG_NO_ERR;
}
