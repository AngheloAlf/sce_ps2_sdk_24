/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: shape.c,v 1.28 2002/05/20 02:07:23 kaneko Exp $	*/
#include <eekernel.h>
#include <eestruct.h>
#include <stdlib.h>
#include <stdio.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)
#define QWSIZE (4)
#define MATRIXSIZE (8 * QWSIZE)

static const sceVu0FMATRIX m[2] = {
    {{1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0}}, 
    {{1.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 1.0}}
};

typedef enum {
    _POINT	= 0,
    _LINE	= 1,
    _LINE_STRIP	= 2,
    _TRIANGLE	= 3,
    _TRIANGLE_STRIP	= 4,
    _TRIANGLE_FAN	= 5,
    _SPRITE	= 6
} _primitive_t;

static const sceHiType shapedt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHAPE, SCE_HIG_DATA_STATUS, SCE_HIP_SHAPE_DATA, SCE_HIP_REVISION };
static const sceHiType basemt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SHAPE, SCE_HIG_DATA_STATUS, SCE_HIP_BASEMATRIX, SCE_HIP_REVISION };
static const sceHiType tex2dt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_TEX2D, SCE_HIG_DATA_STATUS, SCE_HIP_TEX2D_DATA, SCE_HIP_REVISION };
static const sceHiType texenvt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_TEX2D, SCE_HIG_DATA_STATUS, SCE_HIP_TEX2D_ENV, SCE_HIP_REVISION };
static const sceHiType skinbwt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SKIN, SCE_HIG_DATA_STATUS, SCE_HIP_SKIN_BW, SCE_HIP_REVISION };
static const sceHiType skinlwt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_SKIN, SCE_HIG_DATA_STATUS, SCE_HIP_SKIN_LW, SCE_HIP_REVISION };

/************************************************/
/*		Structure			*/
/************************************************/

enum {
    MASTER_CHAIN_IN_STATIC_O	= (1 << 0),
};

typedef struct{
    sceHiPlugShapeHead_t	*shapeH;
    sceHiPlugTex2DHead_t	*tex2dH;
    sceHiPlugTex2DHead_t	*texenvH;
    sceHiPlugShapeHead_t	*basemH;

    sceVu0IVECTOR		common_data_each_shape[2];

    u_int		flags;		/* flags(chain setting) */

    /* skin_pid and object_pid array's size is depend on */
    /* the number of shape that could get from shapeH */
    int				*skin_pid;	/* skin packet id */
    int				*object_pid;	/* object packet id */
    int				shape_pid;	/* shape frame packet id */
} SHAPE_FRAME;

/***************************************************
 * Local Errors
 ***************************************************/
typedef enum {
    _NO_HEAP,
    _PLGBLK_STACK_BROKEN,
    _MATIDX_IS_ILLEAGAL,
    _VU1_OVERFLOW,
    _GEOMETRY_DATA_BROKEN,
    _SKIN_DATA_BROKEN,
    _BASEMATRIX_BROKEN
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_VALUE,
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA
};

static const char *_shape_err_mes[] = {
    "HiP Shape : can't allocate memory\n",
    "HiP Shape : plug stack is null\n",
    "HiP Shape : illeagal matrix id\n",
    "HiP Shape : vu1 memory overflow\n",
    "HiP Shape : geometry data was broken\n",
    "HiP Shape : skin data was broken\n",
    "HiP Shape : basematrix data was broken\n"
};

/* error handling */
static sceHiErr _hip_shape_err(_local_err err)
{
    sceHiErrState.mes = _shape_err_mes[err];
    return _local2higerr[err];
}

/****************************************************************/
/* VU1 Memory management					*/
/****************************************************************/
#define  VUMEM1_OVERFLOW 1
#define  VUMEM1_OK 0

static u_int vumem1isize;
static u_int vumem1osize;
static u_int vumem1irest;
static u_int vumem1orest;
static u_int vumem1iptr;
static u_int vumem1optr;
static u_int vumem1istart;
static u_int vumem1ostart;

static void Vumem1SetParam(u_int, u_int);
static void Vumem1Reset(void);
static int Vumem1CheckRest(u_int isize, u_int osize);
static int Vumem1Rest(int ioffset, int i_per_vertex, int ooffset, int o_per_vertex);
#if 0
static int Vumem1IRest();
static int Vumem1ORest();
static int Vumem1OSize();
#endif
static u_int Vumem1GetIptr(u_int isize);
static u_int Vumem1GetOptr(u_int osize);
static int Vumem1ISize();

typedef union {
    u_long	ul[2];
    qword	qw;    
} GifTag_t;

/* GS register設定用buffer */
/*	
	1. PRMODECONT設定(= 1)	2 qwrod
	2. PRIM REG設定		2 qword
	3. PRMODECONT設定(= 0)	2 qword
  を行う
  PRIM AttributeはVU1で行う
*/
GifTag_t	_gs_set[6];

/* VU1 mem managerの初期化 */
static void Vumem1SetParam(u_int isize, u_int osize)
{
    vumem1isize=isize;
    vumem1osize=osize;
    vumem1istart=0;				/* unpackR を使うときには 0 */
    vumem1ostart=vumem1istart+isize;

    Vumem1Reset();
}

/* VU1 mem managerのリセット(初期値に戻す) */
static void Vumem1Reset()
{
    vumem1iptr=vumem1istart;
    vumem1optr=vumem1ostart;
    vumem1irest=vumem1isize;
    vumem1orest=vumem1osize;
}

