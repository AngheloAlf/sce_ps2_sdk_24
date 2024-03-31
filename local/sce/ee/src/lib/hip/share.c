/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: share.c,v 1.10 2002/05/20 02:07:23 kaneko Exp $	*/
#include <eekernel.h>
#include <stdio.h>
#include <eeregs.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)

#define QWSIZE 4

static const sceHiType sharedt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHARE, SCE_HIG_DATA_STATUS, SCE_HIP_SHARE_DATA, SCE_HIP_REVISION };
static const sceHiType srcdstvt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHARE, SCE_HIG_DATA_STATUS, SCE_HIP_SRCDSTVERTEX, SCE_HIP_REVISION };
static const sceHiType srcdstnt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHARE, SCE_HIG_DATA_STATUS, SCE_HIP_SRCDSTNORMAL, SCE_HIP_REVISION };
static const sceHiType vertidxt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHARE, SCE_HIG_DATA_STATUS, SCE_HIP_VERTEXINDEX, SCE_HIP_REVISION };
static const sceHiType normidxt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHARE, SCE_HIG_DATA_STATUS, SCE_HIP_NORMALINDEX, SCE_HIP_REVISION };
static const sceHiType sharevt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHARE, SCE_HIG_DATA_STATUS, SCE_HIP_SHAREVERTEX, SCE_HIP_REVISION };
static const sceHiType sharent = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHARE, SCE_HIG_DATA_STATUS, SCE_HIP_SHARENORMAL, SCE_HIP_REVISION };
static const sceHiType basemt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHAPE, SCE_HIG_DATA_STATUS, SCE_HIP_BASEMATRIX, SCE_HIP_REVISION };
static const sceHiType shapedt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHAPE, SCE_HIG_DATA_STATUS, SCE_HIP_SHARE_DATA, SCE_HIP_REVISION };

/************************************************/
/*		Structure			*/
/************************************************/
typedef struct _SHARE_FRAME{
    sceHiPlugShareHead_t	*sharedH;
    sceHiPlugShareHead_t	*srcdstvH;
    sceHiPlugShareHead_t	*srcdstnH;
    sceHiPlugShareHead_t	*vertidxH;
    sceHiPlugShareHead_t	*normidxH;
    sceHiPlugShareHead_t	*sharevH;
    sceHiPlugShareHead_t	*sharenH;
    sceHiPlugShapeHead_t	*basemH;
    sceHiPlugShapeHead_t	*shapedH;
    sceVu0FVECTOR		**pGeomVertex;
    sceVu0FVECTOR		**pGeomNormal;
    u_int			pad0;
}SHARE_FRAME;

/***************************************************
 * Local Errors
 ***************************************************/
typedef enum {
    _CANT_FIND_SHAPEV,
    _NO_HEAP,
    _PLGBLK_STACK_BROKEN,
    _SHARE_DATA_BROKEN,
    _BASEMATRIX_DATA_BROKEN,
    _SRCDST_BROKEN,
    _INDEX_BROKEN,
    _GEOMETRY_BROKEN
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_INVALID_DATA,
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA
};

static char *_share_err_mes[] = {
    "HiP Share : can't find shapev data\n",
    "HiP Share : can't allocate memory from HiG Heap\n",
    "HiP Share : plug stack is null",
    "HiP Share : share data was broken\n",
    "HiP Share : basematrix data was broken\n",
    "HiP Share : srcdst data was broken\n",
    "HiP Share : index data was broekn\n"
    "HiP Share : shape geometry data was broken\n"
};

/* error handling */
static sceHiErr _hip_share_err(_local_err err)
{
    sceHiErrState.mes = _share_err_mes[err];
    return _local2higerr[err];
}

