//!!!!=====// Virtex VLX240T 240,000 лог ячеек; 768 DSP48 (умножение с накоплением); 832 BRAM по 2 кбайта
using bincompl;
process Neuronet =
begin
in  inp(int(15), int(15), int(15), int(15));
out ans(int(15));
#define DSP 14         // число умножений в 1 такте
#define B  830    	// число брамов 
#define L 3 		// число слоев
#define N  4		// число нейронов д.б. кратно 4 
#define M  8		// число входов д.б. кратно 4
#define logMiN 2
#define MiN #eval(DSP/N)       // кол-во умножений в такт в каждом нейроне
#define rot #eval(M/MiN)       // сколько потребуется итераций для всех умножений
#if #eval(rot*MiN) != M//!!!! $eval
#set rot #eval(rot+1)
#endif                                                         
#define MM #eval(rot*MiN)
//-- M N MM MiN logMiN rot
#define concat(a,b) a@b
#define R(i,j) concat(r,concat(i,concat(x,j)))
//#define A(i,j) concat(a,concat(i,concat(x,j)))
//#define IN(i) concat(in,i)// побел перед concat
//#define S(i) concat(s,i)



//#define macroIN ,IN(j)
//#define macroS ,S(j)




// определяем регистры Rij, считаем, что они содержат числа со знаком с 4 знаками после запятой
#define i 0
#while i < N
#define j 0
#while j < MiN
, R(i,j)
#set j #eval(j+1)
#endw
#undef j
#set i #eval(i+1)
#endw
#undef i
