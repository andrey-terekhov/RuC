//
//  Defs.h
//  RuC
//
//  Created by Andrey Terekhov on 03/06/14.
//  Copyright (c) 2014 Andrey Terekhov. All rights reserved.
//

#ifndef RuC_Defs_h
#define RuC_Defs_h

#define MAXREPRTAB 10000
#define MAXIDENTAB 10000
#define MAXMEMSIZE 10000
#define MAXTREESIZE 10000
#define MAXMODETAB 10000
#define MAXBOUNDS  1000
#define FUNCSIZE   1000
#define SOURCESIZE 10000
#define LINESSIZE  3000
#define MAXSTRINGL 128
#define INIPROSIZE 1000

// modetab 
#define MFUNCTION 1001
#define MSTRUCT   1002
#define MARRAY    1003
#define MPOINT    1004

// Лексемы операций языка С

#define ASS        9001
#define REMASS     9002
#define SHLASS     9003
#define SHRASS     9004
#define ANDASS     9005
#define EXORASS    9006
#define ORASS      9007

#define PLUSASS    9008
#define MINUSASS   9009
#define MULTASS    9010
#define DIVASS     9011

#define ASSAT      9012   // эти 22 операции с присваиванием оставляют значение на стеке
#define REMASSAT   9013
#define SHLASSAT   9014
#define SHRASSAT   9015
#define ANDASSAT   9016
#define EXORASSAT  9017
#define ORASSAT    9018

#define PLUSASSAT  9019
#define MINUSASSAT 9020
#define MULTASSAT  9021
#define DIVASSAT   9022

#define ASSV       9201
#define REMASSV    9202
#define SHLASSV    9203
#define SHRASSV    9204
#define ANDASSV    9205
#define EXORASSV   9206
#define ORASSV     9207

#define PLUSASSV   9208
#define MINUSASSV  9209
#define MULTASSV   9210
#define DIVASSV    9211

#define ASSATV     9212   // а эти 22 операции с присваиванием не оставляют значение на стеке
#define REMASSATV  9213
#define SHLASSATV  9214
#define SHRASSATV  9215
#define ANDASSATV  9216
#define EXORASSATV 9217
#define ORASSATV   9218

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


#define PLUSASSR   9058
#define MINUSASSR  9059
#define MULTASSR   9060
#define DIVASSR    9061

#define PLUSASSATR 9069
#define MINUSASSATR 9070
#define MULTASSATR 9071
#define DIVASSATR  9072

#define PLUSASSRV  9258
#define MINUSASSRV 9259
#define MULTASSRV  9260
#define DIVASSRV   9261

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

//#define DECX    9451
//#define PLUSSEL 9452       // добавление смещения к адресу структуры, получается адрес поля
#define NOP     9453
#define DEFARR  9454
#define LI      9455
#define LOAD    9456
#define LAT     9457       // это L@
#define STOP    9458
#define SELECT  9459       // операнд - смещение, которое эта команда добавит к верхушке стека
#define FUNCBEG 9460       // обозначает начало кода функции
#define LA      9461
#define CALL1   9462
#define CALL2   9463
#define _RETURN 9464
#define RETURNV 9465
#define B       9466
#define BE0     9467
#define BNE0    9468
#define SLICE   9469
#define ARRINIT 9470
#define WIDEN   9471
#define WIDEN1  9472
#define _DOUBLE 9473
#define STRUCTINIT 9474
#define STRUCTWITHARR 9475

#define COPY00   9300
#define COPY01   9301
#define COPY10   9302
#define COPY11   9303
#define COPY0ST  9304
#define COPY1ST  9305
#define COPY0STASS 9306
#define COPY1STASS 9307


// Коды операций стандартных функций

#define ABSIC     9525
#define GETDIGSENSORC 9526
#define GETANSENSORC  9527
#define ABSC      9528
#define SQRTC     9529
#define EXPC      9530
#define SINC      9531
#define COSC      9532
#define LOGC      9533
#define LOG10C    9534
#define ASINC     9535
#define RANDC     9536
#define ROUNDC    9537


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

#define LMAIN   0
#define LINT   -1
#define LCHAR  -2
#define LFLOAT -3
#define LVOID  -4

#define LBREAK -5
#define LCASE  -6
#define LCONTINUE -7
#define LDEFAULT  -8
#define LDO    -9
#define LELSE  -10
#define LENUM  -11
#define LSTRUCT -12
#define LTYPEDEF -13
#define LFOR   -14
#define LGOTO  -15
#define LIF    -16
#define LRETURN -17
#define LSIZEOF -18
#define LSWITCH -19
#define LWHILE  -20
#define PRINTID -21
#define PRINT   -22
#define GETID   -23
#define SETMOTOR  -24
#define SLEEP     -25     // до этого места операторы, а затем функции
#define GETDIGSENSOR -26
#define GETANSENSOR -27
#define ABS      -28
#define SQRT     -29
#define EXP      -30
#define SIN      -31
#define COS      -32
#define LOG      -33
#define LOG10    -34
#define ASIN     -35
#define RAND     -36
#define ROUND    -37


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
#define TReturn     -325
#define TReturnval  -326
#define TGoto       -327
#define TLabel      -328
#define TPrint      -329
#define TPrintid    -330
#define TGetid      -331
#define TIdenttoaddr -332
#define TSelect     -333
#define TFunidtoval -334
#define TStructbeg  -335
#define TStructend  -336
#define TDeclarr    -337

// Коды ошибок
#define INTERNAL_COMPILER_ERROR            -1
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

#endif
