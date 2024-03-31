/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.5.3
 */
/*
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 */
/*	$Id: tim2.c,v 1.10 2002/05/20 02:07:23 kaneko Exp $	*/
#include <eekernel.h>
#include <stdlib.h>
#include <eeregs.h>
#include <libgraph.h>
#include <libvu0.h>
#include <libhig.h>
#include <libhip.h>

#define __DEBUG	0

#define PADDR_MASK 0x0fffffff
#define Paddr(x) ((u_int)(x)&PADDR_MASK)

static const sceHiType tim2dt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_TIM2, SCE_HIG_DATA_STATUS, SCE_HIP_TIM2_DATA, SCE_HIP_REVISION };
static const sceHiType texenvt = { SCE_HIP_COMMON, SCE_HIP_FRAMEWORK, SCE_HIP_TEX2D, SCE_HIG_DATA_STATUS, SCE_HIP_TEX2D_ENV, SCE_HIP_REVISION };

/* Type Definition */
typedef struct {
    u_int	reserve[3];
    u_int	num;
} _tim2_data_t;
typedef struct {
    u_int	tim2id;
    u_int	*dataptr;
    size_t	size;
    size_t	nameLength;
    char	fname[sizeof(qword)];
} _tim2_item_t;

static const int psmtbl[] = {
    SCE_GS_PSMCT16,
    SCE_GS_PSMCT24,
    SCE_GS_PSMCT32,
    SCE_GS_PSMT4,
    SCE_GS_PSMT8
};

#define SET_QW(qw, three, two, one, zero)  \
        (qw)[3]=three; (qw)[2]=two; (qw)[1]=one; (qw)[0]=zero;

#define QWSIZE (4)

#define INIT_TBP 0x1a40	/* default tbp for safety */
#define INIT_CBP 0x4000	/* default cbp for safety */
#define RESIDENT 2	/* first texture transfer flag */

enum TIM2_gattr_type {
    TIM2_NONE = 0,	// CLUTデータなしのときにClutTypeで使用
    TIM2_RGB16,	// 16bitカラー(ClutType,ImageType両方で使用)
    TIM2_RGB24,	// 24bitカラー(ImageTypeでのみ使用)
    TIM2_RGB32,	// 32bitカラー(ClutType,ImageType両方で使用)
    TIM2_IDTEX4,	// 16色テクスチャ(ImageTypeでのみ使用)
    TIM2_IDTEX8	// 16色テクスチャ(ImageTypeでのみ使用)
};

typedef struct {
    u_char FileId[4];		// ファイルID
    u_char FormatVersion;	// ファイルフォーマットバージョン
    u_char FormatId;		// フォーマットID
    u_short Pictures;		// ピクチャデータ個数
    u_char pad[8];		// 16バイトアラインメント
} TIM2_FILEHEADER;

typedef struct {
    u_int TotalSize;		// ピクチャデータ全体のバイトサイズ
    u_int ClutSize;		// CLUTデータ部のバイトサイズ
    u_int ImageSize;		// イメージデータ部のバイトサイズ
    u_short HeaderSize;		// 画像ヘッダサイズ
    u_short ClutColors;		// クラットサイズ（クラット部の色数）
    u_char PictFormat;		// 画像ID
    u_char MipMapTextures;	// MIPMAP枚数
    u_char ClutType;		// クラット部種別
    u_char ImageType;		// イメージ部種別
    u_short ImageWidth;		// 画像横サイズ(ビット数ではありません)
    u_short ImageHeight;	// 画像縦サイズ(ビット数ではありません)
    
    u_long GsTex0;		// TEX0
    u_long GsTex1;		// TEX1
    u_int GsTexaFbaPabe;	// TEXA, FBA, PABEの混合ビット
    u_int GsTexClut;		// TEXCLUT(下位32bitそのまま)
} TIM2_PICTUREHEADER;

typedef struct {
    u_long GsMiptbp1;		// MIPTBP1(64ビットそのまま)
    u_long GsMiptbp2;		// MIPTBP2(64ビットそのまま)
    u_int MMImageSize[0];	// MIPMAP[?]枚目の画像バイトサイズ
} TIM2_MIPMAPHEADER;

typedef struct {
    int		num;
    int		resident;	/* resident flag */
    int		id;

    int		pic;		/* the picture number that is viewing*/
    u_int	*pbufaddr[8];	/* 
				   the dma buffer-address of picture 
				   source address 
				   max picture(mipmap lv) is 7(0 - 6).
				   pbufadddr[7] for clut address.
				*/
} TIM2_FRAME;

/* local erros */
typedef enum {
    _NOT_TIM2_FILE,
    _VERSION_MISMATCH,
    _HUGE_ARGUMENT_VALUE,
    _INVALID_ALIGN_ID,
    _NO_IMAGE_DATA,
    _INVALID_CLUT_FORMAT_ID,
    _NO_HEAP,
    _PLGBLK_STACK_BROKEN,
    _ADDR_IS_NOT_SET_YET,
    _TIM2_DATA_BROKEN,
    _TEXENV_BROKEN
} _local_err;

static const sceHiErr _local2higerr[] = {
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_VALUE,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_NO_HEAP,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA,
    SCE_HIG_INVALID_DATA
};

