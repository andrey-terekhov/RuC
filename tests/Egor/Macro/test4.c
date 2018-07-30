#define s(a, b, c) a + b - c
#define k(a, b) a + b
#define l(a, b) a - k(a, b)

void main()
{
s(5, 6, 10); // строчный коментарий
/*
длинный коментарий 
*/
k(1, 0);
l(11, 12);
}


