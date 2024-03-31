/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *		I/O Processor Library Sample Program
 *
 *			-- Module --
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                         mylib.c
 *                         develop library
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *       2.3.0          May,19,2001     isii
 */

#include <stdio.h>
#include <kernel.h>

/* メモリに常駐するモジュールは, 以下のようにモジュール名と
 * モジュールバージョンを付けておくと便利です。
 */
#define MYNAME "mylib"
ModuleInfo Module = { MYNAME, 0x0101 };

/* ================================================================
 * 	常駐ライブラリとしての初期化エントリ
 * ================================================================ */

int MyLibInit()
{
    int err, oldei;
    /* mylib.tblから, ユーティリティ loplibgen によってエントリテーブルが
     * 生成されます。エントリテーブルのラベル名には, 'ライブラリ名_entry' が
     *  つけられます。
     */
    extern libhead mylib_entry; /* ライブラリ名_entry を参照 */

    printf("'%s' Start\n", MYNAME);
    /* モジュールの常駐のための初期化、登録などを行う。*/
    CpuSuspendIntr(&oldei);
    err = RegisterLibraryEntries(&mylib_entry);
    CpuResumeIntr(oldei);
    if( err == KE_LIBRARY_FOUND ) {
	/* 既に同名の常駐ライブラリがいるので登録に失敗 */
	printf("'%s' already exist. no resident\n", MYNAME);
	return NO_RESIDENT_END; /* 終了してメモリから退去 */
    } else if( err != KE_OK ) {
	printf("'%s' What happen ?\n", MYNAME);
	return NO_RESIDENT_END; /* 終了してメモリから退去 */
    }
    printf("'%s' resident\n", MYNAME);
    return RESIDENT_END; /* 終了して常駐する */
}

/* ================================================================
 * 	常駐ライブラリの各エントリの定義
 * ================================================================ */

/*
   モジュールは、それぞれ独自の GPレジスタの値を持ちます。
   ところが、あるモジュールから他のモジュールの常駐ライブラリを
   呼び出すと、GPレジスタは呼び出し側モジュールの GP値を保持したまま
   呼び出されます。
   従って常駐ライブラリ内で自モジュール内のグローバルデータを
   アクセスするとき問題が生じます。
   この問題を避けるために、以下のどちらかの方法で常駐ライブラリを
   作成してください。
    1) コンパイラのオプション -G 0 で常駐ライブラリをコンパイルする。
       これによって常駐ライブラリは GP レジスタを使用しないコードになります。
       ただしこの方法は常駐ライブラリが若干大きくなり実行速度が遅くなります。
    2) 常駐ライブラリの各エントリ関数を、下記のように、GPレジスタを待避して
       自モジュールの GP レジスタを設定するようにコーディングする。
      (下記のコーディングは gcc系統のコンパイラを使用した場合の例です。)
 */

void libentry1(char *name)
{
    unsigned long oldgp;
    asm volatile( "  move %0, $gp; la  $gp, _gp" : "=r" (oldgp));

    printf("        %s --> %s:libentry1()\n", name, MYNAME);

    asm volatile( "  move $gp, %0"  : : "r" (oldgp));
}

void internal_libentry2(char *name)
{
    unsigned long oldgp;
    asm volatile( "  move %0, $gp; la  $gp, _gp" : "=r" (oldgp));

    printf("        %s --> %s:libentry2()\n", name, MYNAME);

    asm volatile( "  move $gp, %0"  : : "r" (oldgp));
}