static const char *_tim2_err_mes[] = {
    "HiP Tim2 : this is not TIM2 data file\n",
    "HiP Tim2 : this TIM2 data is not allowed\n",
    "HiP Tim2 : the argument is too huge\n",
    "HiP Tim2 : invalid align ID\n",
    "HiP Tim2 : no image data\n",
    "HiP Tim2 : invalid clut format ID\n",
    "HiP Tim2 : can't allocate memory from HiG Heap\n",
    "HiP Tim2 : plug stack is broken",
    "HiP Tim2 : tim2 data address is NULL.\n",
    "HiP Tim2 : tim2 data broken\n"
    "HiP Tim2 : texenv data broken\n"
};

/* */

static u_int	_init_tbp;
static u_int	_init_cbp;
static u_int	_next_tbp;
static u_int	_next_cbp;

static TIM2_FRAME	*_curtf;	/* current tim2frame for save dam buffer pointer */
static int	_curimg;		/* current image number for save dma buffer */

/* static function definition */
static sceHiErr CheckTim2Header(u_int *);
static sceHiErr GetTim2PictureHeader(int *, int, TIM2_PICTUREHEADER **);
static sceHiErr MakeTim2TexturePacket(int, u_int, int, int, int, int, int, u_long128 *);
static sceHiErr DecodeTim2Image(TIM2_PICTUREHEADER *, sceHiGsGiftag *);
static sceHiErr DecodeTim2Clut(TIM2_PICTUREHEADER *, sceHiGsGiftag *);
static sceHiErr DecodeTim2(TIM2_FRAME *, sceHiPlugTim2Head_t *, sceHiPlugTex2DHead_t *, sceHiGsMemTbl *);
#if __DEBUG
static void TIM2INFO(u_int *);
#endif


/* programs */
static sceHiErr _hip_tim2_err(_local_err err)
{
    sceHiErrState.mes = _tim2_err_mes[err];
    return _local2higerr[err];
}

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

#if __DEBUG
static void TIM2INFO(u_int *data)
{
    TIM2_FILEHEADER	*pFileHdr = (TIM2_FILEHEADER *)data;
    TIM2_PICTUREHEADER	*pPictHdr;
/*    TIM2_MIPMAPHEADER	*pMipHdr;*/
    sceHiErr	err;

    static char *pict_ImageType[] = {
	"NONE", "RGB16", "RGB24", "RGB32", "IDTEX4", "IDTEX8"
    };

    err = GetTim2PictureHeader(data, 0, &pPictHdr);
    if (err != SCE_HIG_NO_ERR) {
	printf("%s\n", sceHiErrState.mes);
	while (1)
	    ;
    }

    /* File Header Information */
    printf("FILE HEADER ADDR  : %08x\n", pFileHdr);
    printf("-- picture number : %d\n", pFileHdr->Pictures);

    /* Picture Header Information */
    printf("PICT HEADER ADDR  : %08x\n", pPictHdr);
    printf("-- image number   : %d\n", pPictHdr->MipMapTextures);
    printf("-- image type     : %s\n", pict_ImageType[pPictHdr->ImageType]);
    printf("-- image width    : %d\n", pPictHdr->ImageWidth);
    printf("-- image height   : %d\n", pPictHdr->ImageHeight);
    printf("-- clut colors    : %d\n", pPictHdr->ClutColors);
    printf("-- clut type      : %s\n", pict_ImageType[pPictHdr->ClutType & 0x3f]);
    printf("-- clut sort      : %s\n", pPictHdr->ClutType & 0x40 ? "Sorted" : "Not Sorted");
    printf("-- CSM?           : %d\n", pPictHdr->ClutType & 0x80 ? 2 : 1);
}
#endif
static sceHiErr CheckTim2Header(u_int *data)
{
    TIM2_FILEHEADER *pHead = (TIM2_FILEHEADER *)data;

    if (!(pHead->FileId[0] == 'T' && pHead->FileId[1] == 'I' && pHead->FileId[2] == 'M' && pHead->FileId[3] == '2'))
	return _hip_tim2_err(_NOT_TIM2_FILE);
    if (!(pHead->FormatVersion == 0x03 ||
	  (pHead->FormatVersion == 0x04 && (pHead->FormatId == 0x00 || pHead->FormatId == 0x01))))
	return _hip_tim2_err(_VERSION_MISMATCH);
    return SCE_HIG_NO_ERR;
}

/* 
  dataで示された TIM2 DATA にある
  numで指定された番号のPictureヘッダーを得る
  得られたaddressは addrに入る
  num番号のデータがなかったばあい SCE_HIG_INVALID_DATAを返す
  データフォーマット内に不備があった場合 SCE_HIG_FAILUREを返す
*/
static sceHiErr GetTim2PictureHeader(int *data, int num, TIM2_PICTUREHEADER **addr)
{
    TIM2_FILEHEADER *pFileHdr = (TIM2_FILEHEADER *)data;
    TIM2_PICTUREHEADER *pPictHdr;
    sceHiErr err;
    int i;

    if ((err = CheckTim2Header(data)) != SCE_HIG_NO_ERR)
	return err;

    if (num >= pFileHdr->Pictures)
	return _hip_tim2_err(_HUGE_ARGUMENT_VALUE);
    switch (pFileHdr->FormatId) {
      case 0x00: /* 16 byte align */
	pPictHdr = (TIM2_PICTUREHEADER *)((char *)data + sizeof(TIM2_FILEHEADER));
	break;
      case 0x01: /* 128 byte align */
	pPictHdr = (TIM2_PICTUREHEADER *)((char *)data + 0x80);
	break;
      default:
	return _hip_tim2_err(_INVALID_ALIGN_ID);
    }
    for (i = 0; i < num; i++) {
	pPictHdr = (TIM2_PICTUREHEADER *)((char *)pPictHdr + pPictHdr->TotalSize);
    }
    *addr = pPictHdr;
    return SCE_HIG_NO_ERR;
}

