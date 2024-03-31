/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: access.c,v 1.8 2002/05/20 02:07:22 kaneko Exp $	*/
#include <eekernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <eeregs.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

static inline u_int *_gethead(sceHiPlug *p, sceHiType t)
{
    u_int *d = NULL;

    /* no error check. if error then d==NULL */
    sceHiGetData(p, &d, t);
    return d;
}

#define QWORD 4
/*********************************************************************************
 *				SCE_HIP_SHAPE_DATA
 *********************************************************************************/
sceHiPlugShapeHead_t *sceHiPlugShapeGetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugShapeHead_t *)_gethead(p,t);
}

sceHiPlugShapeHead_t *sceHiPlugShapeGetDataHead(sceHiPlugShapeHead_t *h, int idx)
{
    int i;
    size_t size;
    int *p;

    p = (int *)(h+1);
    for(i=0;i<idx;i++){
	size = p[1];		/* shape wsize */
	p+=size+QWORD;
    }
    return (sceHiPlugShapeHead_t *)p;
}

sceHiPlugShapeHead_t *sceHiPlugShapeGetMaterialHead(sceHiPlugShapeHead_t *h, int idx)
{
    int	i,j;
    size_t size;
    int *p;
    int gn;

    p = (int *)(h+1);			/* skip shape head */

    for(i=0;i<idx;i++){

	gn = p[1];			/* num of geometries */
	p += QWORD;			/* skip material head */

	/*	Skip Material Data	*/
	size = p[3] + (p[4]&0x7fff);	/* material qwsize */
	p += (size+2)*QWORD;		/* matsize + giftag = 2 */

	/*	Skip Geometry Data	*/
	for(j=0;j<gn;j++){
	    size = p[1];		/* geometry wsize */
	    p += size+QWORD;		/* head + wsize */
	}
    }

    return (sceHiPlugShapeHead_t *)p;
}

sceHiPlugShapeHead_t *sceHiPlugShapeGetGeometryHead(sceHiPlugShapeHead_t *h, int idx)
{
    int i;
    size_t size;
    int *p;

    p = (int *)(h+1);			/* skip material head */

    /*	Skip Material Data	*/
    size = p[3] + (p[4]&0x7fff);	/* material qwsize */
    p += (size+2)*QWORD;		/* matsize + giftag = 2 */

    /*	Skip Geometry Data	*/
    for(i=0;i<idx;i++){
	size = p[1];		/* geometry wsize */
	p += size+QWORD;	/* head + wsize */
    }

    return (sceHiPlugShapeHead_t *)p;
}

/*********************************************************************************
 *				Material
 *********************************************************************************/
sceHiGsGiftag *sceHiPlugShapeGetMaterialGiftag(sceHiPlugShapeHead_t *h)
{
    sceHiGsGiftag *g;

    g = (sceHiGsGiftag *)(h+2);	/* skip head & material size */

    if(g->nloop == 0) return NULL;
    return g;
}

sceVu0FMATRIX *sceHiPlugShapeGetMaterialAttrib(sceHiPlugShapeHead_t *h)
{
    size_t size;
    int *p;

    p = (int *)(h+1);	/* skip head */

    /* material size */
    size = p[3];
    if(size == 0) return NULL;

    /* giftag.nloop */
    size = p[4]&0x7fff;

    p+=(size+2)*QWORD;

    return (sceVu0FMATRIX *)p;
}

/*********************************************************************************
 *				Geometry
 *********************************************************************************/
sceVu0FVECTOR *sceHiPlugShapeGetGeometryVertex(sceHiPlugShapeHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+idx;
}

sceVu0FVECTOR *sceHiPlugShapeGetGeometryNormal(sceHiPlugShapeHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+h->geo.num+idx;
}

sceVu0FVECTOR *sceHiPlugShapeGetGeometryST(sceHiPlugShapeHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+h->geo.num*2+idx;
}

sceVu0FVECTOR *sceHiPlugShapeGetGeometryColor(sceHiPlugShapeHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+h->geo.num*3+idx;
}

/*********************************************************************************
 *				SCE_HIP_BASEMATRIX
 *********************************************************************************/
sceHiPlugShapeMatrix_t *sceHiPlugShapeGetMatrix(sceHiPlugShapeHead_t *h, int idx)
{
    return (sceHiPlugShapeMatrix_t *)(h+1)+idx;
}

/*********************************************************************************
 *				SCE_HIP_HRCHY_DATA
 *********************************************************************************/
sceHiPlugHrchyHead_t *sceHiPlugHrchyGetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugHrchyHead_t *)_gethead(p,t);
}

