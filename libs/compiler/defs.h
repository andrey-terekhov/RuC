/*
 *	Copyright 2014 Andrey Terekhov
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#pragma once

#define MAXREPRTAB	10000
#define MAXIDENTAB	10000
#define MAXTREESIZE 10000
#define MAXMODETAB	1000
#define MAXBOUNDS	100
#define FUNCSIZE	100
#define SOURCESIZE	10000
#define LINESSIZE	300
#define MAXSTRINGL	128

#define MAXPRINTFPARAMS 20


// modetab

enum MODE
{
	mode_undefined = 0,
	mode_integer = -1,
	mode_character = -2,
	mode_float = -3,
	mode_void = -6,

	mode_void_pointer = 15,
	mode_msg_info = 2,
	mode_function = 1001,
	mode_struct = 1002,
	mode_array = 1003,
	mode_pointer = 1004,
};


// Лексемы операций языка С

#define REMASS	9001
#define SHLASS	9002
#define SHRASS	9003
#define ANDASS	9004
#define EXORASS 9005
#define ORASS	9006

#define ASS		 9007
#define PLUSASS	 9008
#define MINUSASS 9009
#define MULTASS	 9010
#define DIVASS	 9011

#define REMASSAT  9012 // эти 22 операции с присваиванием оставляют значение на стеке
#define SHLASSAT  9013
#define SHRASSAT  9014
#define ANDASSAT  9015
#define EXORASSAT 9016
#define ORASSAT	  9017

#define ASSAT	   9018
#define PLUSASSAT  9019
#define MINUSASSAT 9020
#define MULTASSAT  9021
#define DIVASSAT   9022

#define REMASSV	 9201
#define SHLASSV	 9202
#define SHRASSV	 9203
#define ANDASSV	 9204
#define EXORASSV 9205
#define ORASSV	 9206

#define ASSV	  9207
#define PLUSASSV  9208
#define MINUSASSV 9209
#define MULTASSV  9210
#define DIVASSV	  9211

#define REMASSATV  9212 // а эти 22 операции с присваиванием не оставляют значение на стеке
#define SHLASSATV  9213
#define SHRASSATV  9214
#define ANDASSATV  9215
#define EXORASSATV 9216
#define ORASSATV   9217

#define ASSATV		9218
#define PLUSASSATV	9219
#define MINUSASSATV 9220
#define MULTASSATV	9221
#define DIVASSATV	9222

#define LREM   9023
#define LSHL   9024
#define LSHR   9025
#define LAND   9026
#define LEXOR  9027
#define LOR	   9028
#define LOGAND 9029
#define LOGOR  9030

#define EQEQ   9031
#define NOTEQ  9032
#define LLT	   9033
#define LGT	   9034
#define LLE	   9035
#define LGE	   9036
#define LPLUS  9037
#define LMINUS 9038
#define LMULT  9039 // у этих 9 команд есть такие же с плавающей зпт
#define LDIV   9040

#define POSTINC	   9041
#define POSTDEC	   9042
#define INC		   9043
#define DEC		   9044
#define POSTINCAT  9045
#define POSTDECAT  9046
#define INCAT	   9047
#define DECAT	   9048
#define POSTINCV   9241
#define POSTDECV   9242
#define INCV	   9243
#define DECV	   9244
#define POSTINCATV 9245
#define POSTDECATV 9246
#define INCATV	   9247
#define DECATV	   9248


#define UNMINUS 9049

#define LNOT   9052
#define LOGNOT 9053


#define ASSR	  9057
#define PLUSASSR  9058
#define MINUSASSR 9059
#define MULTASSR  9060
#define DIVASSR	  9061

#define ASSATR		9068
#define PLUSASSATR	9069
#define MINUSASSATR 9070
#define MULTASSATR	9071
#define DIVASSATR	9072

#define ASSRV	   9257
#define PLUSASSRV  9258
#define MINUSASSRV 9259
#define MULTASSRV  9260
#define DIVASSRV   9261

#define ASSATRV		 9268
#define PLUSASSATRV	 9269
#define MINUSASSATRV 9270
#define MULTASSATRV	 9271
#define DIVASSATRV	 9272

#define EQEQR	9081
#define NOTEQR	9082
#define LLTR	9083
#define LGTR	9084
#define LLER	9085
#define LGER	9086
#define LPLUSR	9087
#define LMINUSR 9088
#define LMULTR	9089
#define LDIVR	9090

#define POSTINCR	9091
#define POSTDECR	9092
#define INCR		9093
#define DECR		9094
#define POSTINCATR	9095
#define POSTDECATR	9096
#define INCATR		9097
#define DECATR		9098
#define POSTINCRV	9291
#define POSTDECRV	9292
#define INCRV		9293
#define DECRV		9294
#define POSTINCATRV 9295
#define POSTDECATRV 9296
#define INCATRV		9297
#define DECATRV		9298

#define UNMINUSR 9099


// Коды операций виртуальной машины, кроме операций C

#define NOP			  9453
#define DEFARR		  9454
#define LI			  9455
#define LID			  9456
#define LOAD		  9457
#define LOADD		  9458
#define LAT			  9459 // это L@, т.е. адрес ячейки на вершине стека заменить ее значением
#define LATD		  9460 // на вершине стека адрес вещественного значения, которое надо положить на стек
#define STOP		  9461
#define SELECT		  9462 // операнд - смещение, которое эта команда добавит к верхушке стека
#define FUNCBEG		  9463 // обозначает начало кода функции
#define LA			  9464
#define CALL1		  9465
#define CALL2		  9466
#define RETURNVAL	  9467
#define RETURNVOID	  9468
#define B			  9469
#define BE0			  9470
#define BNE0		  9471
#define SLICE		  9472
#define WIDEN		  9473
#define WIDEN1		  9474
#define _DOUBLE		  9475
#define STRINGINIT	  9476
#define ARRINIT		  9477
#define STRUCTWITHARR 9478
#define ADLOGAND	  9479
#define ADLOGOR		  9480
#define BEGINIT		  9481
#define ROWING		  9482
#define ROWINGD		  9483


#define COPY00	   9300 // d1, d2, l
#define COPY01	   9301 // d1, l
#define COPY10	   9302 // d2, l
#define COPY11	   9303 // l
#define COPY0ST	   9304 // d1, l		to stack
#define COPY1ST	   9305 // l			to stack
#define COPY0STASS 9306 // d1, l		from stack
#define COPY1STASS 9307 // l			from stack
#define COPYST	   9308 // d1, d2, l	структура - значение функции


// Коды операций стандартных функций

#define ABSIC 9651
#define CASTC 9526

#define CREATEDIRECTC 9528
#define EXITDIRECTC	  9529

#define SETMOTORC	  9530
#define GETDIGSENSORC 9531
#define GETANSENSORC  9532
#define VOLTAGEC	  9533
#define ABSC		  9534
#define SQRTC		  9535
#define EXPC		  9536
#define SINC		  9537
#define COSC		  9538
#define LOGC		  9539
#define LOG10C		  9540
#define ASINC		  9541
#define RANDC		  9542
#define ROUNDC		  9543

#define STRCPYC	 9544
#define STRNCPYC 9545
#define STRCATC	 9546
#define STRNCATC 9547
#define STRCMPC	 9548
#define STRNCMPC 9549
#define STRSTRC	 9550
#define STRLENC	 9551

#define MSGSENDC			 9552
#define MSGRECEIVEC			 9553
#define JOINC				 9554
#define SLEEPC				 9555
#define SEMCREATEC			 9556
#define SEMWAITC			 9557
#define SEMPOSTC			 9558
#define CREATEC				 9559
#define INITC				 9560
#define DESTROYC			 9561
#define EXITC				 9562
#define GETNUMC				 9563
#define WIFI_CONNECTC		 9571
#define BLYNK_AUTHORIZATIONC 9572
#define BLYNK_SENDC			 9573
#define BLYNK_RECEIVEC		 9574
#define BLYNK_NOTIFICATIONC	 9575
#define BLYNK_PROPERTYC		 9576
#define BLYNK_LCDC			 9577
#define BLYNK_TERMINALC		 9578

#define SETSIGNALC		9579
#define PIXELC			9580
#define LINEC			9581
#define RECTANGLEC		9582
#define ELLIPSEC		9583
#define CLEARC			9584
#define DRAW_STRINGC	9585
#define DRAW_NUMBERC	9586
#define ICONC			9587
#define UPBC			9588
#define SEND_INTC		9589
#define SEND_FLOATC		9590
#define SEND_STRINGC	9591
#define RECEIVE_INTC	9592
#define RECEIVE_FLOATC	9593
#define RECEIVE_STRINGC 9594

#define ASSERTC 9595


// Лексемы

#define QUEST		101
#define LEFTBR		103
#define LEFTSQBR	105
#define STRING		107
#define IDENT		109
#define CHAR_CONST	111
#define INT_CONST	112
#define FLOAT_CONST	113
#define BEGIN		115
#define LAPOST		118
#define LQUOTE		119
#define LEOF		120
#define ARROW		121
#define DOT			122

// Такие коды сделаны для функции token_skip_until()
// Это позволяет передавать несколько аргументов, разделяя их |
#define COMMA		0b00000001
#define COLON		0b00000010
#define RIGHTBR		0b00000100
#define RIGHTSQBR	0b00001000
#define END			0b00010000
#define SEMICOLON	0b00100000


// Ключевые слова

#define LMAIN	0
#define LINT	-1
#define LCHAR	-2
#define LFLOAT	-3
#define LLONG	-4
#define LDOUBLE -5
#define LVOID	-6

#define LBREAK		  -7
#define LCASE		  -8
#define LCONTINUE	  -9
#define LDEFAULT	  -10
#define LDO			  -11
#define LELSE		  -12
#define LENUM		  -13
#define LSTRUCT		  -14
#define LTYPEDEF	  -15
#define LFOR		  -16
#define LGOTO		  -17
#define LIF			  -18
#define LRETURN		  -19
#define LSIZEOF		  -20
#define LSWITCH		  -21
#define LWHILE		  -22
#define PRINTID		  -23
#define PRINT		  -24
#define PRINTF		  -25
#define SCANF		  -26
#define GETID		  -27
#define TCREATEDIRECT -28
#define TEXITDIRECT	  -29

#define STANDARD_FUNC_START -30
#define SETMOTOR			-30
#define GETDIGSENSOR		-31
#define GETANSENSOR			-32
#define VOLTAGE				-33

#define ABS	  -34
#define SQRT  -35
#define EXP	  -36
#define SIN	  -37
#define COS	  -38
#define LOG	  -39
#define LOG10 -40
#define ASIN  -41
#define RAND  -42
#define ROUND -43

#define STRCPY	-44
#define STRNCPY -45
#define STRCAT	-46
#define STRNCAT -47
#define STRCMP	-48
#define STRNCMP -49
#define STRSTR	-50
#define STRLEN	-51


#define TMSGSEND	-52
#define TMSGRECEIVE -53
#define TJOIN		-54
#define TSLEEP		-55
#define TSEMCREATE	-56
#define TSEMWAIT	-57
#define TSEMPOST	-58
#define TCREATE		-59
#define TINIT		-60
#define TDESTROY	-61
#define TEXIT		-62
#define TGETNUM		-63

#define WIFI_CONNECT	   -71
#define BLYNK_AUTORIZATION -72
#define BLYNK_SEND		   -73
#define BLYNK_RECEIVE	   -74
#define BLYNK_NOTIFICATION -75
#define BLYNK_PROPERTY	   -76
#define BLYNK_LCD		   -77
#define BLYNK_TERMINAL	   -78

#define SETSIGNAL	   -79
#define PIXEL		   -80
#define LINE		   -81
#define RECTANGLE	   -82
#define ELLIPS		   -83
#define CLEAR		   -84
#define DRAW_STRING	   -85
#define DRAW_NUMBER	   -86
#define ICON		   -87
#define UPB			   -88
#define SEND_INT	   -89
#define SEND_FLOAT	   -90
#define SEND_STRING	   -91
#define RECEIVE_INT	   -92
#define RECEIVE_FLOAT  -93
#define RECEIVE_STRING -94

#define ASSERT -95


#define LVOIDASTER -150
#define ABSI	   -151


// Узлы дерева

#define TIdent		 -300
#define TConst		 -301
#define TString		 -302
#define TSliceident	 -303
#define TSlice		 -304
#define TIdenttoval	 -305
#define TAddrtoval	 -306
#define TCall1		 -307
#define TCall2		 -308
#define TFuncdef	 -309
#define TDeclid		 -310
#define TStringd	 -311
#define TExprend	 -312
#define TCondexpr	 -313
#define TBegin		 -314
#define TEnd		 -315
#define TIf			 -316
#define TWhile		 -317
#define TDo			 -318
#define TFor		 -319
#define TSwitch		 -320
#define TCase		 -321
#define TDefault	 -322
#define TBreak		 -323
#define TContinue	 -324
#define TReturnvoid	 -325
#define TReturnval	 -326
#define TGoto		 -327
#define TLabel		 -328
#define TPrint		 -329
#define TPrintid	 -330
#define TPrintf		 -331
#define TGetid		 -332
#define TIdenttoaddr -333
#define TSelect		 -334
#define TFunidtoval	 -335
#define TStructbeg	 -336
#define TStructend	 -337
#define TDeclarr	 -338
#define TConstd		 -339
#define TIdenttovald -340
#define TAddrtovald	 -341
#define TBeginit	 -342
#define TStructinit	 -343


// коды команд MIPS

#define bltz  1	  // bltz rs, label		if rs < 0					bgez rs, label	if rs >= 0
#define jump  2	  // j label
#define jal	  3	  // jal label
#define beq	  4	  // beq rs, rt, label	if rs == rt
#define bne	  5	  // bne rs, rt, label	if rs != rt
#define blez  6	  // blez rs, label		if rs <= 0
#define bgtz  7	  // bgtz rs, label		if rs > 0
#define addi  8	  // addi rt, rs, imm		rt = rs + SignImm
#define addiu 9	  // addiu rt, rs, imm	rt = rs + SignImm			no overflow
#define slti  10  // slti rt, rs, imm		rt = rs < SignImm ? 1 : 0
#define sltiu 11  // sltiu rt, rs, imm	rt = rs < SignImm ? 1 : 0	unsigned
#define andi  12  // andi rt, rs, imm		rt = rs & ZeroImm
#define ori	  13  // ori rt, rs, imm		rt = rs | ZeroImm
#define xori  14  // xori rt, rs, imm		rt = rs ^ ZeroImm
#define lui	  15  // lui rt, imm			rt = {imm, 16'b0}
#define li	  115 // li rt, imm(32)		это псевдокоманда (lui + ori)
#define mul	  28  // mul rd, rs, rt		rd = rs * rt
#define lw	  35  // lw rt, imm(rs)		rt = [Address]
#define sw	  43  // sw rt, imm(rs)		[Address] = rt

#define sll	 60	 // sll rd, rt, shamt	rd = rt << shsmt
#define srl	 62	 // srl rd, rt, shamt	rd = rt >> shsmt
#define sra	 63	 // sra rd, rt, shamt	rd = rt >> shsmt			arithmetic
#define sllv 64	 // sllv rd, rt, rs		rd = rt << [rs]4:0
#define srlv 66	 // srlv rd, rt, rs		rd = rt >> [rs]4:0
#define srav 67	 // srav rd, rt, rs		rd = rt >> [rs]4:0			arithmetic
#define jr	 68	 // jr rs				PC = rs
#define jalr 69	 // jalr rs
#define add	 92	 // add rd, rs, rt		rd = rs + rt
#define addu 93	 // addu rd, rs, rt		rd = rs + rt				unsigned
#define sub	 94	 // sub rd, rs, rt		rd = rs - rt
#define subu 95	 // subu rd, rs, rt		rd = rs - rt				unsigned
#define and 96	 // and rd, rs, rt		rd = ts & rt
#define or 97	 // or rd, rs, rt		rd = ts | rt
#define xor 98	 // xor rd, rs, rt		rd = ts ^ rt
#define nor	 99	 // nor rd, rs, rt		rd = ~(rs | rt)
#define slt	 102 // slt rd, rs, rt		rd = rs < rt ? 1 : 0
#define sltu 103 // sltu rd, rs, rt		rd = rs < rt ? 1 : 0		unsigned
