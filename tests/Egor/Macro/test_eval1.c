=====// Virtex VLX240T 240,000 лог ячеек; 768 DSP48 (умножение с накоплением); 832 BRAM по 2 кбайта
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
-- M N MM MiN logMiN rot
#define concat(a,b) a@b//!!!!
#define R(i,j) concat(r,concat(i,concat(x,j)))
#define A(i,j) concat(a,concat(i,concat(x,j)))
#define IN(i) concat(in,i)////!!!! побел перед concat
#define S(i) concat(s,i)

data JT, JK, JD, J, II: uint(10),
irot, layer: uint(4),
tab: [0..15:uint(4)]int(15), 
bram: [0..B: uint(10)][0..1023: uint(10)]int(15)

#define macroIN ,IN(j)//!!!!
#define macroS ,S(j)

#macro loopj(M,loopbody)
#define j 0
#while j < M   
loopbody
#set j #eval(j+1)
#endw
#undef j 
#endm

loopj(MM, macroIN)
loopj(N, macroS)

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

: int(15);
#define macroS0 | S(j):=0 
init
{
JT:=0 | JK:=0 | JD:=0 | II:=0 | J:=0 | irot:=0 | layer:=1//!!!!ошибка
loopj(N, macroS0)
}

inp(a,b,c,d)
{if JT < 16 then
tab[JT]:=a | tab[JT+1]:=b | tab[JT+2]:=c | tab[JT+3]:=d | JT:=JT + 4

elif JK < #eval(rot * N) then
bram[II][JK]:=a | bram[II+1][JK]:=b | bram[II+2][JK]:=c | bram[II+3][JK]:=d | II:=II+4 |
if II == #eval(MiN*N-4) then II:=0 | JK:=JK + 1 else II:=II+4 fi 

elif JD < M
#define j 4
#while j < M
IN(#eval(j-4)) := IN(j) | IN(#eval(j-3)) := IN(#eval(j+1)) | 
IN(#eval(j-2)) := IN(#eval(j+2)) | IN(#eval(j-1)) := IN(#eval(j+3)) |
#set j #eval(j+4)
#endw
#undef j
IN(#eval(M-4)) := a | IN(#eval(M-3)) := b | IN(#eval(M-2)) := c | IN(#eval(M-1)) := d | JD:=JD+4

else
while layer < L
do
while irot < rot
do
 
#define j 0
#while j < MiN
#define i 0
#while i < N
R(i,j):=IN(j) |
#set i #eval(i+1)
#endw
#undef i
#set j #eval(j+1)
#endw
#undef j

#macro macroShift() IN(j):=IN(#eval(MiN+j)) |#endm //!!!! раньше было не хорошо
 
loopj(#eval(MM-MiN), macroShift)
#macro readbram() A(i,j) = bram[#eval(j*N+i)][J] |#endm//!!!!

//#define multk  R(i,j) := mult(R(i,j), A(i,j)) |

#define mult(a,b) let x = sext(a,30) * sext(b,30) in x{29} || x{4:17}

#define multk  R(i,j) := mult(R(i,j), A(i,j)) |//!!!!

#macro loopij(loopbody)
#define j 0
#while j < MiN
#define i 0
#while i < N
loopbody
#set i #eval(i+1)
#endw
#undef i
#set j #eval(j+1)
#endw
#undef j
#endm

// читаем MiN коэффициентов в каждом нейроне
loopij(readbram)
skip;
// умножаем на коэффициенты все входы всех нейронов слоя
loopij(multk)
first:=0 |irot:= irot + 1 | J:=J + 1;//ошибка

// суммируем произведения по всем входам внутри каждого нейрона
#define step 1
#define c 0
#while c < logMiN
#define j 0
#while j < MiN
#define i 0
#while i < N

#if #eval(j+step) < MiN
 R(i,j):=R(i,j) + R(i,#eval(j+step)) |
#endif

#set i #eval(i+1)                                      
#endw
#undef i
#set j #eval(j+2*step)
#endw
skip;
#undef j
#set step #eval(2*step)
#set c #eval(c+1)
#endw
#undef c
#undef step
#define macroSUM S(j):= S(j) + R(j,0) |
loopj(N, macroSUM)
skip
done;
irot:=0 |
#define p4 ({4}: int(15))
#define m4 ({-4}:int(15))
// вычисляем значение пороговой функции и выдаем результаты на след слой
#define i 0
#while i < N
let s = 
if   S(i) < m4 then tab[0]
elif S(i) > p4 then tab[15]
else tab[S(i){3:6}]
fi in
IN(i):=s |
#set i #eval(i+1)
#endw
#undef i
layer:= layer + 1
done;
JD:=0 | inform ans(S(0))
end;//!!!!