/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: micro.c,v 1.22 2002/05/20 02:07:23 kaneko Exp $	*/
#include <eekernel.h>
#include <stdlib.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)

/***************************************************
 * local errs
 ***************************************************/
typedef enum {
    _NO_HEAP,
    _CODE_PACKET_MAKING_ERR,
    _DATA_PACKET_MAKING_ERR,
    _PLGBLK_STACK_BROKEN,
    _ILLEAGAL_MICRO_ID,
    _MICRO_DATA_ERR,
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_NO_HEAP,
    SCE_HIG_FAILURE,
    SCE_HIG_FAILURE,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_VALUE,
    SCE_HIG_FAILURE
};

static const char *_micro_err_mes[] = {
    "HiP Micro : can't allocate memory from HiG Heap\n",
    "HiP Micro : fail to make micro code packet.\n",
    "HiP Micro : fail to make micro data packet.\n",
    "HiP Micro : plug stack is null.\n",
    "HiP Micro : illeagal micro id\n",
    "HiP Micro : micro data error. \n"
};

/* error handling */
static sceHiErr _hip_micro_err(_local_err err)
{
    sceHiErrState.mes = _micro_err_mes[err];
    return _local2higerr[err];
}

/*******************************************************
 *	Micro Code Address
 *******************************************************/
extern u_int sce_micro_base[] __attribute__((section(".vudata")));

sceHiPlugMicroTbl_t sce_default_micro_tbl[] = {
    { sce_micro_base, SCE_HIP_MICRO_ATTR_NONE }
};

typedef union {
    u_long	ul[2];
    u_int	ui[4];
} gtag_t; /* _giftags[2] __attribute__((section(".vudata"))); */

/*******************************************************
 *	Structure		       
 *******************************************************/

/* Data Structure for micro data */
typedef struct{
    gtag_t		giftags[2];
    int			mic_id;			/* micro code id */
    int			*code_id;	/* micro code packet id */
    u_int		*attr;	/* attribute data */
    int			data_id;		/* micro data packet id */

    sceHiPlugMicroTbl_t	*tbl;
    u_int		tblnum;
}MICRO_FRAME __attribute__((aligned(64)));

/*******************************************************
 *	Micro Packet		       
 *******************************************************/

static u_int MicroMakeCODEPacket(MICRO_FRAME *mic, u_int *codeadr, int worksize)
{
	int	id;

	if (sceHiDMAMake_ChainStart())			return 0xffffffff;
	if (sceHiDMAMake_CallPtr(codeadr))		return 0xffffffff;
	if (sceHiDMAMake_LoadPtr((u_int *)8, (u_int *)mic->giftags, 2))	return 0xffffffff;
	if (sceHiDMAInit_DBuf(120, 1024-worksize))	return 0xffffffff;
	if (sceHiDMAMake_ChainEnd(&id))			return 0xffffffff;
	return id;
}

static u_int MicroMakeDATAPacket(u_int *micdata)
{
    int	id;

    if (sceHiDMAMake_ChainStart())			return 0xffffffff;
    if (sceHiDMAMake_LoadPtr((u_int *)16, micdata, 52))	return 0xffffffff;
    /* 予め作成しておくgiftagデータの転送 */
    if (sceHiDMAMake_ChainEnd(&id))			return 0xffffffff;
    return id;
}

/*******************************************************
 *	set giftag params (== fwMicroChange)
 *******************************************************/
static void micro_set_data(sceHiPlug *plug, MICRO_FRAME	*mf, sceHiPlugMicroData_t *data)
{
    sceHiPlugMicroPreCalcArg_t	*arg;
    u_int			nreg, tag2, tag3, flg, prim;
    sceHiGsGeneral	*gen;

    gen = sceHiGsGeneralStatus();

    arg = (sceHiPlugMicroPreCalcArg_t *)plug->args;
    if (arg != NULL) {
	data->aa1 = arg->anticutoff;
	data->fogA = 1 / (arg->fogbegin - arg->fogend);
	data->fogB = arg->fogend;
    }

    /* make gif tag */
    /* ここでは giftagの
       PRE	: PRIM fieldを無視させるため 0に
       FLG	: PACKEDモードに
       NREG	: それぞれのマイクロコード依存に
       REGS	: それぞれのマイクロコード依存に
       の部分を設定し、Init Data部分に置く
       また、Init DataのPRIM部分([2][1])に Primitive atrtibuteを
       設定するようにする（ここの値は直接GSのPRMODEレジスタに書かれる）
       ここで設定したattributeはそのままでは動作しない可能性がある。
       Shape Plugで設定したものとORしたものが実際のGS Kickで使われる
       Primitive属性になる事に注意（see me. shape.c）

       01.01.16
       gs service functionsの設定を有効にするように変更
    */

    /* default value */
    nreg = 3;
    tag2 = (SCE_GS_ST << (4 * 0)) | (SCE_GS_RGBAQ << (4 * 1)) | (SCE_GS_XYZF2 << (4 * 2));
    tag3 = 0;
    flg = SCE_GIF_PACKED;
    prim = 0;

    if (mf->mic_id != -1) {
	/* GS設定で Fog onかつ Micro Code Tableで onなら FOG */
	if (gen->prmode.FGE && (mf->attr[mf->mic_id] & SCE_HIP_MICRO_ATTR_FGE)) {
	    prim |= GS_PRIM_FGE_M;
	}
	/* GS設定にはAntiがない */
	if (mf->attr[mf->mic_id] & SCE_HIP_MICRO_ATTR_ANTI) {
	    nreg = 4;
	    tag2 = (SCE_GIF_PACKED_AD << (4 * 0)) | (SCE_GS_ST << (4 * 1)) | (SCE_GS_RGBAQ << (4 * 2)) | (SCE_GS_XYZF2 << (4 * 3));
	}
	/* make tag */
	mf->giftags[1].ui[3] = tag3;
	mf->giftags[1].ui[2] = tag2;
	mf->giftags[1].ui[1] = (nreg << GIF_TAG1_NREG_O) | (flg << GIF_TAG1_FLG_O);
	mf->giftags[1].ui[0] = 0;
    }
				/* GIF_TAG1 */

    /* GS設定で Alpha onなら on */
    if (gen->prmode.ABE) {
	prim |= GS_PRIM_ABE_M;
    }

    data->prmode = prim;
}

