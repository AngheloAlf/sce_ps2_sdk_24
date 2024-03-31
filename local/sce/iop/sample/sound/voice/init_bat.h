/* SCEI CONFIDENTIAL
 "PlayStation 2" Programmer Tool Runtime Library Release 2.4
 */
/*
 *              I/O Processor Library Sample Program
 *
 *                          - voice -
 *
 *                           Shift-JIS
 *
 *      Copyright (C) 1998-1999 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 *                            init_bat.h
 *
 *     Version   Date          Design     Log
 *  --------------------------------------------------------------------
 *     0.60      Oct.14.1999   morita     first checked in.
 */

//テスト用

//--- common初期化バッチ
sceSdBatch gBatchCommonInit[4] = {
	{ SD_BSET_PARAM, SD_CORE_0|SD_P_MVOLL, 0x3fff },
	{ SD_BSET_PARAM, SD_CORE_0|SD_P_MVOLR, 0x3fff },
	{ SD_BSET_PARAM, SD_CORE_1|SD_P_MVOLL, 0x3fff },
	{ SD_BSET_PARAM, SD_CORE_1|SD_P_MVOLR, 0x3fff },
};

sceSdBatch gBatchCommonInit2[7] = {
	{ SD_BGET_PARAM, SD_CORE_0|SD_P_MVOLL, 0 },
	{ SD_BGET_PARAM, SD_CORE_0|SD_P_MVOLR, 0 },
	{ SD_BGET_PARAM, SD_CORE_1|SD_P_MVOLL, 0 },
	{ SD_BGET_PARAM, SD_CORE_1|SD_P_MVOLR, 0 },
	{ SD_WRITE_IOP, 0x1234, 0x90000 },
	{ SD_WRITE_EE,  0x4321, 0x1000f0 },
	{ SD_RETURN_EE, 16, 0x100000 }
};


//--- ボイス初期化バッチ
sceSdBatch gBatchVoiceInit[2][6] = {
    {
	{ SD_BSET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_VOLL, 0x1fff },
	{ SD_BSET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_VOLR, 0x1fff },
	{ SD_BSET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_PITCH, 0x400 },
	{ SD_BSET_ADDR,  SD_CORE_0|SD_VOICE_XX|SD_VA_SSA , VAG_ADDR },
	{ SD_BSET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_ADSR1, 
			SD_ADSR1(SD_ADSR_A_EXP, 30, 14, 14) },
	{ SD_BSET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_ADSR2, 
			SD_ADSR2(SD_ADSR_S_EXP_DEC, 52, SD_ADSR_R_EXP, 13) },
    },
    {
	{ SD_BSET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_VOLL, 0x1fff },
	{ SD_BSET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_VOLR, 0x1fff },
	{ SD_BSET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_PITCH, 0x400 },
	{ SD_BSET_ADDR,  SD_CORE_1|SD_VOICE_XX|SD_VA_SSA , VAG_ADDR },
	{ SD_BSET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_ADSR1, 
			SD_ADSR1(SD_ADSR_A_EXP, 30, 14, 14) },
	{ SD_BSET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_ADSR2, 
			SD_ADSR2(SD_ADSR_S_EXP_DEC, 52, SD_ADSR_R_EXP, 13) },
    }
};
    

sceSdBatch gBatchVoiceInit2[2][6] = {
    {
	{ SD_BGET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_VOLL, 0x1fff },
	{ SD_BGET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_VOLR, 0x1fff },
	{ SD_BGET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_PITCH, 0x400 },
	{ SD_BGET_ADDR,  SD_CORE_0|SD_VOICE_XX|SD_VA_SSA , VAG_ADDR },
	{ SD_BGET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_ADSR1, 
			SD_ADSR1(SD_ADSR_A_EXP, 30, 14, 14) },
	{ SD_BGET_PARAM, SD_CORE_0|SD_VOICE_XX|SD_VP_ADSR2, 
			SD_ADSR2(SD_ADSR_S_EXP_DEC, 52, SD_ADSR_R_EXP, 13) },
    },
    {
	{ SD_BGET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_VOLL, 0x1fff },
	{ SD_BGET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_VOLR, 0x1fff },
	{ SD_BGET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_PITCH, 0x400 },
	{ SD_BGET_ADDR,  SD_CORE_1|SD_VOICE_XX|SD_VA_SSA , VAG_ADDR },
	{ SD_BGET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_ADSR1, 
			SD_ADSR1(SD_ADSR_A_EXP, 30, 14, 14) },
	{ SD_BGET_PARAM, SD_CORE_1|SD_VOICE_XX|SD_VP_ADSR2, 
			SD_ADSR2(SD_ADSR_S_EXP_DEC, 52, SD_ADSR_R_EXP, 13) },
    }
};


