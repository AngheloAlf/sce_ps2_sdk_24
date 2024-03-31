#
#  IOP SPU2 Synthesizer program Makefile
#
#  $Id: Makefile,v 1.5 2001/05/19 14:00:32 kaol Exp $
#

ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
endif
include PathDefs

TOP = ../../..

     RM = /bin/rm -f

  CDEFS =
XCFLAGS = -I$(TOP)/include -I$(TOP)/../common/include -I/usr/local/sce/common/include $(CDEFS)
ASFLAGS = -I$(TOP)/include -I$(TOP)/../common/include -I/usr/local/sce/common/include $(CDEFS)
LDFLAGS = -L$(TOP)/lib -noiopilb -G 0 -s $(LDOPT)

CFLAGS  = -Wall -G 0 -O2 $(XCFLAGS)

#----------- customize section --------------
MODULE   = sdrdrv
IRX      = $(MODULE).irx
ILB      = $(MODULE).ilb
TBL      = $(MODULE).tbl
ENT      = $(MODULE).o

SRCS = sdd_main.c sdd_com.c sdd_cb.c
OBJS = sdd_main.o sdd_com.o sdd_cb.o $(ENT)

INIT_OBJ = sdd_com.o

LDOPT    = 
LIBS     = -ilb=libsd.ilb -ilb=iop.ilb

all:	$(IRX) $(ILB)

$(IRX): $(OBJS)
	$(LINK.o) $(OBJS) -o $(IRX) $(LIBS)

$(ILB): $(TBL)
	ioplibgen $(TBL) -d $(ILB)

$(ENT): $(TBL)
	ioplibgen $(TBL) -e mylibent.s
	$(AS) -o $(ENT) mylibent.s
	$(RM) mylibent.s

clean:
	$(RM) *.o *.irx *.map *.ilb

clear:
	$(RM) *~

install: all
	cp $(IRX) $(TOP)/modules
	cp $(ILB) $(TOP)/lib

world:	clean install

