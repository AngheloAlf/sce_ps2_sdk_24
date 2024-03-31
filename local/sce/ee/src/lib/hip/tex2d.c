/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: tex2d.c,v 1.16 2002/05/20 02:07:23 kaneko Exp $	*/
#include <eekernel.h>
#include <stdlib.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)

#define QWSIZE	4
#define NUMIDX	3
#define INIT_TBP 0x1a40		/* default tbp for safety */
#define INIT_CBP 0x4000		/* default cbp for safety */
#define RESIDENT 2		/* first texture transfer flag */

#define SET_QW(qw, three, two, one, zero)  \
        (qw)[3]=three; (qw)[2]=two; (qw)[1]=one; (qw)[0]=zero;

static const sceHiType tex2dt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_TEX2D, SCE_HIG_DATA_STATUS, SCE_HIP_TEX2D_DATA, SCE_HIP_REVISION };
static const sceHiType texenvt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_TEX2D, SCE_HIG_DATA_STATUS, SCE_HIP_TEX2D_ENV, SCE_HIP_REVISION };

/************************************************/
/*		Structure			*/
/************************************************/
typedef struct{
    qword GIFtag;
    qword Bitblt;
    qword Trxpos;
    qword Trxreg;
    qword Trxdir;
} TextTransRegs __attribute__ ((aligned(16)));

typedef struct _texflush{
    qword GIFtexflush;		/* flush giftag */
    qword Texflush;		/* flush regs */
}Texflush __attribute__ ((aligned(16)));

typedef struct{
    TextTransRegs texTransRegs;	/* texel transfer gs regs */
    TextTransRegs clutTransRegs;/* clut transfer gs regs */
    qword texGIFtag;		/* texel giftag */
    qword clutGIFtag;		/* clut giftag */
    u_int texBp;		/* TBP (texel base addr at GS) */
    u_int texGsSize;		/* blocks (aligned page break) */
    u_int texTransSize; 	/* qwords */
    u_int clutBp;		/* CBP (clut base addr at GS) */
    u_int clutGsSize;		/* blocks (aligned page break) */
    u_int clutTransSize;	/* qwords */
}Image __attribute__ ((aligned(16)));

typedef struct{
    Image 	*image;		/* upper image array */
    int		resident;	/* resident flag */
    int		id;		/* pbuf id */
    sceHiGsMemTbl	*tbl;	/* resident auto mem */
}TEXTURE_FRAME;

/***************************************************
 * Local Errors
 ***************************************************/
typedef enum {
    _NO_HEAP,
    _PLGBLK_STACK_BROKEN,
    _NO_GS_HEAP,
    _TEX2D_DATA_BROKEN,
    _TEXENV_BROKEN
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA
};

static const char *_tex2d_err_mes[] = {
    "HiP Tex2d : can't allocate memory\n",
    "HiP Tex2d : plug stack is null\n",
    "HiP Tex2d : can't malloc Gs heap\n",
    "HiP Tex2d : tex2d data was broken\n",
    "HiP Tex2d : texenv data was broken\n"
};

/* error handling */
static sceHiErr _hip_tex2d_err(_local_err err)
{
    sceHiErrState.mes = _tex2d_err_mes[err];
    return _local2higerr[err];
}

/************************************************/
/*	       	Packed from Gs addr		*/
/************************************************/
static u_long *getPackedData(sceHiGsPacked *p, u_char addr)
{
    int i,n;

    n = p->giftag->nloop;

    for(i=0;i<n;i++){
	if(addr == p->packed[i].addr)
	    return (u_long *)&(p->packed[i]);
    }

    return NULL;
}