/* VU1 mem のバッファチェック */
static int Vumem1CheckRest(u_int isize, u_int osize)
{
    if (vumem1irest<isize) return VUMEM1_OVERFLOW;
    if (vumem1orest<osize) return VUMEM1_OVERFLOW;
    return VUMEM1_OK;
}

static int Vumem1Rest(int ioffset, int i_per_vertex, int ooffset, int o_per_vertex)
{
    int imax, omax;
    imax=(vumem1irest-ioffset)/i_per_vertex;
    omax=(vumem1orest-ooffset)/o_per_vertex;
    if (imax>omax) return omax;
    else return imax;
}

/* In Bufferの残りを返す */
#if 0
static int Vumem1IRest()
{
    return vumem1irest;
}

/* Out Bufferの残りを返す */
static int Vumem1ORest()
{
    return vumem1orest;
}
#endif
/* In Bufferのサイズを返す */
static int Vumem1ISize()
{
    return vumem1isize;
}

/* Out Bufferのサイズを返す */
#if 0 
static int Vumem1OSize()
{
    return vumem1osize;
}
#endif

/* In Bufferから isize分の領域を割り当ててもらって そこへのpointerを返す */
static u_int Vumem1GetIptr(u_int isize)
{
    u_int iptr;

    iptr=vumem1iptr;
    if (isize>0){
	if (Vumem1CheckRest(isize, 0)!=VUMEM1_OK) {
		return 0xffffffff;
	}
	vumem1irest-=isize;
	vumem1iptr+=isize;
    }
    return iptr;
}

/* Out Bufferから osize分の領域を割り当ててもらって そこへのpointerを返す */
static u_int Vumem1GetOptr(u_int osize)
{
    u_int optr;

    optr=vumem1optr;
    if (osize>0){
	if (Vumem1CheckRest(0, osize)!=VUMEM1_OK) {
		return 0xffffffff;
	}
	vumem1orest-=osize;
	vumem1optr+=osize;
    }
    return optr;
}

/****************************************************************/
/*			Shape Packet				*/
/****************************************************************/

/* 
  ShapeGetGiftag()
  	Shapeで設定するGiftag部分の作成
	ここでは マイクロコードに依存せず、
	Shape形状のみに依存する部分に関する
	giftagを作成する
	すなわち
		NLOOP/EOP/PRIM
	の設定である
	PRIMの設定は Primitive/Attributeに分けて作成される
	Primitive自体はそのまま使用されるがAttributeは
	MicroCode依存部分(ex. FOG)などがあるため
	ここで作成されたものとmicro pluginで作成されたものが
	ORされて実際に使用される

	返値とVU1 Micro Cdeでの動作（括弧内で説明しているもの）
		giftag[0] : EOP | NLOOP	(OutbufferのGiftag部分に直接Copy)
		giftag[1] : Primitive	(PRIMレジスタに直接いれられる)
		prim_reg  : Attribute	(Micro Plug作成dataとORされPRMODEレジスタに)

       01.01.16
       	gs service functionsの設定を有効にするように変更
 */
static void ShapeGetGiftag(qword giftag, u_int *prim_reg, u_int attribute, u_int packsize, int eop)
{
    union {
	/* 上位64bitはshape pluginでは一切touchしない */
	/* Micro Pluginが 作成する */
	u_long128	ul128;
	u_long		ul64[2];
	u_int		ui32[4];
    } Giftag;

    u_int	prim, type, shade, tex, nloop = 0;
    sceHiGsGeneral	*gen;

    prim	= 0x03ff & attribute;
    type	= 0x0007 & attribute;
    shade	= 0x0001;		/* Gouraud */
    tex		= (attribute >> 4) & 0x01;    
    gen		= sceHiGsGeneralStatus();

    /* Primitive種によって loop回数が違う可能性がある(?)*/
    switch (type) {
      case 0:	/* point */
      case 1:	/* line */
      case 2:	/* line strip */
      case 3:	/* triangle */
      case 4:	/* triangle strip */
      case 5:	/* triangle fan */
      case 6:	/* sprite */       
	nloop = packsize;
	break;
      case 7: /* ? */
	break;
    }

    /* making primitive attribute(prim include Primitive/Attribute) */
    /* 1st clear Attribute by Mask */
    prim = prim & GS_PRIM_PRIM_M;
    if (shade && gen->prmode.IIP)
	prim |= (u_long) 1 << GS_PRIM_IIP_O;
    if (tex && gen->prmode.TME)
	prim |= (u_long) 1 << GS_PRIM_TME_O;
/*
    prim |= (u_long) shade << GS_PRIM_IIP_O;
    prim |= (u_long) tex   << GS_PRIM_TME_O;
*/
    *prim_reg = prim & (~GS_PRIM_PRIM_M); /* & ~(GS_PRIM_PRIM_M); attribute */

    /* making tag */
    Giftag.ul128 = 0;	/* clear */
    Giftag.ui32[0] = nloop;
    Giftag.ui32[1] = type; /* primitive type */
    if (eop)
	Giftag.ui32[0] |= 0x8000;

    giftag[0] = Giftag.ui32[0]; giftag[1] = Giftag.ui32[1];
    giftag[2] = Giftag.ui32[2]; giftag[3] = Giftag.ui32[3];
}

#define NUMIDX 3
/*
 *
 */
static const qword __attribute__ ((aligned(16))) giftex01 = {0x00008001,0x10000000,0x0000000e,0x0};

