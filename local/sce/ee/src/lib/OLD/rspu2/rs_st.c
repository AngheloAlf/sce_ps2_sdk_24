/* SCE CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *                      Emotion Engine Library
 *                          Version 0.50
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                        librspu2 - rs_st.c
 *                        streaming functions
 *
 *     Version    Date          Design     Log
 *  --------------------------------------------------------------------
 *     0.30       Jun.17.1999   morita     provisional version.
 *     0.50       Aug.18.1999   morita     rewrite for new siflib.
 */


#include <eekernel.h>
#include <sifcmd.h>
#include <sif.h>
#include <librspu2.h>
#include <rspu2cmd.h>
#include <stdarg.h>
#include "rs_i.h"

void* gRpcStDataAddr;  //--- stream���\���̂�]������EE���̃A�h���X
static sceSifDmaData transData;


/* ------------------------------------------------------------------------
   stream���\���̂�IOP���ɓ]�����邽�߂̏������s�Ȃ��B
   ------------------------------------------------------------------------*/
int sceSpu2StreamEnvInit( SpuStEnv *st )
{
	SpuStVoiceAttr *v;
	int i;

	gRpcStDataAddr = (void*)sceSpu2Remote( 1, rSpuStInit, 0);
	for (i = 0; i < 24; i ++) {
		v = &(st->voice [i]);
		v->status      = SPU_ST_PLAY;
		v->last_size   = 0;
		v->buf_addr    = 0;
		v->data_addr   = 0;
	}
 	st->size = 0;
 	st->low_priority = SPU_OFF;

	PRINTF(( "gRpcStDataAddr = %x\n", gRpcStDataAddr ));
	return 0;
}

/* ------------------------------------------------------------------------
   stream���\���̂�IOP���ɓ]������B
   *st ��un-cache�̈�ł���K�v������B
   ------------------------------------------------------------------------*/
int sceSpu2StreamEnvSet( SpuStEnv *st )
{
	unsigned int did;
	int i;

	transData.data = (u_int)(st);
	transData.addr = (u_int)gRpcStDataAddr;
	transData.size = sizeof(SpuStEnv);
	transData.mode = 0;

	did = sceSifSetDma( &transData, 1 );
	while( sceSifDmaStat( did ) >= 0 ){}
	PRINTF(("send SpuStEnv completed \n"));
	if( did == 0 ) return -1;
	return 0;
}