/************************************************/
/*		Vector				*/
/************************************************/
static sceHiErr SetVector(SHARE_FRAME *sf)
{
    int i, j;
    sceHiPlugShareData_t	*svn;
    sceVu0FVECTOR		*src, *dst;
    sceHiPlugShapeMatrix_t	*mtx;

    for (i = 0; i < sf->sharedH->num; i++) {
	svn = sceHiPlugShareGetShare(sf->sharedH, i);
	if (svn == NULL)	return _hip_share_err(_SHARE_DATA_BROKEN);
	mtx = sceHiPlugShapeGetMatrix(sf->basemH, i);
	if (mtx == NULL)	return _hip_share_err(_BASEMATRIX_DATA_BROKEN);
	if (svn->dat.vlength > 0) {
	    for (j = svn->dat.voffset; j < svn->dat.voffset + svn->dat.vlength; j++) {
		dst = sceHiPlugShareGetDst(sf->srcdstvH, j);
		src = sceHiPlugShareGetSrc(sf->srcdstvH, j);
		if (dst == NULL || src == NULL)
		    return _hip_share_err(_SRCDST_BROKEN);
		sceVu0ApplyMatrix(*dst, mtx->local, *src);
	    }
	}
	if (svn->dat.nlength > 0) {
	    for (j = svn->dat.noffset; j < svn->dat.noffset + svn->dat.nlength; j++) {
		dst = sceHiPlugShareGetDst(sf->srcdstnH, j);
		src = sceHiPlugShareGetSrc(sf->srcdstnH, j);
		if (dst == NULL || src == NULL)
		    return _hip_share_err(_SRCDST_BROKEN);
		sceVu0ApplyMatrix(*dst, mtx->light, *src);
	    }
	}
    }
    return SCE_HIG_NO_ERR;
}

/************************************************/
/*		Share				*/
/************************************************/
static void MakeShare(sceHiPlugShareData_t *shareVN, sceVu0FVECTOR ** geom_tbl, sceHiPlugShareHead_t *srcdstH, sceHiPlugShareHead_t *indexH)
{
    int i, idx;
    int *index;
    sceVu0FVECTOR	*dst;

    for (i = 0; i < shareVN->shr.num; i++) {
	idx = shareVN->shr.offset + i;
	index = sceHiPlugShareGetIndex(indexH, idx);
	dst = sceHiPlugShareGetDst(srcdstH, *index);
	sceVu0CopyVector((geom_tbl[shareVN->shr.geomid])[i], *dst);
    }
}
static sceHiErr SetShare(SHARE_FRAME *sf)
{
    int i;

    /*	Make Vertex	*/
    for (i = 0; i < sf->sharevH->num; i++) {
	MakeShare(sceHiPlugShareGetShare(sf->sharevH, i), sf->pGeomVertex, sf->srcdstvH, sf->vertidxH);
    }

    /*	Make Normal	*/
    for (i = 0; i < sf->sharenH->num; i++) {
	MakeShare(sceHiPlugShareGetShare(sf->sharenH, i), sf->pGeomNormal, sf->srcdstnH, sf->normidxH);
    }

    return SCE_HIG_NO_ERR;
}

/************************************************/
/*		Share Init			*/
/************************************************/
static sceHiErr GeomInit(SHARE_FRAME *sf)
{
    int		ngeom = 0;
    int		i, j, ofs;
    sceHiPlugShapeHead_t	*dat, *mat, *geo;

    /* count geometry */
    dat = sceHiPlugShapeGetDataHead(sf->shapedH, sf->sharevH->shape);
    if (dat == NULL)	return _hip_share_err(_GEOMETRY_BROKEN);
    for (i = 0; i < dat->dat.num; i++) {
	mat = sceHiPlugShapeGetMaterialHead(dat, i);
	if (mat == NULL)	return _hip_share_err(_GEOMETRY_BROKEN);
	ngeom += mat->mat.num;
    }

    /* allocate geometry pointer array */
    sf->pGeomVertex = (sceVu0FVECTOR **) sceHiMemAlign(16, sizeof(sceVu0FVECTOR *) * ngeom);
    sf->pGeomNormal = (sceVu0FVECTOR **) sceHiMemAlign(16, sizeof(sceVu0FVECTOR *) * ngeom);

    /* set geometry pointer */
    for (ofs = i = 0; i < dat->dat.num; i++) {
	mat = sceHiPlugShapeGetMaterialHead(dat, i);
	if (mat == NULL)	return _hip_share_err(_GEOMETRY_BROKEN);
	for (j = 0; j < mat->mat.num; j++) {
	    geo = sceHiPlugShapeGetGeometryHead(mat, j);
	    if (geo == NULL)	return _hip_share_err(_GEOMETRY_BROKEN);
	    sf->pGeomVertex[ofs] = sceHiPlugShapeGetGeometryVertex(geo, 0);
	    if (sf->pGeomVertex[ofs] == NULL)
		return _hip_share_err(_GEOMETRY_BROKEN);
	    sf->pGeomNormal[ofs] = sceHiPlugShapeGetGeometryNormal(geo, 0);
	    if (sf->pGeomNormal[ofs] == NULL)
		return _hip_share_err(_GEOMETRY_BROKEN);
	    ++ofs;
	}
    }
    return SCE_HIG_NO_ERR;
}