/************************************************/
/*		Texture Load			*/
/************************************************/
static sceHiErr Vu1TexInit(TEXTURE_FRAME *tex, sceHiPlugTex2DHead_t *dataH, sceHiPlugTex2DHead_t *envH, sceHiGsMemTbl *tbl)
{
    int		i;
    u_int		tbp, cbp;
    sceHiGsPacked	pkd = { NULL, NULL };
    sceHiPlugTex2DData_t	*tex2d;
    int		envidx, psm, tbw, cpsm = 0, cbw = 0;
    Image	*image;
    u_int	texWidth, texHeight, clutWidth, clutHeight;
    

    tex->image = (Image *) sceHiMemAlign(16, sizeof(Image) * dataH->num);
    if (tex->image == NULL)
	return _hip_tex2d_err(_NO_HEAP);
    image = tex->image;
    if (tbl != NULL) {
	tbp = tbl->addr / SCE_HIGS_BLOCK_ALIGN;
	cbp = (tbl->addr + tbl->size) / SCE_HIGS_BLOCK_ALIGN;
    } else {
	tbp = INIT_TBP;
	cbp = INIT_CBP;
    }

    for (envidx = i = 0; i < dataH->num; i++) {
	tex2d = sceHiPlugTex2DGetData(dataH, i);
	if (tex2d == NULL)	return _hip_tex2d_err(_TEX2D_DATA_BROKEN);
	if (envH && !tex2d->info.mipmap.miplv) {
	    pkd.giftag = sceHiPlugTex2DGetEnv(envH, envidx++);
	    if (pkd.giftag == NULL) return _hip_tex2d_err(_TEXENV_BROKEN);
	    pkd.packed = (sceHiGsPacked_t *)(pkd.giftag + 1);
	}
	tex2d->info.addr = SCE_GS_TEX0_1;
	psm = tex2d->tex0.PSM;
	image[i].texTransSize = (tex2d->texelsize + 3) / 4;
	texHeight = tex2d->texelheight;
	texWidth = tex2d->texelwidth;
	if ((psm == SCE_GS_PSMT8) || (psm == SCE_GS_PSMT4))
	    tbw = 2 * ((texWidth + 127) / 128);
	else
	    tbw = (texWidth + 63) / 64;
	image[i].texGsSize = sceHiGsPageSize(texWidth, texHeight, psm) / SCE_HIGS_BLOCK_ALIGN;
	image[i].texBp = tbp;
	tbp += image[i].texGsSize;
	if (tbp > cbp)
	    tbp = cbp;
	tex2d->tex0.TBP0 = image[i].texBp;
	tex2d->tex0.TBW = tbw;

	image[i].clutTransSize = (tex2d->clutsize + 3) / 4;
	if (image[i].clutTransSize) {
	    cpsm = tex2d->tex0.CPSM;
	    clutHeight = tex2d->clutheight;
	    clutWidth = tex2d->clutwidth;
	    cbw = 1;
	    image[i].clutGsSize = sceHiGsBlockSize(clutWidth, clutHeight, cpsm) / SCE_HIGS_BLOCK_ALIGN;
	    cbp -= image[i].clutGsSize;
	    if (cbp < tbp)
		cbp = tbp;
	    image[i].clutBp = cbp;
	    tex2d->tex0.CBP = image[i].clutBp;
	    tex2d->tex0.CLD = 2;
	} else {
	    clutWidth = 0;
	    clutHeight = 0;
	    image[i].clutGsSize = 0;
	}

	if (pkd.giftag) {
	    sceGsTex0		*tex0 = NULL;
	    sceGsMiptbp1	*tbp1 = NULL;
	    sceGsMiptbp2	*tbp2 = NULL;

	    switch (tex2d->info.mipmap.miplv) {
	      case 0:
		tex0 = (sceGsTex0 *) getPackedData(&pkd, SCE_GS_TEX0_1);
		if (tex0 != NULL) {
		    tex0->TBP0 = tex2d->tex0.TBP0;
		    tex0->TBW = tex2d->tex0.TBW;
		    tex0->CBP = tex2d->tex0.CBP;
		    tex0->CLD = tex2d->tex0.CLD;
		}
		break;
	      case 1:
		tbp1 = (sceGsMiptbp1 *)getPackedData(&pkd, SCE_GS_MIPTBP1_1);
		if (tbp != NULL) {
		    tbp1->TBP1 = tex2d->tex0.TBP0;
		    tbp1->TBW1 = tex2d->tex0.TBW;
		}
		break;
	      case 2:
		tbp1 = (sceGsMiptbp1 *)getPackedData(&pkd, SCE_GS_MIPTBP1_1);
		if (tbp1 != NULL) {
		    tbp1->TBP2 = tex2d->tex0.TBP0;
		    tbp1->TBW2 = tex2d->tex0.TBW;
		}
		break;
	      case 3:
		tbp1 = (sceGsMiptbp1 *)getPackedData(&pkd, SCE_GS_MIPTBP1_1);
		if (tbp1 != NULL) {
		    tbp1->TBP3 = tex2d->tex0.TBP0;
		    tbp1->TBW3 = tex2d->tex0.TBW;
		}
		break;
	      case 4:
		tbp2 = (sceGsMiptbp2 *)getPackedData(&pkd, SCE_GS_MIPTBP2_1);
		if (tbp2 != NULL) {
		    tbp2->TBP4 = tex2d->tex0.TBP0;
		    tbp2->TBW4 = tex2d->tex0.TBW;
		}
		break;
	      case 5:
		tbp2 = (sceGsMiptbp2 *)getPackedData(&pkd, SCE_GS_MIPTBP2_1);
		if (tbp2 != NULL) {
		    tbp2->TBP5 = tex2d->tex0.TBP0;
		    tbp2->TBW5 = tex2d->tex0.TBW;
		}
		break;
	      case 6:
		tbp2 = (sceGsMiptbp2 *)getPackedData(&pkd, SCE_GS_MIPTBP2_1);
		if (tbp2 != NULL) {
		    tbp2->TBP6 = tex2d->tex0.TBP0;
		    tbp2->TBW6 = tex2d->tex0.TBW;
		}
		break;
	    }
	}

	/* GIFtag and GSregs for texel host->local transfer */
	SET_QW(image[i].texTransRegs.GIFtag, 0x0, 0x0000000e, 0x10000000, 0x00008004 );
	SET_QW(image[i].texTransRegs.Bitblt, 0x0, 0x00000050, 
	       ((psm<<(GS_BITBLTBUF_DPSM_O-32))
		|(tbw<<(GS_BITBLTBUF_DBW_O-32))
		| (image[i].texBp<<(GS_BITBLTBUF_DBP_O-32))),
	       0x00000000 );
	SET_QW(image[i].texTransRegs.Trxpos, 0x0, 0x00000051, 0x00000000, 0x00000000 );
	SET_QW(image[i].texTransRegs.Trxreg, 0x0, 0x00000052, texHeight, texWidth);
	SET_QW(image[i].texTransRegs.Trxdir, 0x0, 0x00000053, 0x00000000, 0x00000000 );

	/* GIFtag and GSregs for clut host->local transfer */
	if (image[i].clutTransSize>0){
	    SET_QW(image[i].clutTransRegs.GIFtag, 0x0, 0x0000000e, 0x10000000, 0x00008004 );
	    SET_QW(image[i].clutTransRegs.Bitblt, 0x0, 0x00000050, 
		   ((cpsm<<(GS_BITBLTBUF_DPSM_O-32))
		    |(cbw<<(GS_BITBLTBUF_DBW_O-32))
		    | (image[i].clutBp<<(GS_BITBLTBUF_DBP_O-32))),
		   0x00000000 );
	    SET_QW(image[i].clutTransRegs.Trxpos, 0x0, 0x00000051, 0x00000000, 0x00000000 );
	    SET_QW(image[i].clutTransRegs.Trxreg, 0x0, 0x00000052, clutHeight, clutWidth);
	    SET_QW(image[i].clutTransRegs.Trxdir, 0x0, 0x00000053, 0x00000000, 0x00000000 );
	}

	SET_QW(image[i].texGIFtag, 0x0, 0x0, 0x08000000, image[i].texTransSize|0x8000 );
	if (image[i].clutTransSize) {
	    SET_QW(image[i].clutGIFtag, 0x0, 0x0, 0x08000000, image[i].clutTransSize|0x8000 );
	}
    }
    return SCE_HIG_NO_ERR;
}

