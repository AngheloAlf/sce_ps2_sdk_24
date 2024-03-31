### Makefile

SHELL=/bin/sh

TOP = /usr/local/sce/ee
COMMON_INCDIR = $(TOP)/../common/include
PUB_INCDIR = $(TOP)/include
LIB_INST_DIR = $(TOP)/lib
HDR_INST_DIR = $(TOP)/include

INCOPT = -I$(COMMON_INCDIR) -I$(PUB_INCDIR)

TARGET = librspu2
OBJS = rs_main.o rs_cb.o rs_st.o
INST_HDRS =

PREFIX=ee
AS = $(PREFIX)-gcc
CC = $(PREFIX)-gcc
AR = $(PREFIX)-ar
RANLIB = $(PREFIX)-ranlib
DVPASM = $(PREFIX)-dvp-as
RM=/bin/rm -f
INSTALL = install


CFLAGS = -c -Wa,-al -fno-common -O2 $(INCOPT) -G0
ASFLAGS = -c -xassembler-with-cpp -Wa,-al $(INCOPT)
DVPASMFLAGS = -g $(INCOPT)

.SUFFIXES: .c .s .cc .dsm

all: $(TARGET).a $(LIB_INST_DIR)

$(TARGET).a: $(OBJS)
	$(RM) $@
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@ 

.s.o:
	$(AS) $(ASFLAGS) -o $@ $< > $*.lst

.c.o:
	$(CC) $(CFLAGS) -c $< -o $*.o > $*.lst

.dsm.o:
	$(DVPASM) $(DVPASMFLAGS) -o $@ $< > $*.lst

clean:
	/bin/rm -f *.o *.lst core *.elf *.a

install: all
	$(INSTALL) -d $(LIB_INST_DIR)
#	$(INSTALL) -d $(HDR_INST_DIR)
	cp $(TARGET).a $(LIB_INST_DIR)
#	cp crt0.s $(LIB_INST_DIR)
#	cp app.cmd $(LIB_INST_DIR)
#	cp $(INST_HDRS) $(HDR_INST_DIR)

