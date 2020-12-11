/*
 *	Copyright 2020 Andrey Terekhov, Maxim Menshikov
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


#ifdef __cplusplus
extern "C" {
#endif

const char *const KEYWORDS =
	"MAIN main ГЛАВНАЯ главная\n"
	"INT int ЦЕЛ цел\n"
	"CHAR char ЛИТЕРА литера\n"
	"FLOAT float ВЕЩ вещ\n"
	"LONG long ДЛИН длин\n"
	"DOUBLE double ДВОЙНОЙ двойной\n"
	"VOID void ПУСТО пусто\n"
	"BREAK break ВЫХОД выход\n"
	"CASE case СЛУЧАЙ случай\n"
	"CONTINUE continue ПРОДОЛЖИТЬ продолжить\n"
	"DEFAULT default УМОЛЧАНИЕ умолчание\n"
	"DO do ЦИКЛ цикл\n"
	"ELSE else ИНАЧЕ иначе\n"
	"ENUM enum ПЕРЕЧЕНЬ перечень\n"
	"STRUCT struct СТРУКТУРА структура\n"
	"TYPEDEF typedef ОПРТИПА опртипа\n"
	"FOR for ДЛЯ для\n"
	"GOTO goto ПЕРЕХОД переход\n"
	"IF if ЕСЛИ если\n"
	"RETURN return ВОЗВРАТ возврат\n"
	"SIZEOF sizeof РАЗМЕР размер\n"
	"SWITCH switch ВЫБОР выбор\n"
	"WHILE while ПОКА пока\n"
	"PRINTID printid ПЕЧАТЬИД печатьид\n"
	"PRINT print ПЕЧАТЬ печать\n"
	"PRINTF printf ПЕЧАТЬФ печатьф\n"
	"SCANF scanf ЧИТАТЬФ читатьф\n"
	"GETID getid ЧИТАТЬИД читатьид\n"
	"T_CREATE_DIRECT t_create_direct Н_СОЗДАТЬ_НЕПОСР н_создать_непоср\n"
	"T_EXIT_DIRECT t_exit_direct Н_КОНЕЦ_НЕПОСР н_конец_непоср\n"
	"SETMOTOR setmotor МОТОР мотор\n"
	"GETDIGSENSOR getdigsensor ЦИФРДАТЧИК цифрдатчик\n"
	"GETANSENSOR getansensor АНАЛОГДАТЧИК аналогдатчик\n"
	"SETVOLTAGE setvoltage УСТНАПРЯЖЕНИЕ устнапряжение\n"
	"ABS abs АБС абс\n"
	"SQRT sqrt КВКОР квкор\n"
	"EXP exp ЭКСП эксп\n"
	"SIN sin СИН син\n"
	"COS cos КОС кос\n"
	"LOG log ЛОГ лог\n"
	"LOG10 log10 ЛОГ10 лог10\n"
	"ASIN asin АСИН асин\n"
	"RAND rand СЛУЧ случ\n"
	"ROUND round ОКРУГЛ округл\n"
	"\n"
	"STRCPY strcpy КОПИР_СТРОКУ копир_строку\n"
	"STRNCPY strncpy КОПИР_Н_СИМВ копир_н_симв\n"
	"STRCAT strcat КОНКАТ_СТРОКИ конкат_строки\n"
	"STRNCAT strncat КОНКАТ_Н_СИМВ конкат_н_симв\n"
	"STRCMP strcmp СРАВН_СТРОК сравн_строк\n"
	"STRNCMP strncmp СРАВН_Н_СИМВ сравн_н_симв\n"
	"STRSTR strstr НАЧ_ПОДСТРОК нач_подстрок\n"
	"STRLEN strlen ДЛИНА длина\n"
	"\n"
	"T_MSG_SEND t_msg_send Н_ПОСЛАТЬ н_послать\n"
	"T_MSG_RECEIVE t_msg_receive Н_ПОЛУЧИТЬ н_получить\n"
	"T_JOIN t_join Н_ПРИСОЕД н_присоед\n"
	"T_SLEEP t_sleep Н_СПАТЬ н_спать\n"
	"T_SEM_CREATE t_sem_create Н_СОЗДАТЬ_СЕМ н_создать_сем\n"
	"T_SEM_WAIT t_sem_wait Н_ВНИЗ_СЕМ н_вниз_сем\n"
	"T_SEM_POST t_sem_post Н_ВВЕРХ_СЕМ н_вверх_сем\n"
	"T_CREATE t_create Н_СОЗДАТЬ н_создать\n"
	"T_INIT t_init Н_НАЧАТЬ н_начать\n"
	"T_DESTROY t_destroy Н_ЗАКОНЧИТЬ н_закончить\n"
	"T_EXIT t_exit Н_КОНЕЦ н_конец\n"
	"T_GETNUM t_getnum Н_НОМЕР_НИТИ н_номер_нити\n"
	"\n"
	"#DEFINE #define #ОПРЕД #опред\n"
	"#IFDEF #ifdef #ЕСЛИБЫЛ #еслибыл\n"
	"#IFNDEF #ifndef #ЕСЛИНЕБЫЛ #еслинебыл\n"
	"#IF #if #ЕСЛИ #если\n"
	"#ELIF #elif #ИНЕСЛИ #инесли\n"
	"#ENDIF #endif #КОНЕСЛИ #конесли\n"
	"#ELSE #else #ИНАЧЕ #иначе\n"
	"\n"
	"WIFI_CONNECT wifi_connect WIFI_ПОДКЛЮЧИТЬ wifi_подключить\n"
	"BLYNK_AUTHORIZATION blynk_authorization BLYNK_АВТОРИЗАЦИЯ blynk_авторизация\n"
	"BLYNK_SEND blynk_send BLYNK_ПОСЛАТЬ blynk_послать\n"
	"BLYNK_RECEIVE blynk_receive BLYNK_ПОЛУЧИТЬ blynk_получить\n"
	"BLYNK_NOTIFICATION blynk_notification BLYNK_УВЕДОМЛЕНИЕ blynk_уведомление\n"
	"BLYNK_PROPERTY blynk_property BLYNK_СВОЙСТВО blynk_свойство\n"
	"BLYNK_LCD blynk_lcd BLYNK_ДИСПЛЕЙ blynk_дисплей\n"
	"BLYNK_TERMINAL blynk_terminal BLYNK_ТЕРМИНАЛ blynk_терминал\n"
	"\n"
	"SETSIGNAL setsignal СИГНАЛ сигнал\n"
	"PIXEL pixel ПИКСЕЛЬ пиксель\n"
	"LINE line ЛИНИЯ линия\n"
	"RECTANGLE rectangle ПРЯМОУГОЛЬНИК прямоугольник\n"
	"ELLIPSE ellipse ЭЛЛИПС эллипс\n"
	"CLEAR clear ОЧИСТИТЬ очистить\n"
	"DRAW_STRING draw_string НАРИСОВАТЬ_СТРОКУ нарисовать_строку\n"
	"DRAW_NUMBER draw_number НАРИСОВАТЬ_ЧИСЛО нарисовать_число\n"
	"ICON icon ИКОНКА иконка\n"
	"UPB upb КОЛ_ВО кол_во\n"
	"SEND_INT_TO_ROBOT send_int_to_robot ПОСЛАТЬ_ЦЕЛ_НА_РОБОТ послать_цел_на_робот\n"
	"SEND_FLOAT_TO_ROBOT send_float_to_robot ПОСЛАТЬ_ВЕЩ_НА_РОБОТ послать_вещ_на_робот\n"
	"SEND_STRING_TO_ROBOT send_string_to_robot ПОСЛАТЬ_СТРОКУ_НА_РОБОТ послать_строку_на_робот\n"
	"RECEIVE_INT_FROM_ROBOT receive_int_from_robot ПОЛУЧИТЬ_ЦЕЛ_ОТ_РОБОТА получить_цел_от_робота\n"
	"RECEIVE_FLOAT_FROM_ROBOT receive_float_from_robot ПОЛУЧИТЬ_ВЕЩ_ОТ_РОБОТА получить_вещ_от_робота\n"
	"RECEIVE_STRING_FROM_ROBOT receive_string_from_robot ПОЛУЧИТЬ_СТРОКУ_ОТ_РОБОТА получить_строку_от_робота\n"
	"ASSERT assert ПРОВЕРИТЬ проверить\n";

#ifdef __cplusplus
} /* extern "C" */
#endif
