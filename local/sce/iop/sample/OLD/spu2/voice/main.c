/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *              I/O Processor Library Sample Program
 * 
 *                         - voice -
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
 */

#include <stdio.h>
#include <kernel.h>
#include <libspu2.h>
#include <sys/file.h>

#define DMA_CB_TEST	0
#define IRQ_CB_TEST	0
#define IRQ_ADDR_OFST	0x1000

#define PRINTF(x) printf x

#define VAG_FILENAME	"host1:/usr/local/sce/data/sound/wave/piano.vag"
#define VAG_ADDR	0x5010
#define	REVERB_DEPTH 	0x1fff

char gDoremi[8] = { 36, 38, 40, 41, 43, 45, 47, 48}; 

int soundTest ( void );

int gEndFlag = 0;


void wait_loop( void )
{
	volatile long i,j = 13;

	for ( i = 0; i < 1000 ; i++ ){
		j = j*3;
	}
}


int IntFunc( void* common )
{
	gEndFlag = 1;
	printf("///// interrupt detected. /////\n");
	return 1;
}

char* dataset(int *size)
{
	int fd;
	char *buffer;

	fd = open(VAG_FILENAME, O_RDONLY);
 	*size = lseek(fd, 0, 2);
	if( *size <= 0 ) { printf( "\nCan't load VAG file to iop heap \n" ); return NULL; }
	lseek( fd, 0, 0);

	PRINTF(("allocate IOP heap memory - " ));
	buffer = AllocSysMemory(0, *size, NULL);
	if( buffer == NULL ) { printf( "\nCan't alloc heap \n"); return NULL; }
	PRINTF(("alloced 0x%x  \n", (int)buffer));

	read( fd, buffer, *size);
	close(fd);
	return buffer;
}


int start(int argc, char *argv[])
{
	soundTest();
	return 0;
}


int soundTest(void)
{
	int i, j;
	SpuVoiceAttr s_attr;
	SpuReverbAttr r_attr;
	SpuCommonAttr c_attr;
	char *buffer;
	int size;

	PRINTF(( "voice start...\n" ));

	if( (buffer = dataset(&size)) == NULL ){ return -1; }

	SpuInit();

	CpuEnableIntr();
	EnableIntr( INUM_DMA_4 );
	EnableIntr( 9 );

	
	for( i = 0; i < 2; i++ )
	{
		SpuSetCore(i);
		// --- set commonn attribute
		c_attr.mask = (SPU_COMMON_MVOLL |  SPU_COMMON_MVOLR);
		c_attr.mvol.left  = 0x3fff;	/* Master volume (left) */
		c_attr.mvol.right = 0x3fff;	/* Master volume (right) */
		SpuSetCommonAttr (&c_attr);

		//--- reverb end address‚ð core0‚Æcore1‚Æ‚Å•Ê‚Ì—Ìˆæ‚ÉÝ’è‚·‚é
		SpuSetReverbEndAddr( 0x1fffff - 0x20000*i );
		SpuSetReverbModeDepth( 0, 0 );
	}

	// --- data transfer
	SpuSetTransferMode( SPU_TRANSFER_BY_DMA );
#if DMA_CB_TEST
	SpuSetTransferCallback((void*)IntFunc);
#endif
	SpuSetTransferStartAddr(VAG_ADDR);
	SpuWrite( (unsigned char*)(buffer+64), (unsigned long)size );
#if DMA_CB_TEST
	while( gEndFlag == 0 ){}
#else
	SpuIsTransferCompleted(SPU_TRANSFER_WAIT );
#endif


#if IRQ_CB_TEST
	SpuSetCore(0);
	SpuSetIRQAddr( VAG_ADDR + IRQ_ADDR_OFST  );
	SpuSetIRQCallback((void*)IntFunc);
	SpuSetIRQ(SPU_ON);
#endif


	for( i = 0; i < 2; i++ )
	{
	SpuSetCore(i);
	// --- set voice attribute
	s_attr.mask = (SPU_VOICE_VOLL |
		   SPU_VOICE_VOLR |
		   SPU_VOICE_SAMPLE_NOTE |
		   SPU_VOICE_NOTE |
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
	s_attr.voice = SPU_ALLCH;
	s_attr.volume.left  = 0x1eff;		/* Left volume */
	s_attr.volume.right = 0x1fff;		/* Right volume */
	s_attr.note         = (38<<8);		/* Pitch */
	s_attr.sample_note  = (60<<8);		/* Sample Note */
	s_attr.addr         = (unsigned long)VAG_ADDR; /* Waveform data  */
	s_attr.a_mode       = SPU_VOICE_EXPIncN;	/* Attack curve */
	s_attr.s_mode       = SPU_VOICE_EXPDecN;	/* Sustain curve */
	s_attr.r_mode       = SPU_VOICE_EXPDecN;	/* Release curve */
	s_attr.ar           = 30;		/* Attack rate value */
	s_attr.dr           = 14;		/* Decay rate value */
	s_attr.sl           = 14;		/* Sustain level value */
	s_attr.sr           = 52;		/* Sustain rate value */
	s_attr.rr           = 13;		/* Release rate value */
	SpuSetVoiceAttr (&s_attr);

	// --- set reverb attribute
	r_attr.mask = ( SPU_REV_DEPTHL | SPU_REV_DEPTHR | SPU_REV_MODE );
	r_attr.depth.left = 0;
	r_attr.depth.right = 0;
	r_attr.mode = SPU_REV_MODE_HALL | SPU_REV_MODE_CLEAR_WA;
	SpuSetReverbModeParam( &r_attr );
	}

	SpuSetCore(0);
	// --- reverb on
	SpuSetReverb( SPU_ON );
	SpuSetReverbModeDepth( REVERB_DEPTH, REVERB_DEPTH );
	SpuSetReverbVoice( SPU_ON, SPU_ALLCH );


	// --- Ring!
	for( j = 0; j < 8; j++ )
	{
#if IRQ_CB_TEST
		SpuSetIRQ(SPU_RESET);
#endif
		s_attr.note         = (gDoremi[j]<<8);
		s_attr.voice = SPU_VOICECH(j);
		s_attr.mask = (SPU_VOICE_NOTE );
		SpuSetVoiceAttr (&s_attr);
		SpuSetKey (SpuOn, SPU_VOICECH(j));

		PRINTF(( " ++ key_on  note : %d \n", s_attr.note>>8 ));
		for( i = 0; i < 3000; i++ )
		{
			wait_loop(); 
		}
	}

	SpuSetKey (SpuOff, SPU_ALLCH);
	PRINTF(( "voice completed...\n" ));
	return 0;
}