/* 
  primitive typeによって 最小pack vertex数が違う
*/
static inline int _get_pack_unit(_primitive_t prim_type)
{
    int	unit = 0;

    switch (prim_type) {
      case _POINT:
	unit = 4;
	break;
      case _LINE:
	unit = 4;
	break;
      case _LINE_STRIP:
	unit = 3;
	break;
      case _TRIANGLE:
	unit = 3;
	break;
      case _TRIANGLE_STRIP:
	unit = 4;
	break;
      case _TRIANGLE_FAN:
	unit = 4;
	break;
      case _SPRITE:
	unit = 1;
	break;
    }

    return unit;
}

/* 
  bufferサイズと残りのサイズ、最小パックサイズからpackする量を決定する
  また、パケットの区切とするべき状態であるならば*eop == 1にする
*/
static inline u_int _calc_pack_Vnum(u_int rest, int cycle, int *eop)
{
#define MIN_PACKNUM	(5)	/* 最低pack vertex数 */
#define	IN_OFFSET	(2)	/* send１回当りのgiftagが使用するinbufferの量 qword unit */
#define	IN_PER_VERTEX	(4)	/* vertex１コ当りのinbufferの量 qword unit */
#define OUT_OFFSET	(3)	/* send１回当りのgiftagが使用するoutbufferの量 qword unit */
#define OUT_PER_VERTEX	(4)	/* vertex１コ当りのoutbufferの量 qword unit */
    u_int	pack;
    u_int	buf;	/* Vu1 Memoryに入りきる最大のvertex数 */

    /* VU1 memoryに入りきる最大vertex数を得る */
    buf = Vumem1Rest(IN_OFFSET, IN_PER_VERTEX, OUT_OFFSET, OUT_PER_VERTEX);

    /* pack数はcycleの倍数でなくてはならない */
    pack = buf - (buf % cycle);

    /* 残り頂点数が Vu1 memoryに入るきる量よりすくなければ */
    if (rest <= pack)
	pack = rest; /* 全部入れる */

    /* 入れたのち vu1 memoryに十分なあきがなければEOP */
    *eop = 0;
    if (buf - pack < MIN_PACKNUM)
	*eop = 1;

    return pack;
}

/* 
  primitive種によっては、逆戻り(後方移動)させなければならない
  こともある
*/
static inline u_int _calc_backward_Vnum(_primitive_t prim_type)
{
    u_int	back = 0;

    switch (prim_type) {
      case _POINT:	
      case _LINE:
	back = 0;
	break;
      case _LINE_STRIP:
	back = 1;
	break;
      case _TRIANGLE:
	back = 0;
	break;
      case _TRIANGLE_STRIP:
	back = 2;
	break;
      case _TRIANGLE_FAN:
	back = 1; /* これに加えて例外処理転送1vertexあり */
	break;
      case _SPRITE:
	back = 0;
	break;
    }
    return back;
}

static sceHiErr ShapeMakeVertexPacket(sceHiPlugShapeHead_t *geom, u_int start, u_int pack, int eop)
{
    u_int	iptr;	/* in buffer pointer */
    sceHiErr	err;
    sceVu0FVECTOR	*vertex;
    sceVu0FVECTOR	*normal;
    sceVu0FVECTOR	*st;
    sceVu0FVECTOR	*color;

    vertex = sceHiPlugShapeGetGeometryVertex(geom, start);
    normal = sceHiPlugShapeGetGeometryNormal(geom, start);
    st = sceHiPlugShapeGetGeometryST(geom, start);
    color = sceHiPlugShapeGetGeometryColor(geom, start);
    if (vertex == NULL || normal == NULL || st == NULL || color == NULL)
	return _hip_shape_err(_GEOMETRY_DATA_BROKEN);

    /* pack数のvertexデータ */
    iptr = Vumem1GetIptr(4 * pack);
    if (iptr == 0xffffffff)
	return _hip_shape_err(_VU1_OVERFLOW);
    if ((err = sceHiDMAMake_LoadStep((u_int *)(iptr + 0), (u_int *)vertex, pack, 1, 3)) != SCE_HIG_NO_ERR)	return err;
    if ((err = sceHiDMAMake_LoadStep((u_int *)(iptr + 1), (u_int *)normal, pack, 1, 3)) != SCE_HIG_NO_ERR)	return err;
    if ((err = sceHiDMAMake_LoadStep((u_int *)(iptr + 2), (u_int *)st,     pack, 1, 3)) != SCE_HIG_NO_ERR)	return err;
    if ((err = sceHiDMAMake_LoadStep((u_int *)(iptr + 3), (u_int *)color,  pack, 1, 3)) != SCE_HIG_NO_ERR)	return err;

    if (eop)
	if ((err = sceHiDMAMake_ContinueMicro()) != SCE_HIG_NO_ERR)	return err;

    return SCE_HIG_NO_ERR;
}

