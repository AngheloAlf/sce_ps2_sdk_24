/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: anime.c,v 1.11 2002/05/20 02:07:22 kaneko Exp $	*/
#include <eekernel.h>
#include <stdio.h>
#include <eeregs.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

#define QWORD	(4)
#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)

typedef enum {
    CONSTANT	= (1 << 0),
    LINEAR	= (1 << 1),
    HERMITE	= (1 << 2),
    BEZIER	= (1 << 3),
    BSPLINE	= (1 << 4)
} _interp_t;

typedef enum {
    TX = (1<<0),
    TY = (1<<1),
    TZ = (1<<2),
    RX = (1<<3),
    RY = (1<<4),
    RZ = (1<<5),
    SX = (1<<6),
    SY = (1<<7),
    SZ = (1<<8),
    TXYZ = (1<<9),
    RXYZ = (1<<10),
    RXZY = (1<<11),
    RYXZ = (1<<12),
    RYZX = (1<<13),
    RZXY = (1<<14),
    RZYX = (1<<15),
    SXYZ = (1<<16)
} _fcurve_t;

typedef enum {
    TXYZ_M = (0x01<<9),
    RXYZ_M = (0x3f<<10),
    SXYZ_M = (0x01<<16)
} _fcurve_m;

typedef enum {
    X=0,Y,Z,XYZ
} _factor_t;

typedef struct {
    /* SCE_HIP_ANIME_DATA */
    sceHiPlugAnimeHead_t	*anime_head;
    /* SCE_HIP_HRCHY_DATA */
    sceHiPlugHrchyHead_t	*hrchy_head;

    _interp_t			*interp;
    int				**frame_tbl;
    sceVu0FVECTOR		**value_tbl;
    int				padding[3];
} Anime_t;

typedef struct {
    Anime_t		anime;
    int			current_frame;
    int			padding[2];
} AnimeFrame_t;

static const sceHiType animet = {
    SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_ANIME, SCE_HIG_DATA_STATUS, SCE_HIP_ANIME_DATA, SCE_HIP_REVISION};
static const sceHiType framet = {
    SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_ANIME, SCE_HIG_DATA_STATUS, SCE_HIP_KEYFRAME, SCE_HIP_REVISION};
static const sceHiType valuet = {
    SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_ANIME, SCE_HIG_DATA_STATUS, SCE_HIP_KEYVALUE, SCE_HIP_REVISION};
static const sceHiType hrchyt = {
    SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_HRCHY, SCE_HIG_DATA_STATUS, SCE_HIP_HRCHY_DATA, SCE_HIP_REVISION};

typedef enum {
    _NULL_POINTER,
    _NEG_ANIME_FRAME_NUM,
    _NEG_KEY_FRAME_NUM,
    _NO_HEAP,
    _NEG_KEYVALUE_NUM,
    _PLGBLK_STACK_BROKEN
} _local_err;

/* local err id => hig err id */
static const sceHiErr _local2higerr[] = {
    SCE_HIG_INVALID_VALUE,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA
};

/* error message */
static const char *_anime_err_mes[] = {
    "HiP Anime : null pointer\n",
    "HiP Anime : the frame number is negative\n",
    "HiP Anime : the key frame number is negative\n",
    "HiP Anime : can't allocate memory\n",
    "HiP Anime : the key value number is negative\n",
    "HiP Anime : plug stack is null\n",
};

/* prototype */
static sceHiErr _hip_anime_err(_local_err);
static sceHiErr anime_init(sceHiPlug *, Anime_t *);
static sceHiErr anime_set(Anime_t *, int);
static u_int keypoint(u_int, u_int *, u_int);
static float spline(float, sceVu0FMATRIX, sceVu0FVECTOR);
static void Hermite(sceHiPlugHrchyData_t *, float *, u_int, float);
static void Bezier(sceHiPlugHrchyData_t *, float *, u_int, float);
static float linear(float, float, float);
static void Linear(sceHiPlugHrchyData_t *, float *, u_int, float);
static void Constant(sceHiPlugHrchyData_t *, float *, u_int);

/* error handling */
static sceHiErr _hip_anime_err(_local_err err)
{
    sceHiErrState.mes = _anime_err_mes[err];
    return _local2higerr[err];
}

