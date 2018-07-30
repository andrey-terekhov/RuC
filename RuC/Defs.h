//
//  Defs.h
//  RuC
//
//  Created by Andrey Terekhov on 03/06/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
//

#ifndef RuC_Defs_h
#define RuC_Defs_h
#include <pthread.h>

#define MAXREPRTAB   10000
#define MAXIDENTAB   10000
#define MAXMEMSIZE   1000000
#define MAXTREESIZE  10000
#define MAXMODETAB   10000
#define MAXBOUNDS    1000
#define FUNCSIZE     1000
#define SOURCESIZE   10000
#define LINESSIZE    3000
#define MAXSTRINGL   128
#define INIPROSIZE   1000
#define NUMOFTHREADS 10
#define MAXMEMTHREAD MAXMEMSIZE/NUMOFTHREADS 
#define MAXPRINTFPARAMS 20

// modetab 
#define MFUNCTION 1001
#define MSTRUCT   1002
#define MARRAY    1003
#define MPOINT    1004

// Лексемы операций языка С

#define REMASS     9001
#define SHLASS     9002
#define SHRASS     9003
#define ANDASS     9004
#define EXORASS    9005
#define ORASS      9006

#define ASS        9007
#define PLUSASS    9008
#define MINUSASS   9009
#define MULTASS    9010
#define DIVASS     9011

#define REMASSAT   9012     // эти 22 операции с присваиванием оставляют значение на стеке
#define SHLASSAT   9013
#define SHRASSAT   9014
#define ANDASSAT   9015
#define EXORASSAT  9016
#define ORASSAT    9017

#define ASSAT      9018
#define PLUSASSAT  9019
#define MINUSASSAT 9020
#define MULTASSAT  9021
#define DIVASSAT   9022

#define REMASSV    9201
#define SHLASSV    9202
#define SHRASSV    9203
#define ANDASSV    9204
#define EXORASSV   9205
#define ORASSV     9206

#define ASSV       9207
#define PLUSASSV   9208
#define MINUSASSV  9209
#define MULTASSV   9210
#define DIVASSV    9211

#define REMASSATV  9212   // а эти 22 операции с присваиванием не оставляют значение на стеке
#define SHLASSATV  9213
#define SHRASSATV  9214
#define ANDASSATV  9215
#define EXORASSATV 9216
#define ORASSATV   9217

#define ASSATV     9218
#define PLUSASSATV 9219
#define MINUSASSATV 9220
#define MULTASSATV 9221
#define DIVASSATV  9222

#define LREM       9023
#define LSHL       9024
#define LSHR       9025
#define LAND       9026
#define LEXOR      9027
#define LOR        9028
#define LOGAND     9029
#define LOGOR      9030

#define EQEQ       9031
#define NOTEQ      9032
#define LLT        9033
#define LGT        9034
#define LLE        9035
#define LGE        9036
#define LPLUS      9037
#define LMINUS     9038
#define LMULT      9039    // у этих 27 команд есть такие же с плавающей зпт
#define LDIV       9040

#define POSTINC    9041
#define POSTDEC    9042
#define INC        9043
#define DEC        9044
#define POSTINCAT  9045
#define POSTDECAT  9046
#define INCAT      9047
#define DECAT      9048
#define POSTINCV   9241
#define POSTDECV   9242
#define INCV       9243
#define DECV       9244
#define POSTINCATV 9245
#define POSTDECATV 9246
#define INCATV     9247
#define DECATV     9248


#define UNMINUS    9049

#define LNOT       9052
#define LOGNOT     9053


#define ASSR       9057
#define PLUSASSR   9058
#define MINUSASSR  9059
#define MULTASSR   9060
#define DIVASSR    9061

#define ASSATR     9068
#define PLUSASSATR 9069
#define MINUSASSATR 9070
#define MULTASSATR 9071
#define DIVASSATR  9072

#define ASSRV      9257
#define PLUSASSRV  9258
#define MINUSASSRV 9259
#define MULTASSRV  9260
#define DIVASSRV   9261

#define ASSATRV      9268
#define PLUSASSATRV  9269
#define MINUSASSATRV 9270
#define MULTASSATRV  9271
#define DIVASSATRV   9272

#define EQEQR     9081
#define NOTEQR    9082
#define LLTR      9083
#define LGTR      9084
#define LLER      9085
#define LGER      9086
#define LPLUSR    9087
#define LMINUSR   9088
#define LMULTR    9089
#define LDIVR     9090