/* 
  TIM2 Textureデータ転送用データ作成
  psm : Pixel Format
  tbp : Texture Base Pointer
  tbw : Texture Buffer Width
  w : Texture Width
  h : Texture Height
  x : X of Trans-corrdinate
  y : Y of Trans-coordinate
  pImage : Texure Image Address
*/
static sceHiErr MakeTim2TexturePacket(int psm, u_int tbp, int tbw, int w, int h, int x, int y, u_long128 *pImage)
{
    int	n;
    qword	tag[10];
    int	size;
    sceHiErr	err;

    switch (psm) {
      case SCE_GS_PSMZ32:
      case SCE_GS_PSMCT32:
	n = w * 4;
	size = (w * h) >> 2;
	break;
      case SCE_GS_PSMZ24:
      case SCE_GS_PSMCT24:
	n = w * 3;
	size = (w * h * 3) >> 4;
	break;
      case SCE_GS_PSMZ16:
      case SCE_GS_PSMZ16S:
      case SCE_GS_PSMCT16:
      case SCE_GS_PSMCT16S:
	n = w * 2;
	size = (w * h) >> 3;
	break;
      case SCE_GS_PSMT8H:
      case SCE_GS_PSMT8:
	n = w;
	size = (w * h) >> 4;
	break;
      case SCE_GS_PSMT4:
      case SCE_GS_PSMT4HL:
      case SCE_GS_PSMT4HH:
	size = ((int) w * (int) h) >> 5;
	break;
      default:
	return SCE_HIG_NO_ERR;
    }
    
    SET_QW(tag[0], 0x00000000, 0x0000000e, 0x10000000, 0x00008004);
    SET_QW(tag[1], 0x00000000, 0x00000050,
	   ((psm << (GS_BITBLTBUF_DPSM_O - 32))
	    | (tbw << (GS_BITBLTBUF_DBW_O - 32))
	    | (tbp << (GS_BITBLTBUF_DBP_O - 32))), 0x00000000);
    SET_QW(tag[2], 0x00000000, 0x00000051,
	   (y << (GS_TRXPOS_DSAY_O - 32))
	   | (x << (GS_TRXPOS_DSAX_O - 32)), 0x00000000);
    SET_QW(tag[3], 0x00000000, 0x00000052, h, w);
    SET_QW(tag[4], 0x00000000, 0x00000053, 0x00000000, 0x00000000);

    SET_QW(tag[6], 0x00000000, 0x0000000e, 0x10000000, 0x00008001);
    SET_QW(tag[7], 0x00000000, 0x0000003f, 0x00000000, 0x00000000);

    SET_QW(tag[5], 0x00000000, 0x00000000, 0x08000000, size | 0x8000);

    err = sceHiDMAMake_WaitMicro();
    if (err != SCE_HIG_NO_ERR)
	return err;
    err = sceHiDMAMake_LoadGSLump((u_int *)&(tag[0]), 5);
    if (err != SCE_HIG_NO_ERR)
	return err;
    err = sceHiDMAMake_LoadGSLump((u_int *)&(tag[5]), 1);
    if (err != SCE_HIG_NO_ERR)
	return err;
    err = sceHiDMAGet_BufferPtr(&(_curtf->pbufaddr[_curimg]));
    if (err != SCE_HIG_NO_ERR)
	return err;
    err = sceHiDMAMake_LoadGS((u_int *)pImage, size);
    if (err != SCE_HIG_NO_ERR)
	return err;
    err = sceHiDMAMake_LoadGSLump((u_int *)&(tag[6]), 2);
    if (err != SCE_HIG_NO_ERR)
	return err;
    return SCE_HIG_NO_ERR;
}

/* log2(n)の計算 */
static int getLog2(int n)
{
    int	i;

    for (i = 31; i > 0; i--) {
	if (n & (1 << i)) {
	    break;
	}
    }
    if (n > (1 << i)) {
	i++;
    }
    return i;
}

static void resetTbp(u_int adr)
{
  _init_tbp = adr;
  _next_tbp = _init_tbp;
}

static void resetCbp(u_int adr)
{
    _init_cbp = adr;
    _next_cbp = _init_cbp;
}

static u_int getTbp(size_t size)
{
  u_int tbp;

  tbp = _next_tbp;
  _next_tbp += size;
  if(_next_tbp > _next_cbp){
    _next_tbp = _next_cbp;
  }

  return tbp;
}

static u_int getCbp(size_t size)
{
  u_int cbp;

  _next_cbp -= size;
  if(_next_cbp < _next_tbp){
    _next_cbp = _next_tbp;
  }
  cbp = _next_cbp;

  return cbp;
}