/************************************************/
/*		Animation Init			*/
/************************************************/
static sceHiErr anime_init(sceHiPlug *p, Anime_t *anime)
{
    sceHiPlugAnimeHead_t	*frame_head;
    sceHiPlugAnimeHead_t	*value_head;
    sceHiPlugAnimeHead_t	*key_head;
    int				i;

    anime->anime_head = sceHiPlugAnimeGetHead(p, animet);
    if (anime->anime_head == NULL)
	return _hip_anime_err(_NULL_POINTER);
    anime->hrchy_head = sceHiPlugHrchyGetHead(p, hrchyt);
    if (anime->hrchy_head == NULL)
	return _hip_anime_err(_NULL_POINTER);
    frame_head = sceHiPlugAnimeGetHead(p, framet);
    if (frame_head == NULL)
	return _hip_anime_err(_NULL_POINTER);
    value_head = sceHiPlugAnimeGetHead(p, valuet);
    if (value_head == NULL)
	return _hip_anime_err(_NULL_POINTER);

    /* set interpolation & frame tbl */
    anime->frame_tbl = (int **)sceHiMemAlloc(sizeof(int *) * frame_head->key.num);
    anime->interp = (_interp_t *) sceHiMemAlloc(sizeof(_interp_t) * frame_head->key.num);
    if (anime->frame_tbl == NULL)
	return _hip_anime_err(_NO_HEAP);
    for (i = 0; i < frame_head->top.num; i++) {
	key_head = sceHiPlugAnimeGetKeyHead(frame_head, i);
	if (key_head == NULL)
	    return _hip_anime_err(_NULL_POINTER);
	anime->interp[i] = key_head->key.type;
	anime->frame_tbl[i] = sceHiPlugAnimeGetFrame(key_head, 0);
	if (anime->frame_tbl[i] == NULL)
	    return _hip_anime_err(_NULL_POINTER);
    }

    /* set value tbl */
    anime->value_tbl = (sceVu0FVECTOR **) sceHiMemAlloc(sizeof(sceVu0FVECTOR *) * value_head->key.num);
    if (anime->value_tbl == NULL)
	return _hip_anime_err(_NO_HEAP);
    for (i = 0; i < value_head->key.num; i++) {
	key_head = sceHiPlugAnimeGetKeyHead(value_head, i);
	if (key_head == NULL)
	    return _hip_anime_err(_NULL_POINTER);
	anime->value_tbl[i] = sceHiPlugAnimeGetValue(key_head, 0);
	if (anime->value_tbl[i] == NULL)
	    return _hip_anime_err(_NULL_POINTER);
    }

    return SCE_HIG_NO_ERR;
}

/************************************************/
/*		Animation			*/
/************************************************/
static sceHiErr anime_set(Anime_t *anime, int frame)
{
    int	nkey, hrchy_idx, frame_idx, value_idx;
    sceHiPlugAnimeData_t	*anime_data;
    sceHiPlugHrchyData_t	*hrchy_data;
    int		interp;
    int		*frame_tbl;
    sceVu0FVECTOR		*value_tbl;
    int		keyp, end_frame;
    int		i;

    for (i = 0; i < anime->anime_head->top.num; i++) {
	anime_data = sceHiPlugAnimeGetData(anime->anime_head, i);
	if (anime_data == NULL)
	    return _hip_anime_err(_NULL_POINTER);
	nkey = anime_data->numframe;
	hrchy_idx = anime_data->hrchy;
	frame_idx = anime_data->keyframe;
	value_idx = anime_data->keyvalue;

	hrchy_data = sceHiPlugHrchyGetData(anime->hrchy_head, hrchy_idx);
	if (hrchy_data == NULL)
	    return _hip_anime_err(_NULL_POINTER);
	interp = anime->interp[i];
	frame_tbl = anime->frame_tbl[frame_idx];
	value_tbl = anime->value_tbl[value_idx];

	/* check frame */
	end_frame = frame_tbl[nkey - 1];
	if (end_frame > 0)
	    frame = (u_int) (frame % end_frame);
	else
	    frame = 0;

	/* check key */
	if (nkey > 1)
	    keyp = keypoint(frame, frame_tbl, nkey);
	else
	    keyp = 0;

	/* interpolation */
	switch (interp) {
	    float	t;
	  case CONSTANT:
	    Constant(hrchy_data, (float *)value_tbl, keyp);
	    break;
	  case HERMITE:
	    t = (float)(frame - frame_tbl[keyp]) / (float)(frame_tbl[keyp + 1] - frame_tbl[keyp]);
	    Hermite(hrchy_data, (float *)value_tbl, keyp, t);
	    break;
	  case BEZIER:
	    t = (float)(frame - frame_tbl[keyp]) / (float)(frame_tbl[keyp + 1] - frame_tbl[keyp]);
	    Bezier(hrchy_data, (float *)value_tbl, keyp, t);
	    break;
	  case BSPLINE:
	    break;
	  case LINEAR:
	  default:
	    t = (float)(frame - frame_tbl[keyp]) / (float)(frame_tbl[keyp + 1] - frame_tbl[keyp]);
	    Linear(hrchy_data, (float *)value_tbl, keyp, t);
	    break;
	}
    }
    return SCE_HIG_NO_ERR;
}