#define POSTINCR  9091
#define POSTDECR  9092
#define INCR      9093
#define DECR      9094
#define POSTINCATR 9095
#define POSTDECATR 9096
#define INCATR     9097
#define DECATR     9098
#define POSTINCRV  9291
#define POSTDECRV  9292
#define INCRV      9293
#define DECRV      9294
#define POSTINCATRV 9295
#define POSTDECATRV 9296
#define INCATRV     9297
#define DECATRV     9298

#define UNMINUSR   9099

// Коды операций виртуальной машины, кроме операций C

#define NOP     9453
#define DEFARR  9454
#define LI      9455
#define LID     9456
#define LOAD    9457
#define LOADD   9458
#define LAT     9459       // это L@, т.е. адрес ячейки на вершине стека заменить ее значением
#define LATD    9460       // на вершине стека адрес вещественного значения, которое надо положить на стек
#define STOP    9461
#define SELECT  9462       // операнд - смещение, которое эта команда добавит к верхушке стека
#define FUNCBEG 9463       // обозначает начало кода функции
#define LA      9464
#define CALL1   9465
#define CALL2   9466
#define RETURNVAL  9467
#define RETURNVOID 9468
#define B       9469
#define BE0     9470
#define BNE0    9471
#define SLICE   9472
#define WIDEN   9473
#define WIDEN1  9474
#define _DOUBLE 9475
#define STRINGINIT    9476
#define ARRINIT       9477
#define STRUCTWITHARR 9478
#define ADLOGAND      9479
#define ADLOGOR       9480
#define BEGINIT       9481

#define COPY00   9300
#define COPY01   9301
#define COPY10   9302
#define COPY11   9303
#define COPY0ST  9304
#define COPY1ST  9305
#define COPY0STASS  9306
#define COPY1STASS  9307
#define COPY10V  9502
#define COPY11V  9503
#define COPY1STASSV 9507


// Коды операций стандартных функций

#define ABSIC     9525
#define CASTC     9526

#define CREATEDIRECTC 9528
#define EXITDIRECTC   9529

#define SETMOTORC 9530
#define GETDIGSENSORC 9531
#define GETANSENSORC  9532
#define VOLTAGEC      9533
#define ABSC      9534
#define SQRTC     9535
#define EXPC      9536
#define SINC      9537
#define COSC      9538
#define LOGC      9539
#define LOG10C    9540
#define ASINC     9541
#define RANDC     9542
#define ROUNDC    9543

#define STRCPYC   9544
#define STRNCPYC  9545
#define STRCATC   9546
#define STRNCATC  9547
#define STRCMPC   9548
#define STRNCMPC  9549
#define STRSTRC   9550
#define STRLENC   9551

#define MSGSENDC  9552
#define MSGRECEIVEC 9553
#define JOINC     9554
#define SLEEPC    9555
#define SEMCREATEC  9556
#define SEMWAITC  9557
#define SEMPOSTC  9558
#define CREATEC   9559
#define INITC     9560
#define DESTROYC  9561
#define EXITC     9562
#define GETNUMC   9563
// Лексемы

#define COMMA     100
#define QUEST     101
#define COLON     102
#define LEFTBR    103
#define RIGHTBR   104
#define LEFTSQBR  105
#define RIGHTSQBR 106
#define STRING    107
#define NUMBER    108
#define IDENT     109
#define BEGIN     115
#define END       116
#define SEMICOLON 117
#define LAPOST    118
#define LQUOTE    119
#define LEOF      120
#define ARROW     121
#define DOT       122

// Ответы

#define VAL       110
#define ADDR      111

// Ключевые слова

#define LMAIN      0
#define LINT      -1
#define LCHAR     -2
#define LFLOAT    -3
#define LLONG     -4
#define LDOUBLE   -5
#define LVOID     -6

#define LBREAK    -7
#define LCASE     -8
#define LCONTINUE -9
#define LDEFAULT  -10
#define LDO       -11
#define LELSE     -12
#define LENUM     -13
#define LSTRUCT   -14
#define LTYPEDEF  -15
#define LFOR      -16
#define LGOTO     -17
#define LIF       -18
#define LRETURN   -19
#define LSIZEOF   -20
#define LSWITCH   -21
#define LWHILE    -22
#define PRINTID   -23
#define PRINT     -24
#define PRINTF    -25
#define SCANF     -26
#define GETID     -27
#define TCREATEDIRECT -28
#define TEXITDIRECT -29