/* 
  TIM2 Imageデータ部分をGSに転送するためのDMA Packetを生成し、
  Shape Pluginが使うTEX_ENVデータを作成する
  TEX_ENVデータは, GiftagとRegisterにセットするデータから成り、
  A+DをつかってGIFTAGに直接転送をする

  また引数で渡されるPicture Headerで示されるPictureデータは
  一つのLV0 Imageを必ず持つことを前提としている

  また、引数envは 該当Picture Dataに対応するENVのアドレスを与えていること。

  ph : Picture Header Address
  tbl : Gs Memory Table (include tbp/cbp)
  env : HiG TEX_ENV DATA address
*/
static sceHiErr DecodeTim2Image(TIM2_PICTUREHEADER *ph, sceHiGsGiftag *env)
{
    int i, psm;
    u_long128	*pImage;
    int	w, h;
    int	tbw, tbp;
    sceHiGsPacked pkd = {NULL, NULL};
    sceGsTex0	*tex0 = NULL;
    sceGsTex1	*tex1 = NULL;
    sceGsMiptbp1 *tbp1 = NULL;
    sceGsMiptbp2 *tbp2 = NULL; 
    size_t	texSize;
    TIM2_MIPMAPHEADER	*pm;
    u_int	saveMMIN;
    sceHiErr	err;

    /* mipmap headerを得ておく(ない場合もあり) */
    pm = (TIM2_MIPMAPHEADER *)(ph + 1);

    /* Imageデータがない */
    if (ph->MipMapTextures < 1)
	return _hip_tim2_err(_NO_IMAGE_DATA);

    /* TEX_ENVのpack addressを取得 */
    pkd.giftag = (sceHiGsGiftag *)env;
    pkd.packed = (sceHiGsPacked_t *)(env + 1);

    /* TEX_ENVのregister設定のアドレスを取得 */
    tex0 = (sceGsTex0 *)getPackedData(&pkd, SCE_GS_TEX0_1);
    tex1 = (sceGsTex1 *)getPackedData(&pkd, SCE_GS_TEX1_1);
    tbp1 = (sceGsMiptbp1 *)getPackedData(&pkd, SCE_GS_MIPTBP1_1);
    tbp2 = (sceGsMiptbp2 *)getPackedData(&pkd, SCE_GS_MIPTBP2_1);

    /* get pixel format */
    psm = psmtbl[ph->ImageType - 1];
    ((sceGsTex0 *)&ph->GsTex0)->PSM = psm;

    /* Image Size */
    w = ph->ImageWidth;
    h = ph->ImageHeight;

    /* calc Image Address */
    pImage = (u_long128 *)((char *)ph + ph->HeaderSize);

    /* tbp/tbwの計算 */
    if (w < 64)
	tbw = 1;
    else if (psm == SCE_GS_PSMT8 || psm == SCE_GS_PSMT4) {
#if 1
	tbw = 2 * ((w + 127) / 128);
#else
	tbw = (w + 63) / 64;
	if (tbw & 1) {
	    tbw++;
	}
#endif
    } else {
	tbw = (w + 63) / 64;
    }
    texSize = sceHiGsPageSize(w, h, psm) / SCE_HIGS_BLOCK_ALIGN;
    tbp = getTbp(texSize);

    /* LV0 Textureの転送 */
    _curimg = 0;	/* current image number is 0 */
    err = MakeTim2TexturePacket(psm, tbp, tbw, w, h, 0, 0, pImage);
    if (err != SCE_HIG_NO_ERR)
	return err;

    /* TEX_ENVへの適用 */
    tex0->TBP0 = tbp;
    tex0->TBW = tbw;
    tex0->PSM = psm;
    tex0->TW = getLog2(w);
    tex0->TH = getLog2(h);

    /* --- Mipmapなし状態にしておく */
    tex1->MXL = 0;
    saveMMIN = tex1->MMIN;
    tex1->MMIN = 1;

    /* LV1以降の転送 */
    for (i = 1; i < ph->MipMapTextures; i++) {
	w = w / 2;
	h = h / 2;
	pImage = (u_long128 *)((char *)pImage + pm->MMImageSize[i - 1]);
	if (psm == SCE_GS_PSMT8 || psm == SCE_GS_PSMT4) {
	    tbw = 2 * ((w + 127) / 128);
	} else {
	    tbw = (w + 63) / 64;
	}
	texSize = sceHiGsPageSize(w, h, psm) / SCE_HIGS_BLOCK_ALIGN;
	tbp = getTbp(texSize);

	/* Load Packet */
	_curimg = i;	/* current img number is i */
	err = MakeTim2TexturePacket(psm, tbp, tbw, w, h, 0, 0, pImage);
	if (err != SCE_HIG_NO_ERR)
	    return err;

	switch (i) {
	  case 1:
	    tbp1->TBP1 = tbp;
	    tbp1->TBW1 = tbw;
	    break;
	  case 2:
	    tbp1->TBP2 = tbp;
	    tbp1->TBW2 = tbw;
	    break;
	  case 3:
	    tbp1->TBP3 = tbp;
	    tbp1->TBW3 = tbw;
	    break;
	  case 4:
	    tbp2->TBP4 = tbp;
	    tbp2->TBW4 = tbw;
	    break;
	  case 5:
	    tbp2->TBP5 = tbp;
	    tbp2->TBW5 = tbw;
	    break;
	  case 6:
	    tbp2->TBP6 = tbp;
	    tbp2->TBW6 = tbw;
	    break;
	}
	tex1->MXL = i;
	tex1->MMIN = 5;
    }

    return SCE_HIG_NO_ERR;
}