/************************************************/
/*		Keypoint			*/
/************************************************/
static u_int keypoint(u_int frame, u_int *frame_tbl, u_int nkey)
{
    u_int	i;
    u_int	ret;

    ret = 0;
    for (i = 0; i < nkey; i++) {
	if ((frame >= frame_tbl[i]) && (frame <= frame_tbl[i + 1])) {
	    ret = i;
	    break;
	}
    }

    return ret;
}

/************************************************/
/*		Spline				*/
/************************************************/
/*

  f(t) = ((a*t + b)*t + c)*t + d
  q = { a, b, c, d }
  q = m * p

*/
static float spline(float t, sceVu0FMATRIX m, sceVu0FVECTOR p)
{
  sceVu0FVECTOR q;

  float f;

  sceVu0ApplyMatrix(q, m, p);
  f = ((q[0]*t + q[1])*t + q[2])*t +q[3];
  return f;
}

/************************************************/
/*		Hermite				*/
/************************************************/
static void Hermite(sceHiPlugHrchyData_t *hrc, float *kv, u_int key, float t)
{
  float f;
  u_int i;
  sceVu0FVECTOR *p;
  static sceVu0FMATRIX m ={
    { 2.0f, -3.0f, 0.0f, 1.0f},
    {-2.0f,  3.0f, 0.0f, 0.0f},
    { 1.0f, -2.0f, 1.0f, 0.0f},
    { 1.0f, -1.0f, 0.0f, 0.0f}
  };
  u_int	*fcurve;
  u_int offset;

  fcurve = (u_int *)(kv - QWORD);
  if(!(*fcurve)) *fcurve = TXYZ|RYXZ;

  offset = 0;
  if((*fcurve) & TXYZ_M) offset++;
  if((*fcurve) & RXYZ_M) offset++;
  if((*fcurve) & SXYZ_M) offset++;

  p = (sceVu0FVECTOR *)kv;
  p += key * XYZ * offset;

  if((*fcurve) & TXYZ_M){
    for(i=0;i<XYZ;i++){
      f = spline(t, m, *p);
      hrc->trans[i] = f;
      p ++;
    }
    hrc->trans[XYZ] = 0.0f;
  }

  if((*fcurve) & RXYZ_M){
    for(i=0;i<XYZ;i++){
      f = spline(t, m, *p);
      hrc->rot[i] = f;
      p++;
    }
    hrc->rot[XYZ] = 0.0f;
  }

  if((*fcurve) & SXYZ_M){
    for(i=0;i<XYZ;i++){
      f = spline(t, m, *p);
      hrc->scale[i] = f;
      p++;
    }
    hrc->scale[XYZ] = 0.0f;
  }
}

/************************************************/
/*		Bezier				*/
/************************************************/
static void Bezier(sceHiPlugHrchyData_t *hrc, float *kv, u_int key, float t)
{
  float f;
  u_int i;
  sceVu0FVECTOR *p;
  static sceVu0FMATRIX m ={
    {-1.0f,  3.0f, -3.0f, 1.0f},
    { 3.0f, -6.0f,  3.0f, 0.0f},
    {-3.0f,  3.0f,  0.0f, 0.0f},
    { 1.0f,  0.0f,  0.0f, 0.0f}
  };
  u_int	*fcurve;
  u_int offset;

  fcurve = (u_int *)(kv - QWORD);
  if(!(*fcurve)) *fcurve = TXYZ|RYXZ;

  offset = 0;
  if((*fcurve) & TXYZ_M) offset++;
  if((*fcurve) & RXYZ_M) offset++;
  if((*fcurve) & SXYZ_M) offset++;

  p = (sceVu0FVECTOR *)kv;
  p += key * XYZ * offset;

  if((*fcurve) & TXYZ_M){
    for(i=0;i<XYZ;i++){
      f = spline(t, m, *p);
      hrc->trans[i] = f;
      p ++;
    }
    hrc->trans[XYZ] = 0.0f;
  }

  if((*fcurve) & RXYZ_M){
    for(i=0;i<XYZ;i++){
      f = spline(t, m, *p);
      hrc->rot[i] = f;
      p++;
    }
    hrc->rot[XYZ] = 0.0f;
  }

  if((*fcurve) & SXYZ_M){
    for(i=0;i<XYZ;i++){
      f = spline(t, m, *p);
      hrc->scale[i] = f;
      p++;
    }
    hrc->scale[XYZ] = 0.0f;
  }
}
/************************************************/
/*		Linear				*/
/************************************************/
static float linear(float t, float a, float b)
{
  float f;

  f = (1.0f - t)*a + t*b;
  return f;
}
static void Linear(sceHiPlugHrchyData_t *hrc, float *kv, u_int key, float t)
{
  float a,b;
  float f;
  u_int i;
  sceVu0FVECTOR *p,*q;
  u_int	*fcurve;
  u_int offset;

  fcurve = (u_int *)(kv - QWORD);
  if(!(*fcurve)) *fcurve = TXYZ|RYXZ;

  offset = 0;
  if((*fcurve) & TXYZ_M) offset++;
  if((*fcurve) & RXYZ_M) offset++;
  if((*fcurve) & SXYZ_M) offset++;

  p = q = (sceVu0FVECTOR *)kv;
  p += key * offset;
  q += (key+1) * offset;

  if((*fcurve) & TXYZ_M){
    for(i=0;i<XYZ;i++){
      a = (*p)[i];
      b = (*q)[i];
      f = linear(t, a, b);
      hrc->trans[i] = f;
    }
    hrc->trans[XYZ] = 0.0f;
    p++; q++;
  }
  if((*fcurve) & RXYZ_M){
    for(i=0;i<XYZ;i++){
      a = (*p)[i];
      b = (*q)[i];

      f = linear(t, a, b);
      hrc->rot[i] = f;
    }
    hrc->rot[XYZ] = 0.0f;
    p++; q++;
  }
  if((*fcurve) & SXYZ_M){
    for(i=0;i<XYZ;i++){
      a = (*p)[i];
      b = (*q)[i];

      f = linear(t, a, b);
      hrc->scale[i] = f;
    }
    hrc->scale[XYZ] = 0.0f;
  }
  
}

