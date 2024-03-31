/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *              I/O Proseccor Library Sample Program
 * 
 *                         - stream -
 * 
 *                           Shift-JIS
 * 
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 * 
 *                            main.c
 * 
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.40            Jul,21,1999     morita    provisional version
 *      0.50            Aug,15,1999     morita    alloc/open/read
 *                                                add SpuStSetCore
 */


#include <stdio.h>
#include <kernel.h>
#include <sys/types.h>
#include <libspu2.h>
#include <sys/file.h>

#define PRINTF(x) (void) printf x

#define SPU_BUFSIZE     0x4000
#define SPU_BUFSIZEHALF 0x2000
#define DATA_SIZE	(long)0x7a000 /* 499712 */
#define TOP_ADDR	(long)0x00D0000
#define SPU_MEMORY_TOP	0x5010
#define SPU_MEMORY_MAX	(2*1024*1024)
#define L0      	0
#define R0      	1
#define VOICE_LIMIT 	2
#define SPU_MALLOC_MAX	VOICE_LIMIT
#define VAG_FILENAME_L	"host1:/usr/local/sce/data/sound/wave/tr1l_pad.vb"
#define VAG_FILENAME_R	"host1:/usr/local/sce/data/sound/wave/tr1r_pad.vb"


volatile int gEnd = 1;
unsigned long played_size;	/* the size finished to set */
SpuStEnv *st;
char spu_malloc_rec [SPU_MALLOC_RECSIZ * (SPU_MALLOC_MAX + 1)];
char* gVagTop;


int dataset(void)
{
	int fd;

	PRINTF(("allocate IOP heap memory - " ));
	gVagTop = AllocSysMemory(0, DATA_SIZE*2, NULL);
	if( gVagTop == NULL ) { printf( "\nCan't alloc heap \n"); return -1; }
	PRINTF(("alloced 0x%x  ", (int)gVagTop));

	fd = open(VAG_FILENAME_L, O_RDONLY);
	read( fd, gVagTop, DATA_SIZE);
	close(fd);

	fd = open(VAG_FILENAME_R, O_RDONLY);
	read( fd, gVagTop+DATA_SIZE, DATA_SIZE);
	close(fd);

	return 0;
}



SpuStCallbackProc
spustCB_transfer_finished (unsigned long voice_bit)
{
	long i;
	static int loop = 0;


	if (played_size <= (DATA_SIZE - SPU_BUFSIZEHALF)) {
		played_size += SPU_BUFSIZEHALF;
		for (i = 0; i < VOICE_LIMIT; i ++) {
			st->voice [i].data_addr += SPU_BUFSIZEHALF;
		}
	} 
	else {
		/* return to TOP */
		for (i = 0; i < VOICE_LIMIT; i ++) {
			st->voice [i].data_addr = (u_int)gVagTop+DATA_SIZE*i;
		}
		played_size = SPU_BUFSIZEHALF;
		loop++;
	}

	if( loop == 2 ){
		for (i = 0; i < VOICE_LIMIT; i ++) {
		    st->voice [i].status    = SPU_ST_STOP;
		    st->voice [i].last_size = SPU_BUFSIZEHALF;
		}

	}

	st->size = SPU_BUFSIZE;
	return 0;
}


SpuStCallbackProc
spustCB_preparation_finished (unsigned long voice_bit, long p_status)
{
	if (p_status == SPU_ST_PREPARE) {
		spustCB_transfer_finished (voice_bit);
	}

	SpuStTransfer (SPU_ST_START, voice_bit); // 本格的にスタート！

	return 0;
}


SpuStCallbackProc
spustCB_stream_finished (unsigned long voice_bit, long s_status)
{
	SpuSetKey (SPU_OFF, voice_bit);
	gEnd = 0;
	return 0;
}