/*
  TIM2 CLUTデータ部分をGSに転送するためのDMA Packetを生成し、
  Shape Pluginが使う TEX_ENVデータを作成する
*/
static sceHiErr DecodeTim2Clut(TIM2_PICTUREHEADER *ph, sceHiGsGiftag *env)
{
    int	cpsm;
    u_long128	*pClut;
    int	i, w, h;
    int cbw, cbp;
    size_t	clutSize;
    sceHiErr	err;
    
    sceHiGsPacked	pkd = {NULL, NULL};
    sceGsTex0	*tex0 = NULL;

    if (ph->ClutType == TIM2_NONE) {
	/* Clutが存在していない */
	return SCE_HIG_NO_ERR;
    }

    pClut = (void *)((char *)ph + ph->HeaderSize + ph->ImageSize);

    if ((ph->ClutType & 0x3f) == TIM2_RGB16) {
	cpsm = SCE_GS_PSMCT16;
    } else if ((ph->ClutType & 0x3f) == TIM2_RGB24) {
	cpsm = SCE_GS_PSMCT24;
    } else {
	cpsm = SCE_GS_PSMCT32;
    }

    /* clutのw/h, cbwを得る (CSM1で考える) */
    if (ph->ImageType == TIM2_IDTEX4) {
	w = 8;
	h = ph->ClutColors / w;
    } else if (ph->ImageType == TIM2_IDTEX8) {
	w = 16;
	h = ph->ClutColors / w;
    } else {
	/* なんのフォーマット？ */
	return _hip_tim2_err(_INVALID_CLUT_FORMAT_ID);
    }
    cbw = 1;	/* clutの場合 buffer widthは常に１ */

    /* cbp を計算する */    
    clutSize = sceHiGsBlockSize(w, h, cpsm) / SCE_HIGS_BLOCK_ALIGN;
    cbp = getCbp(clutSize);

    /* CLUTの転送 (CSM1に揃える) */
    switch ((ph->ClutType << 8) | ph->ImageType) {
      case (((TIM2_RGB16 | 0x40) << 8) | TIM2_IDTEX4): /* 16 CSM1 16bit*/
      case (((TIM2_RGB24 | 0x40) << 8) | TIM2_IDTEX4): /* 16 CSM1 24bit*/
      case (((TIM2_RGB32 | 0x40) << 8) | TIM2_IDTEX4): /* 16 CSM1 32bit*/
      case (( TIM2_RGB16         << 8) | TIM2_IDTEX8): /* 256 CSM1 16bit */
      case (( TIM2_RGB24         << 8) | TIM2_IDTEX8): /* 256 CSM1 24bit */
      case (( TIM2_RGB32         << 8) | TIM2_IDTEX8): /* 256 CSM1 32bit */
	/* 並び換え済の物 => そのまま転送可能 */
	_curimg = 7;	/* 7 is clut number */
	err = MakeTim2TexturePacket(cpsm, cbp, 1, 16, (ph->ClutColors / 16), 0, 0, pClut);
	if (err != SCE_HIG_NO_ERR)
	    return err;
	break;
      case (( TIM2_RGB16         << 8) | TIM2_IDTEX4): /* 16 CSM1 16bit */
      case (( TIM2_RGB24         << 8) | TIM2_IDTEX4): /* 16 CSM1 24bit */
      case (( TIM2_RGB32         << 8) | TIM2_IDTEX4): /* 16 CSM1 32bit */
      case (((TIM2_RGB16 | 0x80) << 8) | TIM2_IDTEX4): /* 16 CSM2 16bit */
      case (((TIM2_RGB24 | 0x80) << 8) | TIM2_IDTEX4): /* 16 CSM2 24bit */
      case (((TIM2_RGB32 | 0x80) << 8) | TIM2_IDTEX4): /* 16 CSM2 32bit */
      case (((TIM2_RGB16 | 0x80) << 8) | TIM2_IDTEX8): /* 256 CSM2 16bit */
      case (((TIM2_RGB24 | 0x80) << 8) | TIM2_IDTEX8): /* 256 CSM2 24bit */
      case (((TIM2_RGB32 | 0x80) << 8) | TIM2_IDTEX8): /* 256 CSM2 32bit */
	/* リニア配列(CSM1でかつ入れ換えしていない、CSM2)の場合 */
	/* CSM1に変換してしまう */
	for (i = 0; i < (ph->ClutColors / 16); i++) {
	    _curimg = 7;	/* 7 is clutnumber */
	    err = MakeTim2TexturePacket(cpsm, cbp, 1, 8, 2, (i & 1) * 8, (i >> 1) * 2, pClut);
	    if (err != SCE_HIG_NO_ERR)
		return err;
	    if ((ph->ClutType & 0x3f) == TIM2_RGB16) {
		pClut = (u_long128 *)((char *)pClut + 2 * 16);
	    } else if ((ph->ClutType & 0x3f) == TIM2_RGB24) {
		pClut = (u_long128 *)((char *)pClut + 3 * 16);
	    } else {
		pClut = (u_long128 *)((char *)pClut + 4 * 16);
	    }
	}
	break;
    }

    /* TEX_ENVのpack addressを取得 */
    pkd.giftag = (sceHiGsGiftag *)env;
    pkd.packed = (sceHiGsPacked_t *)(env + 1);

    /* TEX_ENVのregister設定のアドレスを取得 */
    tex0 = (sceGsTex0 *)getPackedData(&pkd, SCE_GS_TEX0_1);

    /* TEX_ENVの設定 */
    tex0->CPSM = cpsm;
    tex0->CBP = cbp;    
    tex0->CSM = 0;	/* 常に CSM1 */
    tex0->CSA = 0;	/* Offset 0 */
    tex0->CLD = 2;

    return SCE_HIG_NO_ERR;
}