static sceHiErr ShapeMakeVertexGiftag(sceHiPlugShapeHead_t *geom, u_int pack, int eop)
{
    qword	giftag1;
    qword	giftag2;
    u_int	iptr;	/* in buffer pointer */
    sceHiErr	err;
    
    ShapeGetGiftag(giftag1, &giftag2[0], geom->geo.prim, pack, eop);
    giftag2[1] = giftag2[2] = giftag2[3] = 0;

    /* giftagなどのdata２qword */
    iptr = Vumem1GetIptr(2);
    if (iptr == 0xffffffff)
	return _hip_shape_err(_VU1_OVERFLOW);
    if ((err = sceHiDMAMake_LumpStart((u_int *)iptr)) != SCE_HIG_NO_ERR)	return err;
    if ((err = sceHiDMAMake_Lump(giftag1)) != SCE_HIG_NO_ERR)			return err;
    if ((err = sceHiDMAMake_Lump(giftag2)) != SCE_HIG_NO_ERR)			return err;
    if ((err = sceHiDMAMake_LumpEnd()) != SCE_HIG_NO_ERR)			return err;

    return SCE_HIG_NO_ERR;
}

static sceHiErr ShapeMakeExceptionTriFan(sceHiPlugShapeHead_t *geom)
{
    u_int	iptr;	/* in buffer pointer */
    sceVu0FVECTOR	*vertex;
    sceVu0FVECTOR	*normal;
    sceVu0FVECTOR	*st;
    sceVu0FVECTOR	*color;
    sceHiErr	err;

    vertex = sceHiPlugShapeGetGeometryVertex(geom, 0);
    normal = sceHiPlugShapeGetGeometryNormal(geom, 0);
    st = sceHiPlugShapeGetGeometryST(geom, 0);
    color = sceHiPlugShapeGetGeometryColor(geom, 0);

    if (vertex == NULL || normal == NULL || st == NULL || color == NULL)
	return _hip_shape_err(_GEOMETRY_DATA_BROKEN);

    iptr = Vumem1GetIptr(4);
    if (iptr == 0xffffffff)
	return _hip_shape_err(_VU1_OVERFLOW);
    if ((err = sceHiDMAMake_LoadPtr((u_int *)(iptr + 0), (u_int *)vertex, 1)) != SCE_HIG_NO_ERR)	return err;
    if ((err = sceHiDMAMake_LoadPtr((u_int *)(iptr + 1), (u_int *)normal, 1)) != SCE_HIG_NO_ERR)	return err;
    if ((err = sceHiDMAMake_LoadPtr((u_int *)(iptr + 2), (u_int *)st,     1)) != SCE_HIG_NO_ERR)	return err;
    if ((err = sceHiDMAMake_LoadPtr((u_int *)(iptr + 3), (u_int *)color,  1)) != SCE_HIG_NO_ERR)	return err;
    return SCE_HIG_NO_ERR;
}

static sceHiErr ShapeMakeGeometryPacket(sceHiPlugShapeHead_t *geom, int lastflag)
{
    u_int		restVnum;	/* 操作残りvertex数 */
    u_int		packVnum;	/* packするvertex数 */
    u_int		start;		/* send開始vertex number */
    sceHiErr		err;
    int			cycle;		/* pack最小単位 */
    _primitive_t	prim_type;	/* primitive種 */
    int			optr;
    int			eop;

    prim_type = geom->geo.prim & 0x07;

    /* primitive種別な 最小pack単位を得る */
    cycle = _get_pack_unit(prim_type);

    /* 未転送vertexが存在する限り */	
    for (restVnum = geom->geo.num, start = 0; restVnum > 0; restVnum -= packVnum, start += packVnum) {

	/* packするvertex数 および EOPを決定する */
	if (prim_type == _TRIANGLE_FAN && start != 0) {
	    /*
	      triangle fanで geometry最初のデータでなければ 例外処理転送1 vertex分
	      restVnumに上乗せした状態で計算する
	    */
	    packVnum = _calc_pack_Vnum(restVnum + 1, cycle, &eop);
	} else
	    packVnum = _calc_pack_Vnum(restVnum, cycle, &eop);

	if (lastflag)
	    eop = 1;

	/* Geometry Packetはすべて Double Buffer内に作られる */
	if ((err = sceHiDMAMake_DBufStart()) != SCE_HIG_NO_ERR)	return err;

	/* PACKET : giftag  */ 
	if ((err = ShapeMakeVertexGiftag(geom, packVnum, eop)) != SCE_HIG_NO_ERR)	return err;
	optr = Vumem1GetOptr(3);	/* Giftagでoutbufferに3qword使われる */
	if (optr == 0xffffffff)
	    return _hip_shape_err(_VU1_OVERFLOW);

	/* triangle fanの場合、geometryの最初以外は 例外処理転送が存在する */
	/* PACKET: exception triangle fan */	
	if (prim_type == _TRIANGLE_FAN && start != 0) {
	    if ((err = ShapeMakeExceptionTriFan(geom)) != SCE_HIG_NO_ERR)		return err;
	    optr = Vumem1GetOptr(4);	/* exeption tri fanで outbufferに1qword使われる */
	    if (optr == 0xffffffff)
		return _hip_shape_err(_VU1_OVERFLOW);
	    packVnum--;	/* 例外処理転送1vertex分減らしておく */
	}

	/* startからpackVnum分転送する */
	err = ShapeMakeVertexPacket(geom, start, packVnum, eop);
	optr = Vumem1GetOptr(4 * packVnum);	/* packVnum * 4 qwordが outbufferで使われる */
	if (optr == 0xffffffff)
	    return _hip_shape_err(_VU1_OVERFLOW);
	if (err != SCE_HIG_NO_ERR)
	    return err;	    

	/* Double Bufferingを一旦終了させておく */
	if ((err = sceHiDMAMake_DBufEnd()) != SCE_HIG_NO_ERR)	return err;

	/* 終わりでなければbackwardする量を決定する */
	if (packVnum != restVnum) {
	    packVnum -= _calc_backward_Vnum(prim_type);
	}

	/* End of PacketならBufferをresetする */
	if (eop) {
	    Vumem1Reset();
	}
    }

    return SCE_HIG_NO_ERR;
}