/*******************************************************
 *	Micro Plug		       
 *******************************************************/
sceHiErr sceHiPlugMicro(sceHiPlug *plug, int process)
{
    sceHiPlugMicroData_t	*data;
    MICRO_FRAME	*micro_frame;
    int		i;

    switch(process){
      case SCE_HIG_INIT_PROCESS:

	data = sceHiPlugMicroGetData(plug);
	if (data == NULL)	return _hip_micro_err(_MICRO_DATA_ERR);

	micro_frame = (MICRO_FRAME *)sceHiMemAlign(16, sizeof(MICRO_FRAME) *1);
	if(micro_frame == NULL) return _hip_micro_err(_NO_HEAP);
	
	/* Micro Code Packet Making */
	if (plug->args == (int)NULL) {
	    /* default setting */
	    micro_frame->tblnum = 1;
	    micro_frame->tbl = sce_default_micro_tbl;
	} else {
	    micro_frame->tblnum = ((sceHiPlugMicroInitArg_t *)plug->args)->tblnum;
	    micro_frame->tbl    = ((sceHiPlugMicroInitArg_t *)plug->args)->tbl;
	}

	micro_frame->code_id = (int *) sceHiMemAlloc(sizeof(int) * micro_frame->tblnum);
	if (micro_frame->code_id == NULL)
	    return _hip_micro_err(_NO_HEAP);
	micro_frame->attr = (u_int *) sceHiMemAlloc(sizeof(u_int) * micro_frame->tblnum);
	if (micro_frame->attr == NULL)
	    return _hip_micro_err(_NO_HEAP);
	for (i = 0; i < micro_frame->tblnum; i++) {
	    micro_frame->attr[i] = micro_frame->tbl[i].attr;
	}
	for (i = 0; i < micro_frame->tblnum; i++) {
	    micro_frame->code_id[i] = MicroMakeCODEPacket(micro_frame, micro_frame->tbl[i].micro, 0);
	    if (micro_frame->code_id[i] == 0xffffffff)
		return _hip_micro_err(_CODE_PACKET_MAKING_ERR);
	}

	micro_frame->data_id = MicroMakeDATAPacket((u_int *)data);
	if(micro_frame->data_id == 0xffffffff)
	    return _hip_micro_err(_DATA_PACKET_MAKING_ERR);

	/* giftag making (common) */
	/*				nloop, eop, pre, prim, flg, nreg */
	micro_frame->giftags[0].ul[0] = SCE_GIF_SET_TAG(1, 0, 0, 0, 0, 2);
	micro_frame->giftags[0].ul[1] = (SCE_GIF_PACKED_AD << (4 * 0)) | (SCE_GIF_PACKED_AD << (4 * 1));

	plug->args = 0;
	plug->stack = (u_int)micro_frame;
	break;

      case SCE_HIG_PRE_PROCESS:
	micro_frame = (MICRO_FRAME *)plug->stack;
	if (micro_frame == NULL) return _hip_micro_err(_PLGBLK_STACK_BROKEN);

	data = sceHiPlugMicroGetData(plug);
	if (data == NULL)	return _hip_micro_err(_MICRO_DATA_ERR);

	if (plug->args == NULL)
	    micro_frame->mic_id = 0;
	else {
	    micro_frame->mic_id = ((sceHiPlugMicroPreCalcArg_t *)plug->args)->micro;
	    if (micro_frame->mic_id >= (int)micro_frame->tblnum)
		micro_frame->mic_id = 0;
	}

	/* set Initial Data */
	micro_set_data(plug, micro_frame, data);

	break;

      case SCE_HIG_POST_PROCESS:
	micro_frame = (MICRO_FRAME *)plug->stack;
	if(micro_frame == NULL) return _hip_micro_err(_PLGBLK_STACK_BROKEN);
	if (micro_frame->mic_id != -1) {
	    /* id check */
	    if (micro_frame->mic_id < 0)
		return _hip_micro_err(_ILLEAGAL_MICRO_ID);
	    if (micro_frame->mic_id >= (int)micro_frame->tblnum)
		micro_frame->mic_id = 0;
		/* Micro Code転送要求 */
	    sceHiDMARegist(micro_frame->code_id[micro_frame->mic_id]);
	}
	/* Micro Data転送 */
	sceHiDMARegist(micro_frame->data_id);
	break;

      case SCE_HIG_END_PROCESS:
	micro_frame = (MICRO_FRAME *)plug->stack;
	if(micro_frame == NULL) return _hip_micro_err(_PLGBLK_STACK_BROKEN);

	for (i = 0; i < micro_frame->tblnum; i++)
	    sceHiDMADel_Chain(micro_frame->code_id[i]);
	sceHiDMADel_Chain(micro_frame->data_id);
	sceHiMemFree((u_int *)Paddr(micro_frame->code_id));
	sceHiMemFree((u_int *)Paddr(micro_frame->attr));
	sceHiMemFree((u_int *)Paddr(micro_frame));

	plug->stack = NULL;
	plug->args = NULL;
	break;
      default:
	break;
    }

    return SCE_HIG_NO_ERR;
}