/*
  TIM2 Pictureのイメージデータ部分をGSに転送するDMA Packet用データを生成
  tim2 : TIM2 Plugin Frame Structure Address
  data : TIM2_DATA data
  env : TEX2D_ENV data
  tbl : GS Memory Table
*/
static sceHiErr DecodeTim2(TIM2_FRAME *tim2, sceHiPlugTim2Head_t *dataH, sceHiPlugTex2DHead_t *envH, sceHiGsMemTbl *tbl)
{ 
    sceHiErr	err = SCE_HIG_NO_ERR;
    int		i;
    TIM2_PICTUREHEADER	*ph;
    sceHiPlugTim2Data_t	*tim2itm;
    sceHiGsGiftag	*tags;

    _curtf = tim2;

    /* tbp/cbpの初期化 */
    if (tbl != NULL) {
	resetTbp(tbl->addr / SCE_HIGS_BLOCK_ALIGN);
	resetCbp((tbl->addr + tbl->size) / SCE_HIGS_BLOCK_ALIGN);
    } else {
	resetTbp(INIT_TBP);
	resetCbp(INIT_CBP);
    }


    /* packet making */
    err = sceHiDMAMake_ChainStart();
    if (err != SCE_HIG_NO_ERR)
	return err;
    for (i = 0; i < dataH->num; i ++) {
	tim2itm = sceHiPlugTim2GetData(dataH, i);
	if (tim2itm == NULL)	return _hip_tim2_err(_TIM2_DATA_BROKEN);
	if (tim2itm->ptr == NULL) {
	    return _hip_tim2_err(_ADDR_IS_NOT_SET_YET);
	}
	if ((err = GetTim2PictureHeader(tim2itm->ptr, 0, &ph)) != SCE_HIG_NO_ERR)
	    return err;
	tags = sceHiPlugTex2DGetEnv(envH, tim2itm->id);
	if (tags == NULL)	return _hip_tim2_err(_TEXENV_BROKEN);
	if ((err = DecodeTim2Image(ph, tags)) != SCE_HIG_NO_ERR)
	    return err;
	if ((err = DecodeTim2Clut(ph, tags)) != SCE_HIG_NO_ERR)
	    return err;	
    }
    err = sceHiDMAMake_ChainEnd(&(tim2->id));

    return err;
}

sceHiErr sceHiPlugTim2(sceHiPlug *plug, int process)
{
    sceHiPlugTim2Head_t	*dataH;
    sceHiPlugTex2DHead_t	*envH;
    sceHiErr	err;
    TIM2_FRAME	*tim2_frame;
    sceHiGsMemTbl	*tbl = NULL;

    switch (process) {
      case SCE_HIG_INIT_PROCESS:
	dataH = sceHiPlugTim2GetHead(plug, tim2dt);
	envH = sceHiPlugTex2DGetHead(plug, texenvt);

	tim2_frame = (TIM2_FRAME *)sceHiMemAlign(16, sizeof(TIM2_FRAME));
	if (tim2_frame == NULL)
	    return _hip_tim2_err(_NO_HEAP);

	if(plug->args != NULL){
	    tim2_frame->resident = ((sceHiPlugTim2InitArg_t *)plug->args)->resident ? RESIDENT : FALSE;
	    tbl = ((sceHiPlugTim2InitArg_t *)plug->args)->tbl;
	}
	else{
	    tim2_frame->resident = FALSE;
	    tbl = NULL;
	}

	err = DecodeTim2(tim2_frame, dataH, envH, tbl);
	if (err != SCE_HIG_NO_ERR)
	    return err;
	plug->stack = (u_int) tim2_frame;
	plug->args = NULL;
	break;

      case SCE_HIG_PRE_PROCESS:
	break;

      case SCE_HIG_POST_PROCESS:
	tim2_frame = (TIM2_FRAME *)plug->stack;
	if (tim2_frame == NULL)
	    return _hip_tim2_err(_PLGBLK_STACK_BROKEN);

	if(tim2_frame->resident == RESIDENT){
	    err = sceHiDMARegist(tim2_frame->id);
	    if (err != SCE_HIG_NO_ERR)
		return err;
	    tim2_frame->resident = TRUE;
	}
	else if(tim2_frame->resident == FALSE){
	    err = sceHiDMARegist(tim2_frame->id);
	    if (err != SCE_HIG_NO_ERR)
		return err;
	}

	break;

      case SCE_HIG_END_PROCESS:
	tim2_frame = (TIM2_FRAME *)plug->stack;
	if (tim2_frame == NULL)
	    return _hip_tim2_err(_PLGBLK_STACK_BROKEN);
	err = sceHiDMADel_Chain(tim2_frame->id);
	if (err != SCE_HIG_NO_ERR)
	    return err;
	sceHiMemFree((u_int *)Paddr(tim2_frame));
	plug->stack = NULL;
	plug->args = NULL;
	break;
    }
    return SCE_HIG_NO_ERR;
}