static sceHiErr ShapeMakeMaterialPacket(SHAPE_FRAME *sf, int shape_idx, int material_idx)
{
    int		i;
    sceHiPlugShapeHead_t	*datH;
    sceHiPlugShapeHead_t	*matH;
    sceHiPlugTex2DData_t	*tex2d;
    sceHiPlugShapeHead_t	*geoH;
    sceHiGsGiftag		*envtag;
    sceVu0FMATRIX		*matattr;
    int				numofattr;
    sceHiGsGiftag		*textag;
    sceHiErr			err;
    int				eop;

    datH = sceHiPlugShapeGetDataHead(sf->shapeH, shape_idx);
    matH = sceHiPlugShapeGetMaterialHead(datH, material_idx);
    matattr = sceHiPlugShapeGetMaterialAttrib(matH);
    if (datH == NULL || matH == NULL)
	return _hip_shape_err(_GEOMETRY_DATA_BROKEN);
    numofattr = ((int *)(matH + 1))[3];	/* see also gformat.doc */

    if ((sf->texenvH != NULL) && (matH->mat.tex_num)) {
	textag = sceHiPlugTex2DGetEnv(sf->texenvH, matH->mat.tex_id);
	if (textag != NULL) {
	    envtag = sceHiPlugShapeGetMaterialGiftag(matH);
	    /* PACKET: send giftag for texture */
	    if ((err = sceHiDMAMake_WaitMicro()) != SCE_HIG_NO_ERR)
		return err;
	    if (envtag != NULL && envtag->nloop) {
		if ((err = sceHiDMAMake_LoadGS((u_int *)envtag, envtag->nloop + 1)) != SCE_HIG_NO_ERR)
		    return err;
	    }
	    if ((err = sceHiDMAMake_LoadGS((u_int *)textag, textag->nloop + 1)) != SCE_HIG_NO_ERR)
		return err;
	}
	/* PACKET: send material data */
	if (matattr != NULL) {
	    if ((err = sceHiDMAMake_LoadPtr((u_int *)24, (u_int *)matattr, numofattr)) != SCE_HIG_NO_ERR)
		return err;
	}
    } else if ((sf->tex2dH != NULL) && (matH->mat.tex_num)) {
	tex2d = sceHiPlugTex2DGetData(sf->tex2dH, matH->mat.tex_id);
	if (tex2d != NULL) {
	    /* PACKET: send giftag for texture */
	    if ((err = sceHiDMAMake_WaitMicro()) != SCE_HIG_NO_ERR)
		return err;
	    if ((err = sceHiDMAMake_LoadGS((u_int *)giftex01, 1)) != SCE_HIG_NO_ERR)
		return err;
	    if ((err = sceHiDMAMake_LoadGS((u_int *)&tex2d->tex0, 1)) != SCE_HIG_NO_ERR)
		return err;
	}
	/* PACKET: send material data */
	if (matattr != NULL) {
	    if ((err = sceHiDMAMake_LoadPtr((u_int *)24, (u_int *)matattr, numofattr)) != SCE_HIG_NO_ERR)
		return err;
	}
    } else {
	if (matattr != NULL) {
	    if ((err = sceHiDMAMake_WaitMicro()) != SCE_HIG_NO_ERR)
		return err;
	    if ((err = sceHiDMAMake_LoadPtr((u_int *)24, (u_int *)matattr, numofattr)) != SCE_HIG_NO_ERR)
		return err;
	}
    }

    /* PACKET: send each geometry */
    Vumem1Reset();
    for (i = 0; i < matH->mat.num; i++) {
	geoH = sceHiPlugShapeGetGeometryHead(matH, i);
	if (geoH == NULL)	return _hip_shape_err(_GEOMETRY_DATA_BROKEN);
	eop = i == matH->mat.num - 1 ? 1 : 0;
	err = ShapeMakeGeometryPacket(geoH, eop);
	if (err != SCE_HIG_NO_ERR)
	    return err;
    }
    return SCE_HIG_NO_ERR;
}

static sceHiErr ShapeMakeShapePacket(SHAPE_FRAME *sf, int idx)
{
    int		i;
    sceHiPlugShapeHead_t	*datH;
    sceHiErr	err;

    if ((err = sceHiDMAMake_ChainStart()) != SCE_HIG_NO_ERR)			return err;

    /* PACKET:send data to VU */
    if ((err = sceHiDMAMake_LoadPtr((u_int *)10, (u_int *)sf->common_data_each_shape, 2)) != SCE_HIG_NO_ERR)	return err;

    /* PACKET:calc view volume */
    if ((err = sceHiDMAMake_ExecMicro()) != SCE_HIG_NO_ERR)				return err;
    if ((err = sceHiDMAMake_ContinueMicro()) != SCE_HIG_NO_ERR)			return err;

    /* PACKET: send each material */
    datH = sceHiPlugShapeGetDataHead(sf->shapeH, idx);
    if (datH == NULL)	return _hip_shape_err(_GEOMETRY_DATA_BROKEN);
    for (i = 0; i < datH->dat.num; i++) {
	err = ShapeMakeMaterialPacket(sf, idx, i);
	if (err != SCE_HIG_NO_ERR)
	    return err;
    }
    if ((err = sceHiDMAMake_ChainEnd(&sf->object_pid[idx])) != SCE_HIG_NO_ERR)		return err;

    return SCE_HIG_NO_ERR;
}

