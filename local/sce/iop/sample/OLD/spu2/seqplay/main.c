/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library  Release 1.6
 */
/* 
 *              I/O Proseccor Library Sample Program
 * 
 *                         - seqplay -
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


#include <sys/types.h>
#include <sys/file.h>
#include <libsnd2.h>
#include <libspu2.h>
#include <stdio.h>
#include <kernel.h>
#include <timerman.h>

/* tickmode: currentry only NOTICK mode available... */
#define NOTICK			1

//#define PARTLY /* SsVabTransBodyPartly() */

#define  SEQ_FILENAME	"host1:/usr/local/sce/data/sound/seq/fuga.seq"
#define  VH_FILENAME	"host1:/usr/local/sce/data/sound/wave/simple.vh"
#define  VB_FILENAME	"host1:/usr/local/sce/data/sound/wave/simple.vb"
#define  ALLOC_IOP_HEAP_SIZE   0x100000
#define  VH_ADDR_OFST	0x10000
#define  VB_ADDR_OFST	0x20000
#define MVOL		127			/* main volume */
#define SVOL		127			/* seq data volume */

#define PRINTF(x) printf x

#ifdef PARTLY
#define BUFSIZE 0x800
unsigned char buf [BUFSIZE];
#endif /* PARTLY */

volatile int gTimerID;


int IntFunc( int* common )
{
	int c, m ;
	m = GetTimerStatus(gTimerID);
	if ( m & tEQUF_1 ) {
		*common = 1;
		c = GetTimerCounter(gTimerID);
// 		printf(" -- timer %4d %x %x\n", c, m);
	}
	return 1;
}


void setTimer( volatile int* common )
{
	// --- use H-Sync timer
	gTimerID = AllocHardTimer(TC_HLINE , 16, 1);
	RegisterIntrHandler(GetHardTimerIntrCode(gTimerID),
			HTYPE_C, (void*)IntFunc, (void*)common);
	SetTimerCompare(gTimerID, 66);  /* 1/240 tick */
	SetTimerMode( gTimerID , tEXTC_1 | tGATF_0 | tZRET_1 | tREPT_1 | tCMP_1);
	EnableIntr(GetHardTimerIntrCode(gTimerID));
}

	char seq_table[SS_SEQ_TABSIZ * 4 * 5]; /* seq data table */


char* dataset(void)
{
	int fd, size;
	char *buffer;

	PRINTF(("allocate IOP heap memory - " ));
	buffer = AllocSysMemory(0, ALLOC_IOP_HEAP_SIZE, NULL);
	if( buffer == NULL ) { printf( "\nCan't alloc heap \n"); return NULL; }
	PRINTF(("alloced 0x%x  ", (int)buffer));

	fd = open(SEQ_FILENAME, O_RDONLY);
 	size = lseek(fd, 0, 2);
	if( size <= 0 ) { printf( "\nCan't load seq file to iop heap \n" ); return NULL; }
	lseek( fd, 0, 0);
	read( fd, buffer, size);
	close(fd);

	fd = open(VH_FILENAME, O_RDONLY);
 	size = lseek(fd, 0, 2);
	if( size <= 0 ) { printf( "\nCan't load vh file to iop heap \n" ); return NULL; }
	lseek( fd, 0, 0);
	read(fd, buffer+VH_ADDR_OFST, size);
	close(fd);

	fd = open(VB_FILENAME, O_RDONLY);
 	size = lseek(fd, 0, 2);
	if( size <= 0 ) { printf( "\nCan't load vb file to iop heap \n" ); return NULL; }
	lseek( fd, 0, 0);
	read(fd, buffer+VB_ADDR_OFST, size);
	close(fd);

	return buffer;
}


int start(int argc, char **argv) 
{
	short vab;	/* vab data id */
	short seq;	/* seq data id */
	volatile int common;
	char *buffer;

#ifdef PARTLY
	short vab_;			/* work */
	unsigned long top;
#endif /* PARTLY */

	if( (buffer = dataset()) == NULL ){ return -1; }


	PRINTF(( "\nseqplay start...\n" ));
	SsInit();			/* reset sound */

	CpuEnableIntr();
	EnableIntr( INUM_DMA_4 );

	// --- It is nesessary to set CORE-1 volume even using CORE-0 voice.
	SpuSetCore(1);
	SpuSetReverbModeDepth( 0x0, 0x0 );
	SsSetMVol(MVOL, MVOL);	/* set main volume */
	SpuSetCore(0);
	SpuSetReverbModeDepth( 0x0, 0x0 );
	SsSetMVol(MVOL, MVOL);	/* set main volume */


	SsSetTableSize (seq_table, 4, 5); /* keep seq data table area */
#ifdef NOTICK
	SsSetTickMode(SS_NOTICK | 240 );	/* set tick mode = NOTICK */
#else  /* NOTICK */
	SsSetTickMode(SS_TICK240);		/* set tick mode = TICK240 */
#endif /* NOTICK */

	vab = SsVabOpenHead (buffer+VH_ADDR_OFST, -1);
	if (vab == -1) {
		printf ("SsVabOpenHead : failed !!!\n");
		return 1;
	}

#ifdef PARTLY
	top = 0;
	memcpy (buf, (unsigned char *)&(buffer+VB_ADDR_OFST)  [top], BUFSIZE);
	while ((vab_ = SsVabTransBodyPartly (buf, BUFSIZE, vab)) != vab) {
	switch (vab_) {
	case -2:		/* Continue */
	    SsVabTransCompleted (SS_WAIT_COMPLETED);
	    top += BUFSIZE;
	    memcpy (buf, (unsigned char *) &(buffer+VB_ADDR_OFST) [top], BUFSIZE);
	    break;
	case -1:		/* Failed */
	    printf ("SsVabTransBodyPartly : failed !!!\n");
	    return 1;
	    break;
	default:
	    break;
	}
    }
#else /* PARTLY */
	if (SsVabTransBody (buffer+VB_ADDR_OFST, vab) != vab) {
		printf ("SsVabTransBody : failed !!!\n");
	return 1;
	}
#endif /* PARTLY */

	SsVabTransCompleted (SS_WAIT_COMPLETED);
	PRINTF(("SsVabTransCompleted : startaddr : %x\n", (unsigned int)SpuGetTransferStartAddr() ));

	seq = SsSeqOpen((u_long *)(buffer), vab); /* open seq data */
	SsStart();
	SsSeqSetVol(seq, SVOL, SVOL); /* set seq data volume */

	setTimer(&common);

	SsSeqPlay(seq, SSPLAY_PLAY, 1);

	while ( SsIsEos( seq, 0) ) {
#ifdef NOTICK
		while( common == 0 ){};
 		common = 0;
		SsSeqCalledTbyT();
#endif /* NOTICK */
	}

	SsSeqClose(seq);		/* close seq data */
	SsVabClose(vab);		/* close vab data */
	SsEnd();			/* sound system end */
	PRINTF(( "seqplay completed...\n" ));
	SsQuit();
	return 0;
}