int sceHiPlugTim2Num(sceHiPlug *plug)
{
    sceHiPlugTim2Head_t	*dataH;

    dataH = sceHiPlugTim2GetHead(plug, tim2dt);

    if (dataH == NULL)
	return -1;
    return dataH->num;
}

char *sceHiPlugTim2GetName(sceHiPlug *plug, int idx)
{
    sceHiPlugTim2Head_t	*dataH;
    sceHiPlugTim2Data_t	*data;

    dataH = sceHiPlugTim2GetHead(plug, tim2dt);
    if (dataH == NULL)
	return NULL;
    data = sceHiPlugTim2GetData(dataH, idx);
    if (data == NULL)
	return NULL;
    /* file nameの文字は16文字(4word)まで */

    return (char *)data->fname;
}

sceHiErr sceHiPlugTim2SetData(sceHiPlug *plug, int idx, u_int *fdata)
{
    sceHiPlugTim2Head_t	*dataH;
    sceHiPlugTim2Data_t	*data;

    dataH = sceHiPlugTim2GetHead(plug, tim2dt);
    data = sceHiPlugTim2GetData(dataH, idx);
    data->ptr = fdata;

    return SCE_HIG_NO_ERR;
}

sceHiErr sceHiPlugTim2GetNPictures(sceHiPlug *plug, int n, int *num)
{
    TIM2_FILEHEADER	*pfh;
    sceHiPlugTim2Head_t	*dataH;
    sceHiPlugTim2Data_t	*data;

    dataH = sceHiPlugTim2GetHead(plug, tim2dt);
    data = sceHiPlugTim2GetData(dataH, n);
    pfh = (TIM2_FILEHEADER *)data->ptr;
    if (pfh == NULL)
	return _hip_tim2_err(_ADDR_IS_NOT_SET_YET);
    *num = pfh->Pictures;
    return SCE_HIG_NO_ERR;
}

static int _calc_image_size(int psm, int w, int h)
{
    int size;

    switch (psm) {
      case SCE_GS_PSMZ32:
      case SCE_GS_PSMCT32:
	size = (w * h) >> 2;
	break;
      case SCE_GS_PSMZ24:
      case SCE_GS_PSMCT24:
	size = (w * h * 3) >> 4;
	break;
      case SCE_GS_PSMZ16:
      case SCE_GS_PSMZ16S:
      case SCE_GS_PSMCT16:
      case SCE_GS_PSMCT16S:
	size = (w * h) >> 3;
	break;
      case SCE_GS_PSMT8H:
      case SCE_GS_PSMT8:
	size = (w * h) >> 4;
	break;
      case SCE_GS_PSMT4:
      case SCE_GS_PSMT4HL:
      case SCE_GS_PSMT4HH:
	size = ((int) w * (int) h) >> 5;
	break;
      default:
	size = 0;
	break;
    }
    return size;
}