static sceHiErr ShapeFrameMakeSubPacket(SHAPE_FRAME *sf)
{
    int i;
    sceHiErr	err;

    sf->object_pid = (int *)sceHiMemAlign(16, sizeof(int) * sf->shapeH->top.num);
    if (sf->object_pid == NULL)
	return _hip_shape_err(_NO_HEAP);

    for (i = 0; i < sf->shapeH->top.num; i++) {
	err = ShapeMakeShapePacket(sf, i);
	if (err != SCE_HIG_NO_ERR)
	    return err;
    }
    return SCE_HIG_NO_ERR;
}

static sceHiErr ShapeFrameMakePacket(SHAPE_FRAME *sf)
{
    int	i;
    sceHiErr err;
    sceHiPlugShapeMatrix_t	*daddr;

    /* 全objectを統括する Packet列生成 */
    if ((err = sceHiDMAMake_DynamicChainStart()) != SCE_HIG_NO_ERR)
	return err;
    _gs_set[0].ul[0] = SCE_GIF_SET_TAG(1, 1, 0, 0, 0, 1);
    _gs_set[0].ul[1] = (long)SCE_GIF_PACKED_AD;
    _gs_set[1].ul[0] = 0;	/* PRMODCONT = 0 */
    _gs_set[1].ul[1] = (long)SCE_GS_PRMODECONT;
    if ((err = sceHiDMAMake_LoadGS((u_int *)_gs_set, 2)))
	return err;
    /* Matrixの埋め込み&object ID call */
    for (i = 0; i < sf->basemH->top.num; i++) {
	/* 先頭の第１ビット目で visible/invisible */
	/* ただしマトリクスデータを持たないものは適用不可 */
	daddr = sceHiPlugShapeGetMatrix(sf->basemH, i);
	if (daddr == NULL || (!(daddr->flags & 0x1))) {

	    if(sf->skin_pid[i] != -1){
		err = sceHiDMAMake_CallID(sf->skin_pid[i]);
		if(err != SCE_HIG_NO_ERR) return err;
	    }

	    if (daddr->shape != -1) {
		/*	shape packet	*/
		if ((err = sceHiDMAMake_LoadPtr((u_int *)0, (u_int *)daddr->local, 8)) != SCE_HIG_NO_ERR)	return err;
		if ((err = sceHiDMAMake_CallID(sf->object_pid[daddr->shape])) != SCE_HIG_NO_ERR) 		return err;
	    }
	}
    }

    /* 最後に PRMODCONT = 1にしておかなければならない・・・ */
    if ((err = sceHiDMAMake_WaitMicro()))	return err;
    _gs_set[2].ul[0] = SCE_GIF_SET_TAG(1, 1, 0, 0, 0, 1);
    _gs_set[2].ul[1] = (long)SCE_GIF_PACKED_AD;
    _gs_set[3].ul[0] = 1;	/* PRMODCONT = 1 */
    _gs_set[3].ul[1] = (long)SCE_GS_PRMODECONT;
    if ((err = sceHiDMAMake_LoadGS((u_int *)&(_gs_set[2]), 2)))
	return err;

    if ((err = sceHiDMAMake_DynamicChainEnd()))
	return err;

    return SCE_HIG_NO_ERR;
}

static sceHiErr ShapeFrameMakePacketInStatic(SHAPE_FRAME *sf)
{
    int i;
    sceHiErr err;
    sceHiPlugShapeMatrix_t	*daddr;

    if ((err = sceHiDMAMake_ChainStart()) != SCE_HIG_NO_ERR)
	return err;
    _gs_set[0].ul[0] = SCE_GIF_SET_TAG(1, 1, 0, 0, 0, 1);
    _gs_set[0].ul[1] = (long) SCE_GIF_PACKED_AD;
    _gs_set[1].ul[0] = 0;
    _gs_set[1].ul[1] = (long) SCE_GS_PRMODECONT;
    if ((err = sceHiDMAMake_LoadGS((u_int *)_gs_set, 2)))
	return err;
    for (i = 0; i < sf->basemH->top.num; i++) {
	daddr = sceHiPlugShapeGetMatrix(sf->basemH, i);
	if (daddr == NULL || (!(daddr->flags & 1))) {
	    if(sf->skin_pid[i] != -1){
		err = sceHiDMAMake_CallID(sf->skin_pid[i]);
		if(err != SCE_HIG_NO_ERR) return err;
	    }

	    if (daddr->shape != -1) {
		if ((err = sceHiDMAMake_LoadPtr((u_int *)0, (u_int *)daddr->local, 8)) != SCE_HIG_NO_ERR)
		    return err;
		if ((err = sceHiDMAMake_CallID(sf->object_pid[daddr->shape])) != SCE_HIG_NO_ERR)
		    return err;
	    }
	}
    }

    if ((err = sceHiDMAMake_WaitMicro()))
	return err;
    _gs_set[2].ul[0] = SCE_GIF_SET_TAG(1, 1, 0, 0, 0, 1);
    _gs_set[2].ul[1] = (long) SCE_GIF_PACKED_AD;
    _gs_set[3].ul[0] = 1;
    _gs_set[3].ul[1] = (long) SCE_GS_PRMODECONT;
    if ((err = sceHiDMAMake_LoadGS((u_int *)&(_gs_set[2]), 2)))
	return err;
    if ((err = sceHiDMAMake_ChainEnd(&sf->shape_pid)))
	return err;
    return SCE_HIG_NO_ERR;
}