sceHiPlugHrchyData_t *sceHiPlugHrchyGetData(sceHiPlugHrchyHead_t *h, int idx)
{
    return (sceHiPlugHrchyData_t *)(h+1)+idx;
}

/*********************************************************************************
 *				SCE_HIP_PIVOT_DATA
 *********************************************************************************/
sceVu0FVECTOR	*sceHiPlugHrchyGetPivot(sceHiPlugHrchyHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+idx;
}

/*********************************************************************************
 *				SCE_HIP_TEX2D_DATA
 *********************************************************************************/
sceHiPlugTex2DHead_t *sceHiPlugTex2DGetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugTex2DHead_t *)_gethead(p,t);
}

sceHiPlugTex2DData_t *sceHiPlugTex2DGetData(sceHiPlugTex2DHead_t *h, int idx)
{
    int *d = (int *)(h+1);
    size_t size;
    int i;

    for(i=0;i<idx;i++){
	size = d[4]+d[5]+2*QWORD;
	d += size;
    }
    return (sceHiPlugTex2DData_t *)d;
}

u_int *sceHiPlugTex2DGetTexel(sceHiPlugTex2DData_t *d)
{
    return (u_int *)(d+1);
}

u_int *sceHiPlugTex2DGetClut(sceHiPlugTex2DData_t *d)
{
    return (u_int *)(d+1)+d->texelsize;
}

/*********************************************************************************
 *				SCE_HIP_TEX2D_ENV
 *********************************************************************************/
sceHiGsGiftag *sceHiPlugTex2DGetEnv(sceHiPlugTex2DHead_t *h, int idx)
{
    sceHiGsGiftag *g = (sceHiGsGiftag *)h + 1;
    int i;

    for(i=0;i<idx;i++){
	g += (g->nloop + 1);
    }
    return g;
}

/*********************************************************************************
 *				SCE_HIP_MICRO_DATA
 *********************************************************************************/
sceHiPlugMicroData_t *sceHiPlugMicroGetData(sceHiPlug *p)
{
    static sceHiType t = {
	SCE_HIP_COMMON, 
	SCE_HIP_FRAMEWORK,
	SCE_HIP_MICRO,
	SCE_HIG_DATA_STATUS,
	SCE_HIP_MICRO_DATA,
	SCE_HIP_REVISION
    };
    return (sceHiPlugMicroData_t *)_gethead(p,t);
}

/*********************************************************************************
 *				SCE_HIP_SHADOWBOX_DATA
 *********************************************************************************/
sceHiPlugShadowBoxData_t *sceHiPlugShadowBoxGetData(sceHiPlug *p)
{
    static sceHiType t = {
	SCE_HIP_COMMON, 
	SCE_HIP_FRAMEWORK,
	SCE_HIP_SHADOW,
	SCE_HIG_DATA_STATUS,
	SCE_HIP_SHADOWBOX_DATA,
	SCE_HIP_REVISION
    };
    return (sceHiPlugShadowBoxData_t *)_gethead(p,t);
}

/*********************************************************************************
 *                              SCE_HIP_CLUTBUMP_DATA
 *********************************************************************************/
sceHiPlugClutBumpHead_t *sceHiPlugClutBumpGetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugClutBumpHead_t *)_gethead(p,t);
}

sceHiPlugClutBumpData_t *sceHiPlugClutBumpGetData(sceHiPlugClutBumpHead_t *h, int idx)
{
    return (sceHiPlugClutBumpData_t *)(h+1)+idx;
}

/*********************************************************************************
 *                              SCE_HIP_CLUTBUMP_NORMAL
 *********************************************************************************/
sceVu0FVECTOR *sceHiPlugClutBumpGetNormal(sceHiPlugClutBumpHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+idx*256;
}

/*********************************************************************************
 *                              SCE_HIP_TIM2_DATA
 *********************************************************************************/
sceHiPlugTim2Head_t *sceHiPlugTim2GetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugTim2Head_t *)_gethead(p, t);
}

sceHiPlugTim2Data_t *sceHiPlugTim2GetData(sceHiPlugTim2Head_t *h, int idx)
{
    return (sceHiPlugTim2Data_t *)(h+1)+idx;
}

/*********************************************************************************
 *                              SCE_HIP_ANIME_DATA
 *********************************************************************************/
sceHiPlugAnimeHead_t *sceHiPlugAnimeGetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugAnimeHead_t *)_gethead(p, t);
}

sceHiPlugAnimeData_t *sceHiPlugAnimeGetData(sceHiPlugAnimeHead_t *h, int idx)
{
    return (sceHiPlugAnimeData_t *)(h+1)+idx;
}

