#define A 5 
#define B 3 
#define C 7 
#define D 1 

#define concat(a, b) a@b
#define case1(t) concat(concat(int a, t), ; )
#define case2(t) concat(concat(double b, t), ; )

#macro cost(M, N)
    #define j 0
    #while j < M 
        #ifdef INTEGER 
            case1(#eval(j + N))
        #else
            case2(#eval(j + N))
        #endif
        #set j #eval(j + 1)
    #endw
    #undef j 
#endm

#if #eval(A + B) == #eval(C + D) || A > C
    #define INTEGER 0
    cost(A, B)
#else
    cost(C, D)
#endif

void main()
{
// code
}