#define STANDARD_FUNC_START -30
#define SETMOTOR  -30
#define GETDIGSENSOR -31
#define GETANSENSOR  -32
#define VOLTAGE      -33

#define ABS       -34
#define SQRT      -35
#define EXP       -36
#define SIN       -37
#define COS       -38
#define LOG       -39
#define LOG10     -40
#define ASIN      -41
#define RAND      -42
#define ROUND     -43

#define STRCPY    44
#define STRNCPY   45
#define STRCAT    46
#define STRNCAT   47
#define STRCMP    48
#define STRNCMP   49
#define STRSTR    50
#define STRLEN    51


#define TMSGSEND   -52
#define TMSGRECEIVE -53
#define TJOIN      -54
#define TSLEEP     -55
#define TSEMCREATE -56
#define TSEMWAIT   -57
#define TSEMPOST   -58
#define TCREATE    -59
#define TINIT      -60
#define TDESTROY   -61
#define TEXIT      -62
#define TGETNUM    -63

#define SH_DEFINE     -64   // #define
#define SH_IFDEF      -65   // #ifdef
#define SH_IFNDEF     -66   // #ifndef
#define SH_IF         -67   // #if
#define SH_ELIF       -68   // #elif
#define SH_ENDIF      -69   // #endif
#define SH_ELSE       -70   // #else

#define LVOIDASTER    -150

// Узлы дерева
#define TIdent      -300
#define TConst      -301
#define TString     -302
#define TSliceident -303
#define TSlice      -304
#define TIdenttoval -305
#define TAddrtoval  -306
#define TCall1      -307
#define TCall2      -308
#define TFuncdef    -309
#define TDeclid     -310
#define TInit       -311
#define TExprend    -312
#define TCondexpr   -313
#define TBegin      -314
#define TEnd        -315
#define TIf         -316
#define TWhile      -317
#define TDo         -318
#define TFor        -319
#define TSwitch     -320
#define TCase       -321
#define TDefault    -322
#define TBreak      -323
#define TContinue   -324
#define TReturnvoid -325
#define TReturnval  -326
#define TGoto       -327
#define TLabel      -328
#define TPrint      -329
#define TPrintid    -330
#define TPrintf     -331
#define TGetid      -332
#define TIdenttoaddr -333
#define TSelect     -334
#define TFunidtoval -335
#define TStructbeg  -336
#define TStructend  -337
#define TDeclarr    -338
#define TConstd     -339
#define TIdenttovald -340
#define TAddrtovald -341
#define TBeginit    -342
#define TEndinit    -343

// Коды ошибок