static const Texflush __attribute__ ((aligned(16))) texflush = {
    {0x00008001, 0x10000000, 0x0000000e, 0x0},
    {0x00000000, 0x00000000, 0x0000003f, 0x0}
};
/************************************************/
/*		Texture Packet			*/
/************************************************/
static u_int Vu1ResidentTextureMakePacket(TEXTURE_FRAME *tex, sceHiPlugTex2DHead_t *dataH)
{
    Image *image;
    int texID;
    int	id;
    u_int	*texel, *clut;
    sceHiPlugTex2DData_t	*data;

    if (tex != NULL) {
	sceHiDMAMake_ChainStart();
	for (texID = 0; texID < dataH->num; texID++) {
	    image = tex->image;
	    data = sceHiPlugTex2DGetData(dataH, texID);
	    if (data == NULL)	return _hip_tex2d_err(_TEX2D_DATA_BROKEN);
	    texel = sceHiPlugTex2DGetTexel(data);
	    clut = sceHiPlugTex2DGetClut(data);
	    sceHiDMAMake_WaitMicro();
	    sceHiDMAMake_LoadGS(image[texID].texTransRegs.GIFtag, 5);
	    sceHiDMAMake_LoadGS(image[texID].texGIFtag, 1);
	    sceHiDMAMake_LoadGS(texel, image[texID].texTransSize);
	    if (image[texID].clutTransSize > 0) {
		sceHiDMAMake_LoadGS(image[texID].clutTransRegs.GIFtag, 5);
		sceHiDMAMake_LoadGS(image[texID].clutGIFtag, 1);
		sceHiDMAMake_LoadGS(clut, image[texID].clutTransSize);
	    }
	    sceHiDMAMake_LoadGS((int *)&texflush, 2);
	}
	sceHiDMAMake_ChainEnd(&id);
    }

    return id;
}

