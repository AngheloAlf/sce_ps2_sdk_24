#
#  IOP module sample program Makefile
#
#  $Id: Makefile,v 1.4 2001/01/12 01:56:59 komaki Exp $
#
ifeq ($(wildcard PathDefs),)
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)
	$(MAKE) all
else
include PathDefs
endif

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

#----------- customize section --------------
ifdef SCETOP
TOP	    = $(SCETOP)
else
TOP	    = /usr/local/sce
endif
EE_LIBNET_DIR	= ../../../../ee/sample/inet/libnet
MSIFILB	= msifrpc.ilb
INETILB	= inet.ilb
NETCNFILB	= netcnf.ilb
INETCTLILB	= inetctl.ilb
IOPINC = $(TOP)/iop/include
IOPLIB = $(TOP)/iop/lib
COMMONINC = $(TOP)/common/include

INCOPT = -I$(COMMONINC) -I$(IOPINC) -I$(EE_LIBNET_DIR)

#INST_DIR    = ../../target

CFLAGS   =  -g -Wall $(INCOPT)
ASFLAGS  = 
LOADLIBES=

MODNAME  = libnet
ENTRY    = start

OBJS	 = libnet.o

INSTALL     = install

#all: $(MODNAME).irx dumpcheck
all: $(MODNAME).irx $(EE_LIBNET_DIR)/libnet.a

$(EE_LIBNET_DIR)/libnet.a:
	cd $(EE_LIBNET_DIR); make

clean:
	rm -f *.o *.irx *.ilb *.map

# 常駐ライブラリのコンパイル ===============================================
$(MODNAME).irx : $(OBJS) $(MODNAME).tbl
#	  ライブラリエントリテーブルの作成
	$(ILBGEN) $(MODNAME).tbl -e $(MODNAME)ent.s
	$(AS) -o $(MODNAME)ent.o $(MODNAME)ent.s
	rm -f $(MODNAME)ent.s
#	   常駐ライブラリの本体のコンパイルとエントリテーブルとのリンク
# 	   -e オプションで初期化エントリを指定してください
	$(LINK.o) -e $(ENTRY) $(OBJS) $(MODNAME)ent.o  \
		$(LOADLIBES) $(LDLIBS) -o $@ -ilb=$(INETILB) -ilb=$(MSIFILB) -ilb=$(NETCNFILB) -ilb=$(INETCTLILB)

# ライブラリを利用するプログラムのリンク時に必要な main.ilb ファイルの作成
$(MODNAME).ilb: $(MODNAME).tbl
	$(ILBGEN) $(MODNAME).tbl -d $(MODNAME).ilb

#install : all
#	$(INSTALL) -d $(INST_DIR)
#	cp $(MODNAME).irx $(INST_DIR)

# 参考 ===============================
#	  プログラムがどの常駐ライブラリを呼び出しているかは
#	  ioplibdumpユーティリティで確認できます。
#	  さらに、どの関数を呼び出しているかも ilb ファイルを
#	  指定することで確認できます
#dumpcheck:
#	$(ILBDUMP) $(MODNAME).ilb