#define after_type_must_be_ident           201
#define wait_right_sq_br                   202
#define only_functions_may_have_type_VOID  203
#define decl_and_def_have_diff_type        204
#define decl_must_start_from_ident_or_decl 205
#define no_comma_in_param_list             206
#define wrong_param_list                   207
#define no_comma_in_type_list              208
#define wrong_type_list                    209
#define func_def_must_be_first             210
#define func_def_must_have_param_list      211
#define def_must_end_with_semicomma        212
#define func_and_protot_have_dif_num_params 213
#define param_types_are_dif                214
#define wait_ident_after_comma_in_decl     215
#define wait_rightbr_in_call               216
#define func_decl_req_params               217
#define wait_while_in_do_stmt              218
#define no_semicolon_after_stmt            219
#define cond_must_be_in_brkts              220
#define repeated_decl                      221
#define arr_init_must_start_from_BEGIN     222
#define no_comma_in_init_list              223
#define ident_is_not_declared              224
#define no_rightsqbr_in_slice              225
#define void_in_expr                       226
#define index_must_be_int                  227
#define slice_not_from_array               228
#define call_not_from_function             229
#define no_comma_in_act_params             230
#define float_instead_int                  231
#define wrong_number_of_params             232
#define wait_rightbr_in_primary            233
#define unassignable_inc                   234
#define wrong_addr                         235
#define no_colon_in_cond_expr              236
#define not_assignable                     237
#define func_not_in_call                   238
#define no_colon_in_case                   239
#define case_after_default                 240
#define no_ident_after_goto                241
#define no_leftbr_in_for                   242
#define no_semicolon_in_for                243
#define no_rightbr_in_for                  244
#define int_op_for_float                   245
#define assmnt_float_to_int                246
#define more_than_1_main                   247
#define no_main_in_program                 248
#define no_leftbr_in_printid               249
#define no_rightbr_in_printid              250
#define no_ident_in_printid                251
#define float_in_switch                    252
#define init_int_by_float                  253
#define must_be_digit_after_dot            254
#define no_leftbr_in_setmotor              255
#define no_rightbr_in_setmotor             256
#define no_comma_in_setmotor               257
#define param_setmotor_not_int             258
#define no_leftbr_in_sleep                 259
#define no_rightbr_in_sleep                260
#define no_leftbr_in_stand_func            261
#define no_rightbr_in_stand_func           262
#define bad_param_in_stand_func            263
#define no_ret_in_func                     264
#define bad_type_in_ret                    265
#define notvoidret_in_void_func            266
#define bad_escape_sym                     267
#define no_right_apost                     268
#define decl_after_strmt                   269
#define too_long_string                    270
#define no_ident_after_aster               271
#define aster_before_func                  272
#define aster_not_for_pointer              273
#define aster_with_row                     274
#define float_in_condition                 275
#define wrong_fun_as_param                 276
#define no_right_br_in_paramfun            277
#define no_ident_in_paramfun               278
#define par_type_void_with_nofun           279
#define ident_in_declarator                280
#define array_before_func                  281
#define wait_definition                    282
#define wait_declarator                    283
#define two_idents_for_1_declarer          284
#define function_has_no_body               285
#define declarator_in_call                 286
#define diff_formal_param_type_and_actual  287
#define case_or_default_not_in_switch      288
#define break_not_in_loop_or_switch        289
#define continue_not_in_loop               290
#define not_primary                        291
#define wrong_operand                      292
#define must_be_digit_after_exp            293
#define label_not_declared                 294
#define repeated_label                     295
#define wrong_pnt_assn                     296
#define comm_not_ended                     297
#define operand_is_pointer                 298
#define pointer_in_print                   299
#define wrong_struct                       300
#define after_dot_must_be_ident            301
#define field_not_found                    302
#define get_field_not_from_struct          303
#define get_field_not_from_struct_pointer  304
#define get_field_not_from_struct_pointer1 399
#define error_in_array_initialization      305
#define error_in_initialization            306
#define type_missmatch                     307
#define array_assigment                    308 
#define wrong_struct_ass                   309
#define not_enough_expr                    310
#define wrong_init                         311
#define wrong_array_init                   312
#define too_many_elems                     313
#define no_field                           314
#define slice_from_func                    315
#define bad_toval                          316
#define wait_end                           317
#define act_param_not_ident                318
#define unassignable                       319
#define pnt_before_array                   320
#define array_size_must_be_int             321
#define no_semicomma_in_struct             322
#define wait_ident_after_semicomma_in_struct 323
#define empty_init                         324
#define ident_not_type                     325
#define not_decl                           326
#define predef_but_notdef                  327
#define print_without_br                   328
#define select_not_from_struct             329
#define select_from_func_value             330
#define init_not_struct                    331
#define param_threads_not_int              332

#define else_after_elif                    333
#define sh_if_not_found                    334
#define no_ident_after_define              335
#define endif_not_found                    336
#define macro_params_not_found             337
#define wait_ident_after_comma_in_macro_params 338
#define wait_rightbr_in_macro_params       339
#define params_count_not_equals_in_macro   340
#define wrong_arg_in_send                  341
#define wrong_arg_in_create                342

#define no_leftbr_in_printf                343
#define no_rightbr_in_printf               344
#define wrong_first_printf_param           345
#define wrong_printf_param_type            346
#define wrong_printf_param_number          347
#define printf_no_format_placeholder       348
#define printf_unknown_format_placeholder  349
#define too_many_printf_params             350

#define no_mult_in_cast                    351
#define no_rightbr_in_cast                 352
#define not_pointer_in_cast                353
#define empty_bound_without_init           354
#define begin_with_notarray                355
#define string_and_notstring               356
#define wrong_init_in_actparam             357
#define no_comma_or_end                    358
#define no_ident_in_define                 359
#define not_int_in_define                  360
#define getdigsensorerr                    361

//  коды предупреждений

#define too_long_int                       400
#endif