/************************************************/
/*		Constant			*/
/************************************************/
static void Constant(sceHiPlugHrchyData_t *hrc, float *kv, u_int key)
{
  sceVu0FVECTOR *p;
  u_int	*fcurve;
  u_int offset;

  fcurve = (u_int *)(kv - QWORD);
  if(!(*fcurve)) *fcurve = TXYZ|RYXZ;

  offset = 0;
  if((*fcurve) & TXYZ_M) offset++;
  if((*fcurve) & RXYZ_M) offset++;
  if((*fcurve) & SXYZ_M) offset++;

  p = (sceVu0FVECTOR *)kv;
  p += key * offset;

  if((*fcurve) & TXYZ_M){
    sceVu0CopyVector(hrc->trans, *p);
    p++;
  }
  if((*fcurve) & RXYZ_M){
    sceVu0CopyVector(hrc->rot, *p);
    p++;
  }
  if((*fcurve) & SXYZ_M){
    sceVu0CopyVector(hrc->scale, *p);
  }
}

/************************************************/
/*		Animation Plug			*/
/************************************************/
sceHiErr sceHiPlugAnime(sceHiPlug *plug, int process)
{
    AnimeFrame_t	*anime_frame;
    sceHiErr		err;
    sceHiPlugAnimePreCalcArg_t	*arg;

    switch (process) {
      case SCE_HIG_INIT_PROCESS:
	anime_frame = (AnimeFrame_t *) sceHiMemAlign(16, sizeof(AnimeFrame_t));
	if (anime_frame == NULL)
	    return _hip_anime_err(_NO_HEAP);
	err = anime_init(plug, &anime_frame->anime);
	if (err != SCE_HIG_NO_ERR)
	    return err;
	anime_frame->current_frame = 0;
	plug->stack = (u_int)anime_frame;
	break;
      case SCE_HIG_PRE_PROCESS:
	anime_frame = (AnimeFrame_t *)plug->stack;
	if (anime_frame == NULL)
	    return _hip_anime_err(_PLGBLK_STACK_BROKEN);
	arg = (sceHiPlugAnimePreCalcArg_t *)plug->args;
	if (arg != NULL) {
	    if (arg->setframe_enable)
		anime_frame->current_frame = arg->setframe;
	    else
		anime_frame->current_frame++;
	    arg->currentframe= anime_frame->current_frame;
	} else
	    anime_frame->current_frame++;
	err = anime_set(&anime_frame->anime, anime_frame->current_frame);
	if (err != SCE_HIG_NO_ERR)
	    return err;
	break;
      case SCE_HIG_POST_PROCESS:
	break;
      case SCE_HIG_END_PROCESS:
	anime_frame = (AnimeFrame_t *) plug->stack;
	if (anime_frame == NULL)
	    return _hip_anime_err(_PLGBLK_STACK_BROKEN);
	/* free allocation */
	sceHiMemFree((u_int *)Paddr(anime_frame->anime.interp));
	sceHiMemFree((u_int *)Paddr(anime_frame->anime.frame_tbl));
	sceHiMemFree((u_int *)Paddr(anime_frame->anime.value_tbl));
	/* clear memory */
	anime_frame->current_frame = 0;
	anime_frame->anime.anime_head = 0;
	anime_frame->anime.hrchy_head = 0;
	anime_frame->anime.interp = NULL;
	anime_frame->anime.frame_tbl = NULL;
	anime_frame->anime.value_tbl = NULL;
	plug->stack = 0;
	plug->args = NULL;
      default:
	break;
    }
    return SCE_HIG_NO_ERR;
}