int start()
{
	int	i;
	SpuVoiceAttr s_attr;
	SpuCommonAttr c_attr;
	unsigned long buffer_addr [SPU_MALLOC_MAX];


	PRINTF(("\nstream start...\n" ));

	if( dataset() < 0 ){ return -1; }

	// --- SPU Initialize
	SpuInit ();
	CpuEnableIntr();
	EnableIntr( INUM_DMA_4 );

	SpuSetCore( 0 );
	SpuSetTransferMode( SPU_TRANSFER_BY_DMA );
	SpuSetTransferStartAddr( SPU_MEMORY_TOP );
	SpuWrite0( SPU_MEMORY_MAX - SPU_MEMORY_TOP );

	SpuInitMalloc (SPU_MALLOC_MAX, spu_malloc_rec);
	
	// --- 	SPU Common attributes
	c_attr.mask = (SPU_COMMON_MVOLL |
		       SPU_COMMON_MVOLR);


	for( i = 0; i < 2; i++ ){
	SpuSetCore(i);
	SpuSetKey (SPU_OFF, SPU_ALLCH);

	c_attr.mvol.left  = 0x3fff;	/* Master volume (left) */
	c_attr.mvol.right = 0x3fff;	/* Master volume (right) */
	SpuSetCommonAttr (&c_attr);
	}

	SpuSetCore(0);
	SpuStSetCore(0); //--- ストリーミングで使用するコアを指定

	 // --- SPU streaming setting
	st = SpuStInit (0);
	/* For finishing SPU streaming preparation */
	(void) SpuStSetPreparationFinishedCallback ((SpuStCallbackProc)
				spustCB_preparation_finished);
	/* For next transferring */
	(void) SpuStSetTransferFinishedCallback ((SpuStCallbackProc)
				spustCB_transfer_finished);
	/* For finising SPU streaming with some voices */
	(void) SpuStSetStreamFinishedCallback ((SpuStCallbackProc)
				spustCB_stream_finished);

	/* 		Allocate buffers in sound buffer                    */

	/* for SPU streaming itself */
	for (i = 0; i < VOICE_LIMIT; i ++) {
	    if ((buffer_addr [i] = SpuMalloc (SPU_BUFSIZE)) == -1) {
		return 0;		/* ERROR */
	    }
	}

	/* Set SPU streaming environment */

	/* Size of each buffer in sound buffer */
	st->size = SPU_BUFSIZE;

	/* Top address of each buffer in sound buffer */
	for (i = 0; i < VOICE_LIMIT + 1; i ++) {
	    st->voice [i].buf_addr = buffer_addr [i];
	}
	/* </SPU-STREAMING> */

	// --- Voice attributes
	s_attr.mask = (SPU_VOICE_VOLL |
		       SPU_VOICE_VOLR |
		       SPU_VOICE_PITCH |
		       SPU_VOICE_WDSA |
		       SPU_VOICE_ADSR_AMODE |
		       SPU_VOICE_ADSR_SMODE |
		       SPU_VOICE_ADSR_RMODE |
		       SPU_VOICE_ADSR_AR |
		       SPU_VOICE_ADSR_DR |
		       SPU_VOICE_ADSR_SR |
		       SPU_VOICE_ADSR_RR |
		       SPU_VOICE_ADSR_SL
		       );
	
	/* attribute values: L-ch */
	s_attr.volume.left  = 0x3fff;
	s_attr.volume.right = 0x0;
	s_attr.pitch        = 0x1200;
	s_attr.a_mode       = SPU_VOICE_LINEARIncN;
	s_attr.s_mode       = SPU_VOICE_LINEARIncN;
	s_attr.r_mode       = SPU_VOICE_LINEARDecN;
	s_attr.ar           = 0x0;
	s_attr.dr           = 0x0;
	s_attr.sr           = 0x0;
	s_attr.rr           = 0x3;
	s_attr.sl           = 0xf;
	
	for (i = L0; i < VOICE_LIMIT; i += 2) {
	    s_attr.voice = SPU_VOICECH (i);
	    s_attr.addr  = buffer_addr [i];
	    SpuSetVoiceAttr (&s_attr);
	}
	
	/* attribute values: R-ch */
	s_attr.volume.left  = 0x0;
	s_attr.volume.right = 0x3fff;
	for (i = R0; i < VOICE_LIMIT; i += 2) {
	    s_attr.voice = SPU_VOICECH (i);
	    s_attr.addr  = buffer_addr [i];
	    SpuSetVoiceAttr (&s_attr);
	}
	

	// --- start transfer
	for (i = 0; i < VOICE_LIMIT; i ++) {
		st->voice [i].data_addr = (u_int)gVagTop+DATA_SIZE*i;
		st->voice [i].status    = SPU_ST_PLAY;
	}
	played_size = SPU_BUFSIZEHALF;
	SpuStTransfer (SPU_ST_PREPARE, SPU_VOICECH (L0) | SPU_VOICECH (R0));

	// --- main loop
	while(gEnd){}

	// --- clean up
	SpuStQuit ();
	SpuQuit ();
	PRINTF(("stream completed...\n" ));

	return 0;
}


/* ----------------------------------------------------------------
 *	End on File
 * ---------------------------------------------------------------- */
/* DON'T ADD STUFF AFTER THIS */