sceHiErr sceHiPlugTim2SetPicture(sceHiPlug *plug, int n, int num)
{
    TIM2_FRAME	*tf;
    u_int	*save;		/* the dma buffer pointer that saved */
    u_long128	*pImage;	/* picture image address */
    u_long128	*pClut;		/* clut image address */
    sceHiPlugTim2Head_t	*dataH;
    sceHiPlugTim2Data_t	*data;
    TIM2_PICTUREHEADER	*ph;
    TIM2_MIPMAPHEADER	*pm;
    sceHiErr	err;
    int	w, h, i, psm;

    /* get TIM2_FRAME */
    tf = (TIM2_FRAME *)plug->stack;
    if (tf == NULL)
	return _hip_tim2_err(_PLGBLK_STACK_BROKEN);

    /* get TIM2 Data Address */
    dataH = sceHiPlugTim2GetHead(plug, tim2dt);
    data = sceHiPlugTim2GetData(dataH, n);

    /* re-set image/clut address */
    err = sceHiDMAGet_BufferPtr(&save); 
    if (err != SCE_HIG_NO_ERR)
	return err;
    {
	/* picture */
	err = GetTim2PictureHeader(data->ptr, num, &ph);
	if (err != SCE_HIG_NO_ERR)
	    return err;
	pm = (TIM2_MIPMAPHEADER *)(ph + 1);
	pImage = (u_long128 *)((char *)ph + ph->HeaderSize);
	psm = psmtbl[ph->ImageType - 1];
	w = ph->ImageWidth;
	h = ph->ImageHeight;
	{
#if __DEBUG	    
	    printf("IMAGE ADDRESS : 0x%08x\n", (u_int)pImage);
#endif
	    err = sceHiDMASet_BufferPtr(tf->pbufaddr[0]);
	    if (err != SCE_HIG_NO_ERR)
		return err;
	    err = sceHiDMAMake_LoadGS((u_int *)pImage, _calc_image_size(psm, w, h));
	    if (err != SCE_HIG_NO_ERR)
		return err;
	}
	for (i = 1; i < ph->MipMapTextures; i++) {
	    w = w / 2;
	    h = h / 2;
	    pImage = (u_long128 *)((char *)pImage + pm->MMImageSize[i - 1]);
	    {
#if __DEBUG
		printf("  MIPMAP [%d] : 0x%08x\n", (u_int)pImage);
#endif
		err = sceHiDMASet_BufferPtr(tf->pbufaddr[i]);
		if (err != SCE_HIG_NO_ERR)
		    return err;
		err = sceHiDMAMake_LoadGS((u_int *)pImage, _calc_image_size(psm, w, h));
		if (err != SCE_HIG_NO_ERR)
		    return err;
	    }
	}

	/* clut */
	if (ph->ClutType != TIM2_NONE) {
	    pClut = (void *)((char *)ph + ph->HeaderSize + ph->ImageSize);
	    if ((ph->ClutType & 0x3f) == TIM2_RGB16) {
		psm = SCE_GS_PSMCT16;
	    } else if ((ph->ClutType & 0x3f) == TIM2_RGB24) {
		psm = SCE_GS_PSMCT24;
	    } else {
		psm = SCE_GS_PSMCT32;
	    }
	    if (ph->ImageType == TIM2_IDTEX4) {
		w = 8;
		h = ph->ClutColors / w;
	    } else if (ph->ImageType == TIM2_IDTEX8) {
		w = 16;
		h = ph->ClutColors / w;
	    } else {
		return _hip_tim2_err(_INVALID_CLUT_FORMAT_ID);
	    }
	    switch ((ph->ClutType << 8) | ph->ImageType) {
	      case (((TIM2_RGB16 | 0x40) << 8) | TIM2_IDTEX4): /* 16 CSM1 16bit*/
	      case (((TIM2_RGB24 | 0x40) << 8) | TIM2_IDTEX4): /* 16 CSM1 24bit*/
	      case (((TIM2_RGB32 | 0x40) << 8) | TIM2_IDTEX4): /* 16 CSM1 32bit*/
	      case (( TIM2_RGB16         << 8) | TIM2_IDTEX8): /* 256 CSM1 16bit */
	      case (( TIM2_RGB24         << 8) | TIM2_IDTEX8): /* 256 CSM1 24bit */
	      case (( TIM2_RGB32         << 8) | TIM2_IDTEX8): /* 256 CSM1 32bit */
		{
		    err = sceHiDMASet_BufferPtr(tf->pbufaddr[7]);
		    if (err != SCE_HIG_NO_ERR)
			return err;
		    err = sceHiDMAMake_LoadGS((u_int *)pClut, _calc_image_size(psm, w, h));
		    if (err != SCE_HIG_NO_ERR)
			return err;
		}
		break;
	      case (( TIM2_RGB16         << 8) | TIM2_IDTEX4): /* 16 CSM1 16bit */
	      case (( TIM2_RGB24         << 8) | TIM2_IDTEX4): /* 16 CSM1 24bit */
	      case (( TIM2_RGB32         << 8) | TIM2_IDTEX4): /* 16 CSM1 32bit */
	      case (((TIM2_RGB16 | 0x80) << 8) | TIM2_IDTEX4): /* 16 CSM2 16bit */
	      case (((TIM2_RGB24 | 0x80) << 8) | TIM2_IDTEX4): /* 16 CSM2 24bit */
	      case (((TIM2_RGB32 | 0x80) << 8) | TIM2_IDTEX4): /* 16 CSM2 32bit */
	      case (((TIM2_RGB16 | 0x80) << 8) | TIM2_IDTEX8): /* 256 CSM2 16bit */
	      case (((TIM2_RGB24 | 0x80) << 8) | TIM2_IDTEX8): /* 256 CSM2 24bit */
	      case (((TIM2_RGB32 | 0x80) << 8) | TIM2_IDTEX8): /* 256 CSM2 32bit */
		for (i = 0; i < (ph->ClutColors / 16); i++) {
		    {
			err = sceHiDMASet_BufferPtr(tf->pbufaddr[7]);
			if (err != SCE_HIG_NO_ERR)
			    return err;
			err = sceHiDMAMake_LoadGS((u_int *)pClut, _calc_image_size(psm, w, h));
			if (err != SCE_HIG_NO_ERR)
			    return err;
		    }
		    if ((ph->ClutType & 0x3f) == TIM2_RGB16) {
			pClut = (u_long128 *)((char *)pClut + 2 * 16);
		    } else if ((ph->ClutType & 0x3f) == TIM2_RGB24) {
			pClut = (u_long128 *)((char *)pClut + 3 * 16);
		    } else {
			pClut = (u_long128 *)((char *)pClut + 4 * 16);
		    }
		}
		break;
	    }
	}
    }
    err = sceHiDMASet_BufferPtr(save);
    if (err != SCE_HIG_NO_ERR)
	return err;

    return SCE_HIG_NO_ERR;
}

