
#define DSP 14         // число умножений в 1 такте
#define N  4		
#define MiN #eval(DSP/N)       // сколько потребуется итераций для всех умножений

#define concat(a,b) a@b
#define R(i,j) concat(r,concat(i,concat(x,j)))
#define A(i,j) concat(a,concat(i,concat(x,j)))


#define mult(a,b) let x = sext(a,30) * sext(b,30) in x{29} || x{4:17}

#define multk  R(i,j) := mult(R(i,j), A(i,j)) |

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

loopij(multk)