static sceHiErr ShareInit(SHARE_FRAME *sf, sceHiPlug *p)
{
    sceHiErr	err;

    sf->sharedH = sceHiPlugShareGetHead(p, sharedt);
    if (sf->sharedH == NULL)
	return _hip_share_err(_SHARE_DATA_BROKEN);
    sf->srcdstvH = sceHiPlugShareGetHead(p, srcdstvt);
    if (sf->srcdstvH == NULL)
	return _hip_share_err(_SRCDST_BROKEN);
    sf->srcdstnH = sceHiPlugShareGetHead(p, srcdstnt);
    if (sf->srcdstnH == NULL)
	return _hip_share_err(_SRCDST_BROKEN);
    sf->vertidxH = sceHiPlugShareGetHead(p, vertidxt);
    if (sf->vertidxH == NULL)
	return _hip_share_err(_INDEX_BROKEN);
    sf->normidxH = sceHiPlugShareGetHead(p, normidxt);
    if (sf->normidxH == NULL)
	return _hip_share_err(_INDEX_BROKEN);
    sf->sharevH = sceHiPlugShareGetHead(p, sharevt);
    if (sf->sharevH == NULL)
	return _hip_share_err(_SHARE_DATA_BROKEN);
    sf->sharenH = sceHiPlugShareGetHead(p, sharent);
    if (sf->sharenH == NULL)
	return _hip_share_err(_SHARE_DATA_BROKEN);
    sf->basemH = sceHiPlugShapeGetHead(p, basemt);
    if (sf->basemH == NULL)
	return _hip_share_err(_BASEMATRIX_DATA_BROKEN);
    sf->shapedH = sceHiPlugShapeGetHead(p, shapedt);
    if (sf->shapedH == NULL)
	return _hip_share_err(_GEOMETRY_BROKEN);
    err = GeomInit(sf);

    return err;
}

/************************************************/
/*		Share Plug			*/
/************************************************/
sceHiErr sceHiPlugShare(sceHiPlug *plug, int process)
{
    sceHiErr	err;
    SHARE_FRAME	*share_frame;

    switch(process){
      case SCE_HIG_INIT_PROCESS:
	share_frame = (SHARE_FRAME *)sceHiMemAlign(16, sizeof(SHARE_FRAME) * 1);
	if(share_frame == NULL) return _hip_share_err(_NO_HEAP);

	err = ShareInit(share_frame, plug);
	if(err != SCE_HIG_NO_ERR) return err;

	plug->stack = (u_int)share_frame;
	break;

      case SCE_HIG_PRE_PROCESS:
	share_frame = (SHARE_FRAME *)plug->stack;
	if(share_frame == NULL) return _hip_share_err(_PLGBLK_STACK_BROKEN);
	err = SetVector(share_frame);		if(err != SCE_HIG_NO_ERR) return err;
	err = SetShare(share_frame);		if(err != SCE_HIG_NO_ERR) return err;
	break;

      case SCE_HIG_POST_PROCESS:
	break;

      case SCE_HIG_END_PROCESS:
	share_frame = (SHARE_FRAME *)plug->stack;
	if(share_frame == NULL) return _hip_share_err(_PLGBLK_STACK_BROKEN);
	sceHiMemFree((u_int *)Paddr(share_frame->pGeomVertex));
	sceHiMemFree((u_int *)Paddr(share_frame->pGeomNormal));
	sceHiMemFree((u_int *)Paddr(share_frame));
	plug->stack = NULL;
	plug->args = NULL;
	break;

      default:
	break;
    }

    return SCE_HIG_NO_ERR;
}