#define NBONE	4	/* max 4 bone unit */
#define QWORD	4	/* qword size */
#define MTXID	0	/* matrix offset */
static sceHiErr set_skin_id(sceHiPlug *p, SHAPE_FRAME *sf)
{
    sceHiPlugSkinHead_t *h[2];	/* BW, LW header */
    sceVu0FMATRIX	*mtx;	/* LW matrix */
    int			*mid;	/* BW index */
    sceHiDMAChainID_t	*cid;	/* chain id */
    sceHiErr		err;
    int i;
    
    /*	initialize	*/
    sf->skin_pid = (int *) sceHiMemAlloc(sizeof(int) * sf->basemH->top.num);
    if (sf->skin_pid == NULL)
	return _hip_shape_err(_NO_HEAP);
    for (i = 0; i < sf->basemH->top.num; i++) {
	sf->skin_pid[i] = -1;
    }

    /*	get SKIN_BW & SKIN_LW	*/
    h[0] = sceHiPlugSkinGetHead(p, skinbwt);
    if(h[0] == NULL)
	return SCE_HIG_NO_ERR;
    h[1] = sceHiPlugSkinGetHead(p, skinlwt);
    if(h[1] == NULL)
	return SCE_HIG_NO_ERR;

    /*	get LW 1st matrix ptr	*/
    mtx = sceHiPlugSkinGetLW(h[1], 0);

    /*	get BW id ptr	*/
    mid = sceHiPlugSkinGetBW(h[0], h[0]->num);

    if (mtx == NULL || mid == NULL)
	return _hip_shape_err(_SKIN_DATA_BROKEN);

    /* create dma packet for LW */
    for(i = 0; i < h[1]->num / NBONE; i++){

	cid = &(sf->skin_pid[mid[MTXID]]);	/* skin dma id pointer */

	err = sceHiDMAMake_ChainStart();
	if(err != SCE_HIG_NO_ERR) return err;
	err = sceHiDMAMake_WaitMicro();			/* for single buffer */
	if(err != SCE_HIG_NO_ERR) return err;
	err = sceHiDMAMake_LoadPtr((u_int *)72, (u_int *)mtx, NBONE*QWORD); /* matrix[0-4] to vu1mem 72 */
	if(err != SCE_HIG_NO_ERR) return err;
	err = sceHiDMAMake_ChainEnd(cid);
	if(err != SCE_HIG_NO_ERR) return err;

	mtx += NBONE;	/* next 4 matrix */
	mid += 2;		/* next dma id */
    }

    return SCE_HIG_NO_ERR;
}

static sceHiErr free_skin_packet(SHAPE_FRAME *sf)
{
    sceHiErr		err;
    int i;

    /* delete chain */
    for(i=0;i<sf->basemH->top.num;i++){
	if (sf->skin_pid[i] != -1){
	    err = sceHiDMADel_Chain(sf->skin_pid[i]);
	    if(err != SCE_HIG_NO_ERR)
		return err;
	}
    }
    return SCE_HIG_NO_ERR;
}

/************************************************/
/*		Shape Plug			*/
/************************************************/
sceHiErr sceHiPlugShape(sceHiPlug *plug, int process)
{
    sceHiErr	err;
    SHAPE_FRAME	*shape_frame;
    int		i;

    switch(process){
      case SCE_HIG_INIT_PROCESS:

	shape_frame = (SHAPE_FRAME *) sceHiMemAlign(16, sizeof(SHAPE_FRAME));
	if (shape_frame == NULL)
	    return _hip_shape_err(_NO_HEAP);
	shape_frame->flags = 0;
	
	shape_frame->shapeH = sceHiPlugShapeGetHead(plug, shapedt);
	if (shape_frame->shapeH == NULL)
	    return _hip_shape_err(_GEOMETRY_DATA_BROKEN);
	shape_frame->tex2dH = sceHiPlugTex2DGetHead(plug, tex2dt);
	shape_frame->texenvH = sceHiPlugTex2DGetHead(plug, texenvt);
	shape_frame->basemH = sceHiPlugShapeGetHead(plug, basemt);

	{
		u_int bufsize = (1024 - 120) / 2;
		u_int isize_per_vtx = 4;
		u_int osize_per_vtx = 5;
		u_int vnum, isize, osize;

		vnum = bufsize / (isize_per_vtx + osize_per_vtx);
		osize = vnum * osize_per_vtx;
		isize = bufsize - osize;
		Vumem1SetParam(isize, osize);

		shape_frame->common_data_each_shape[0][0] = Vumem1ISize();
	}

	/*	Dma Packet for vu1 skin	*/
	err = set_skin_id(plug, shape_frame);
	if(err != SCE_HIG_NO_ERR) return err;

	err = ShapeFrameMakeSubPacket(shape_frame);
	if(err != SCE_HIG_NO_ERR) return err;

	if (shape_frame->flags & MASTER_CHAIN_IN_STATIC_O)
	    err = ShapeFrameMakePacketInStatic(shape_frame);

	if(err != SCE_HIG_NO_ERR) return err;

	plug->stack = (u_int)shape_frame;	/* push stack */
	break;

      case SCE_HIG_PRE_PROCESS:
	break;

      case SCE_HIG_POST_PROCESS:
	shape_frame = (SHAPE_FRAME *)plug->stack;	/* pop stack */
	if(shape_frame == NULL) return _hip_shape_err(_PLGBLK_STACK_BROKEN);

	if (!(shape_frame->flags & MASTER_CHAIN_IN_STATIC_O))
	    /* dynamic send */
	    err = ShapeFrameMakePacket(shape_frame);
	else
	    err = sceHiDMARegist(shape_frame->shape_pid);
	if (err != SCE_HIG_NO_ERR)
	    return err;
	break;

      case SCE_HIG_END_PROCESS:
	shape_frame = (SHAPE_FRAME *)plug->stack;	/* pop stack */
	if(shape_frame == NULL) return _hip_shape_err(_PLGBLK_STACK_BROKEN);

	/* free packets */
	err = free_skin_packet(shape_frame);
	if (err != SCE_HIG_NO_ERR) return err;
	if (shape_frame->flags & MASTER_CHAIN_IN_STATIC_O) {
	    err = sceHiDMADel_Chain(shape_frame->shape_pid);
	    if (err != SCE_HIG_NO_ERR)
		return err;
	}
	for (i = 0; i < shape_frame->shapeH->top.num; i++) {
	    err = sceHiDMADel_Chain(shape_frame->object_pid[i]);
	    if (err != SCE_HIG_NO_ERR)
		return err;
	}

	/* memory free */
	sceHiMemFree((u_int *)Paddr(shape_frame->skin_pid));
	sceHiMemFree((u_int *)Paddr(shape_frame->object_pid));
	sceHiMemFree((u_int *)Paddr(shape_frame));

	plug->stack = NULL;
	plug->args = NULL;
	break;
      default:
	break;
    }

    return SCE_HIG_NO_ERR;
}