/************************************************/
/*		Tex2D Plug			*/
/************************************************/
sceHiErr sceHiPlugTex2D(sceHiPlug *plug, int process)
{
    sceHiPlugTex2DHead_t	*dataH, *envH;
    sceHiErr	err;
    TEXTURE_FRAME	*tex_frame;
    sceHiGsMemTbl	*tbl = NULL;

    switch(process){
      case SCE_HIG_INIT_PROCESS:
	dataH = sceHiPlugTex2DGetHead(plug, tex2dt);
	envH = sceHiPlugTex2DGetHead(plug, texenvt);

	tex_frame = (TEXTURE_FRAME *)sceHiMemAlign(16, sizeof(TEXTURE_FRAME));
	if (tex_frame == NULL)
	    return _hip_tex2d_err(_NO_HEAP);

	if(plug->args != NULL){
	    tex_frame->resident = ((sceHiPlugTex2dInitArg_t *)plug->args)->resident ? RESIDENT : FALSE;
	    tbl = ((sceHiPlugTex2dInitArg_t *)plug->args)->tbl;
	}
	else{
	    tex_frame->resident = FALSE;
	    tbl = NULL;
	}

	if((tex_frame->resident == RESIDENT)&&(tbl == NULL)){
	    tbl = sceHiGsMemAlloc(SCE_HIGS_BLOCK_ALIGN, sceHiPlugTex2DSize(plug));
	    if(tbl == NULL)	return _hip_tex2d_err(_NO_GS_HEAP);
	    tex_frame->tbl = tbl;	/* for free */
	}
	else{
	    tex_frame->tbl = NULL;
	}

	  err = Vu1TexInit(tex_frame, dataH, envH, tbl);
	  if(err != SCE_HIG_NO_ERR) return err;
	  tex_frame->id = Vu1ResidentTextureMakePacket(tex_frame, dataH);

	  plug->stack = (u_int)tex_frame;
	  plug->args = NULL;
	  break;

      case SCE_HIG_POST_PROCESS:
	  tex_frame = (TEXTURE_FRAME *)plug->stack;
	  if(tex_frame == NULL) return _hip_tex2d_err(_PLGBLK_STACK_BROKEN);

	  if(tex_frame->resident == RESIDENT){
	      sceHiDMARegist(tex_frame->id);
	      tex_frame->resident = TRUE;
	  }
	  else if(tex_frame->resident == FALSE){
	      sceHiDMARegist(tex_frame->id);
	  }
	  break;

      case SCE_HIG_END_PROCESS:
	  tex_frame = (TEXTURE_FRAME *)plug->stack;
	  if(tex_frame == NULL) return _hip_tex2d_err(_PLGBLK_STACK_BROKEN);
	  sceHiDMADel_Chain(tex_frame->id);
	  if(tex_frame->tbl != NULL)	sceHiGsMemFree(tex_frame->tbl);
	  sceHiMemFree((u_int *)Paddr(tex_frame->image));
	  sceHiMemFree((u_int *)Paddr(tex_frame));
	  plug->stack = NULL;
	  plug->args = NULL;
	  break;

      default:
	  break;
    }

    return SCE_HIG_NO_ERR;
}

size_t sceHiPlugTex2DSize(sceHiPlug *plug)
{
    sceHiPlugTex2DData_t	*data;
    sceHiPlugTex2DHead_t	*dataH;
    size_t	size;
    int		num,i;

    dataH = sceHiPlugTex2DGetHead(plug, tex2dt);
    if (dataH == NULL)	return _hip_tex2d_err(_TEX2D_DATA_BROKEN);

    num = dataH->num;
    size = 0;
    for(i=0;i<num;i++){
	data = sceHiPlugTex2DGetData(dataH, i);
	if (data == NULL)	return _hip_tex2d_err(_TEX2D_DATA_BROKEN);
	if(data->texelsize)
	    size += sceHiGsPageSize(data->texelwidth, data->texelheight, data->tex0.PSM);
	if(data->clutsize)
	    size += sceHiGsBlockSize(data->clutwidth, data->clutheight, data->tex0.CPSM);
    }

    return size;
}