/*********************************************************************************
 *                              SCE_HIP_KEYFRAME/KEYVALUE
 *********************************************************************************/
sceHiPlugAnimeHead_t *sceHiPlugAnimeGetKeyHead(sceHiPlugAnimeHead_t *h, int idx)
{
    int *d=(int *)(h+1);
    int i;

    for(i=0;i<idx;i++){
	d += d[2]+QWORD;
    }
    return (sceHiPlugAnimeHead_t *)d;
}

int *sceHiPlugAnimeGetFrame(sceHiPlugAnimeHead_t *h, int idx)
{
    return (int *)(h+1)+idx;
}

sceVu0FVECTOR *sceHiPlugAnimeGetValue(sceHiPlugAnimeHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+idx*(h->key.size/(h->key.num*QWORD));
}

/*********************************************************************************
 *				SCE_HIP_SHARE_DATA
 *********************************************************************************/
sceHiPlugShareHead_t *sceHiPlugShareGetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugShareHead_t *)_gethead(p, t);
}

sceHiPlugShareData_t *sceHiPlugShareGetData(sceHiPlugShareHead_t *h, int idx)
{
    return (sceHiPlugShareData_t *)(h+1)+idx;
}

/*********************************************************************************
 *			SCE_HIP_SHAREVERTEX/SHARENORMAL
 *********************************************************************************/
sceHiPlugShareData_t *sceHiPlugShareGetShare(sceHiPlugShareHead_t *h, int idx)
{
    return (sceHiPlugShareData_t *)(h+1)+idx;
}

/*********************************************************************************
 *			SCE_HIP_VERTEXINDEX/NORMALINDEX
 *********************************************************************************/
int *sceHiPlugShareGetIndex(sceHiPlugShareHead_t *h, int idx)
{
    return (int *)(h+1)+idx;
}

/*********************************************************************************
 *			SCE_HIP_SRCDSTVERTEX/SRCDSTNORMAL
 *********************************************************************************/
sceVu0FVECTOR *sceHiPlugShareGetSrc(sceHiPlugShareHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+idx;
}

/*********************************************************************************
 *			SCE_HIP_SRCDSTVERTEX/SRCDSTNORMAL
 *********************************************************************************/
sceVu0FVECTOR *sceHiPlugShareGetDst(sceHiPlugShareHead_t *h, int idx)
{
    return (sceVu0FVECTOR *)(h+1)+h->num+idx;
}

/*********************************************************************************
 *			SCE_HIP_SKIN_DATA/LB/LW/BW
 *********************************************************************************/
sceHiPlugSkinHead_t *sceHiPlugSkinGetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugSkinHead_t *)_gethead(p, t);
}

sceHiPlugSkinData_t *sceHiPlugSkinGetData(sceHiPlugSkinHead_t *h, int idx)
{
    return (sceHiPlugSkinData_t *)(h+1)+idx;
}

sceVu0FMATRIX *sceHiPlugSkinGetLB(sceHiPlugSkinHead_t *h, int idx)
{
    return (sceVu0FMATRIX *)(h+1)+idx;
}

sceVu0FMATRIX *sceHiPlugSkinGetLW(sceHiPlugSkinHead_t *h, int idx)
{
    return (sceVu0FMATRIX *)(h+1)+idx;
}

int *sceHiPlugSkinGetBW(sceHiPlugSkinHead_t *h, int idx)
{
    return (int *)(h+1)+idx;
}

/*********************************************************************************
 *				SCE_HIP_CLIP_DATA
 *********************************************************************************/
sceHiPlugClipHead_t *sceHiPlugClipGetHead(sceHiPlug *p, sceHiType t)
{
    return (sceHiPlugClipHead_t *)_gethead(p, t);
}

sceHiPlugClipData_t *sceHiPlugClipGetData(sceHiPlugClipHead_t *h, int idx)
{
    return (sceHiPlugClipData_t *)(h+1)+idx;
}

/*********************************************************************************
 *				SCE_HIP_CAMERA_DATA
 *********************************************************************************/
sceHiPlugCameraData_t *sceHiPlugCameraGetData(sceHiPlug *p)
{
    const sceHiType t = {
	SCE_HIP_COMMON, 
	SCE_HIP_FRAMEWORK,
	SCE_HIP_CAMERA,
	SCE_HIG_DATA_STATUS,
	SCE_HIP_CAMERA_DATA,
	SCE_HIP_REVISION
    };
    return (sceHiPlugCameraData_t *)_gethead(p, t);
}
