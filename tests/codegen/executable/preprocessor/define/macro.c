#define concat(a, b) a @ b
#define R(i, j) concat(r, concat(i, concat(x, j)))
#define A(i, j) concat(a, concat(i, concat(x, j)))
#define IN(i)  concat(in, i)
#define S(i)  concat(s, i)



#define macroIN ,IN(j)
#define macroS ,S(j)
void main()
{

} 