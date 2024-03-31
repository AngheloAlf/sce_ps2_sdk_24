/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
*/
/*
 *                   Library Sample Program
 *
 *                         - CD/DVD -
 *
 *	Wavfile format -> PS2 PCM raw-format convertor
 *	Wavfile(Sampling Rate 48KHz stereo 16-bit integer little endian)
 *
 *      Copyright (C) 2000 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *		   make: gcc -o wav2int wav2int.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#define NORMAL 1
#define ABNORM 0

char mfna[64];
char kfna[64];
unsigned char srcbuf[1024];
unsigned char dstbuf[1024];

// wav format ------------------------
#define RIFF_HEADER_SIZE 44
typedef struct {
        unsigned char     chunkID[4];
        unsigned int      chunkSize;
        unsigned short*   data;
} DATAHeader;

typedef struct {
        unsigned char     chunkID[4];
        unsigned int      chunkSize;
        unsigned short    waveFmtType;
        unsigned short    channel;
        unsigned int      samplesPerSec;
        unsigned int      bytesPerSec;
        unsigned short    blockSize;
        unsigned short    bitsPerSample;
} FMTHeader;

typedef struct {
        unsigned char     chunkID[4];
        unsigned int      chunkSize;
        unsigned char     formType[4];
        FMTHeader         fmtHdr;
        DATAHeader        dataHdr;
} RIFFHeader;
//------------------------------------

RIFFHeader gWavHdr;

int optset(dataca,data)
int dataca;
char *data[];
{
int cnt1;
static char *kakutyo= ".int";
        if(dataca == 1) {
                puts("");
                puts(" Wav format -> int format  PS2 PCM raw-format convertor");
                puts("              wav2int filename");
                return(0);
        }
        cnt1= 1;
        while( *data[cnt1] == '-') cnt1++;
        strncpy(mfna,data[cnt1],32);
        strncpy(kfna,mfna,32);

        cnt1= 1;
        while( *data[cnt1] == '-'){
                if(*(data[cnt1] + 1) == 'A' || *(data[cnt1] + 1) == 'a'){
                        puts("-A");
                }
                cnt1++;
        }
        while( kfna[cnt1] != '.' && kfna[cnt1] ) cnt1++;
        kfna[cnt1]= 0;
        strcat(kfna,kakutyo);

        return(NORMAL);
}

void main(argc,argv)
{
int cnt1,cnt2;
unsigned int offset,readcnt;
short dzero= 0;
FILE *wfp,*rfp;
        if(!optset(argc,argv)) return;

        printf("open fname= %s\n",mfna);
        if((rfp= fopen(mfna,"rb")) == NULL){
                puts("file not found");
                return;
        }
        printf("open fname= %s\n",kfna);
        if((wfp= fopen(kfna,"wb")) == NULL){
                puts("file open fail");
                return;
	}
        if(fread((unsigned char*)(&gWavHdr),1,RIFF_HEADER_SIZE,rfp)
						 != RIFF_HEADER_SIZE ) {
                printf("file read failed. %s \n", mfna);
                return;
        }
        offset = (unsigned int)&(gWavHdr.dataHdr.data) -(unsigned int)&gWavHdr;
        fseek(rfp, offset , SEEK_SET );
	//printf("Data start= %x\n", offset);
	while((readcnt= fread(srcbuf,1,1024,rfp)) != 0){
	    //printf("Read Data = %d\n", readcnt);
	    for(cnt1= 0; cnt1 < 1024; cnt1+= 4){
		if(readcnt < cnt1) fwrite(&dzero,1,2,wfp);
		else fwrite(&srcbuf[cnt1],1,2,wfp);
	    }
	    for(cnt1= 2; cnt1 < 1024; cnt1+= 4){
		if(readcnt < cnt1) fwrite(&dzero,1,2,wfp);
		else fwrite(&srcbuf[cnt1],1,2,wfp);
	    }
	}
	fclose(rfp);
	fclose(wfp);
}
