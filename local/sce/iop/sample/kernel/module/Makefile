#
#  IOP module sample program Makefile
#
#  $Id: Makefile,v 1.6 2001/05/20 09:23:33 tei Exp $
#

#----------- customize section --------------
DBGFLAG  = -g
#OPTFLAG  = -O2
CFLAGS   = -Wall $(DBGFLAG) $(OPTFLAG)
ASFLAGS  = $(DBGFLAG) 
LDFLAGS  = $(DBGFLAG)
LOADLIBES=

PROGRAMS = mylib.irx mylibrm.irx client.irx  clientrm.irx \
		modlist.irx searchmd.irx removemd.irx

all: $(PROGRAMS) dumpcheck

clean:
	rm -f *.o *.irx *.ilb *.map

run: $(PROGRAMS) 
	dsreset 0 2; dsistart mylib.irx; dsistart client.irx

# 常駐ライブラリのコンパイル ===============================================
mylib.irx : mylib.o mylib.tbl
#	  ライブラリエントリテーブルの作成
	$(ILBGEN) mylib.tbl -e mylibent.s
	$(AS) -o mylibent.o mylibent.s
	rm -f mylibent.s
#	   常駐ライブラリの本体のコンパイルとエントリテーブルとのリンク
# 	   -e オプションで初期化エントリを指定してください
	$(LINK.o) -e MyLibInit mylib.o mylibent.o  \
		$(LOADLIBES) $(LDLIBS) -o $@

mylibrm.irx : mylibrm.o mylib.tbl
	$(ILBGEN) mylib.tbl -e mylibent.s
	$(AS) -o mylibent.o mylibent.s
	rm -f mylibent.s
	$(LINK.o) -e MyLibInit mylibrm.o mylibent.o  \
		$(LOADLIBES) $(LDLIBS) -o $@

# ライブラリを利用するプログラムのリンク時に必要な mylib.ilb ファイルの作成
mylib.ilb: mylib.tbl
	$(ILBGEN) mylib.tbl -d mylib.ilb

# ライブラリを利用するプログラムのコンパイル ===============================
client.irx : client.o mylib.ilb
	$(LINK.o) client.o $(LOADLIBES) $(LDLIBS) -o $@ \
		-ilb=mylib.ilb
clientrm.irx : clientrm.o mylib.ilb
	$(LINK.o) clientrm.o $(LOADLIBES) $(LDLIBS) -o $@ \
		-ilb=mylib.ilb

# 参考 ===============================
#	  プログラムがどの常駐ライブラリを呼び出しているかは
#	  ioplibdumpユーティリティで確認できます。
#	  さらに、どの関数を呼び出しているかも ilb ファイルを
#	  指定することで確認できます
dumpcheck:
	$(ILBDUMP) client.irx 
	$(ILBDUMP) client.irx : mylib.ilb

#----------- rules --------------
%.irx: %.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

-include PathDefs
PathDefs:
	iop-path-setup > PathDefs || (rm -f PathDefs ; exit 1)

