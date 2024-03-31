/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/* 
 *                    I/O Processor Library
 *
 *                           - ftp -
 *
 *                        Version 1.0.0
 *                          Shift-JIS
 *
 *      Copyright (C) 2001 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 * hddpart.c - including functions to access HDD
 *
 * $Id: hddpart.c,v 1.3 2001/06/19 00:34:02 ywashizu Exp $
 * $Author: ywashizu $
 *
 */


#include "hddpart.h"

static void showHddMode(u_short mode) {
    int i;
    char *p, str[12];
    p = str;
    *p++ = (SCE_STM_ISDIR(mode)) ? 'd' : (SCE_STM_ISLNK(mode)) ? 'l': '-';
    for (i=8; i>=0; i--) {
	if ((1 << i) & mode) {
	    if (i == 9) *p++ = 's';
	    else if ((i%3) == 2) *p++ = 'r';
	    else if ((i%3) == 1) *p++ = 'w';
	    else if ((i%3) == 0) *p++ = 'x';
	} else *p++ = '-';
    }
    *p = '\0';
    printf("%s", str);
}

static void showHddStat(struct sce_stat *ss) {
    char *strmonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			  "Jal", "Aug", "Sep", "Oct", "Nov", "Dec",};
    unsigned long long  size = ((unsigned long long)ss->st_hisize << 32) | ss->st_size;

    showHddMode(ss->st_mode);
    printf(" %-5d %-5d 0x%04x %5d %9d %s %2d %2d:%02d",
	    ss->st_private[0], ss->st_private[1], ss->st_attr,
	    ss->st_private[2], (u_int)size,
	    strmonth[ss->st_mtime[5]-1],
	    ss->st_mtime[4], ss->st_mtime[3], ss->st_mtime[2]);
}

int showHddDir(char *dir) {
    int r, fd;
    struct sce_dirent dirbuf;

    if ((fd = dopen(dir)) < 0) {
	printf("cannot open directory %s, %d\n", dir, fd);
	return fd;
    }
    while ((r = dread(fd, &dirbuf)) > 0) {
	showHddStat(&dirbuf.d_stat);
	printf(" %s\n", dirbuf.d_name);
    }
    dclose(fd);
    return r;
}

void releaseHdd(char* path)
{
    int r;
    char path_name[256];

    sprintf(path_name, "%s/", path);

    if ((r = umount(path)) < 0) {
        printf("cannot umount partition %d\n", r);
        return;
    }
}

int setupHdd(char* path, char* cwd, int mode)
{
    int r;
    int zonesz;
    int fd;
    char path_name[256];

    sprintf(path_name, "%s/", path);

    if(mode){
	// initialize disk
	if ((r = format("hdd0:", NULL, NULL, 0)) < 0) {
	    printf("cannot format disk\n");
	    return -1;
	}
	
	r = devctl("hdd0:", HDIOC_MAXSECTOR, NULL, 0, NULL, 0);
	printf("max # of sectors for one partition = %d, %dMbytes\n", r,
	       ((r/1024)*SECTOR_SIZE)/1024);
	
	if ((fd = open("hdd0:_tmp,,,1G,PFS", O_CREAT|O_RDWR)) < 0) {
	    printf("cannot create partition\n");
	    return -1;
	}
	close(fd);                  // we must close before, format or mount
	
	// initialize partition
	zonesz = 8192;
	if ((r = format(path_name, "hdd0:_tmp", &zonesz, 4)) < 0) {
	    printf("cannot format partition\n");
	    return -1;
	}
    }

    if ((r = mount(path_name, "hdd0:_tmp", SCE_MT_RDWR, NULL, 0)) < 0) {
        printf("cannot mount partition\n");
        return -1;
    }

    // clear path buffer
    memset(cwd, 0, sizeof(cwd));
    cwd[0] = '\0';

    printf("cwd first %s\n", cwd);

    return 0;
}

void updateCwd(char *p, char* cwd){
    char *q;

    if(!strcmp(p, ".")){
	return ;
    }
    else if(!strcmp(p, "..")){
	if((q = strrchr(cwd, '/'))){
	    *q = '\0';
	}
	else{
		cwd[0] = '\0';
	}
    }
    else if(p[0] == '/'){                    // in case absolute path
	memset(cwd, 0, sizeof(cwd));
	strcat(cwd, &p[1]);
    }
    else{
	if(strlen(cwd))strcat(cwd, "/");
	strcat(cwd, p);
    }
}