/*
 * matrixID  matidxに対応したシェープの表示・非表示設定をする
 */
sceHiErr sceHiPlugShapeInvisible(sceHiPlug *plug, int matidx, int flag)
{
    sceHiErr err;
    SHAPE_FRAME	*sf;
    sceHiPlugShapeMatrix_t	*matdata;

    sf = (SHAPE_FRAME *)plug->stack;
    if (sf == NULL)
	return _hip_shape_err(_PLGBLK_STACK_BROKEN);

    /* change flags in Matrix-Data */
    matdata = sceHiPlugShapeGetMatrix(sf->basemH, matidx);
    if (sf->basemH->top.num <= matidx)
	return _hip_shape_err(_MATIDX_IS_ILLEAGAL);
    flag = ((flag & 0x0001) << 0);
    matdata->flags &= (~1);
    matdata->flags |= flag;

    /* if the master chain is static, re-make it */
    if (sf->flags & MASTER_CHAIN_IN_STATIC_O) {
	/* delete it */
	if ((err = sceHiDMADel_Chain(sf->shape_pid)) != SCE_HIG_NO_ERR)
	    return err;
	/* re-make it */
	if ((err = ShapeFrameMakePacketInStatic(sf)) != SCE_HIG_NO_ERR)
	    return err;
    }

    return SCE_HIG_NO_ERR;
}

/*
 * ShapeパケットのMaster ChainのDynamic/Static切換え等設定をする
 *
 * NOTE: DON'T CALL this function AFTER PRE_PROCESS.
 * -- sequence
 *	sceHiCallPlug(plug, INIT_PROCESS)
 *	.
 *	.
 *	while (1) { // main-loop
 *		.
 *		.
 *	    sceHiPlugShapeMasterChainSetting()	// this function
 *		.
 *		.
 *	    sceHiCallPlug(plug, PRE_PROCESS)
 *	    sceHiCallPlug(plug, POST_PROCESS)
 *	    sceHiDMASend();
 *		.
 *		.
 *	}
 * -- illeagal
 *	while (1) {
 *	    sceHiCallPlug(plug, PRE_PROCESS)
 *	    sceHiCallPlug(plug, POST_PROCESS)
 *	    sceHiPlugShapeMasterChainSetting()	// this function
 *	    sceHiDMASend();
 *	}
 *
 */
sceHiErr sceHiPlugShapeMasterChainSetting(sceHiPlug *plug, int flag)
{
    /* flag ... 0 bit : 0 then Dynamic, 1 then Static */
    sceHiErr err;
    SHAPE_FRAME *sf;

    sf = (SHAPE_FRAME *)plug->stack;
    if (sf == NULL)
	return _hip_shape_err(_PLGBLK_STACK_BROKEN);

    /* check flags */
    if ((sf->flags & MASTER_CHAIN_IN_STATIC_O) != (flag & MASTER_CHAIN_IN_STATIC_O)) {
	if (flag & MASTER_CHAIN_IN_STATIC_O) {
	    /* dynamic -> static */
	    /* note : NOW, none master chain here. just need to make master chain in static */
	    if ((err = ShapeFrameMakePacketInStatic(sf)) != SCE_HIG_NO_ERR)
		return err;
	} else {
	    /* static -> dynamic */
	    /* note : need to delete static chain. the master chain will be made later in POST process*/
	    if ((err = sceHiDMADel_Chain(sf->shape_pid)) != SCE_HIG_NO_ERR)
		return err;
	}
    }

    /* copy flag */
    sf->flags = flag;

    return SCE_HIG_NO_ERR;
}
