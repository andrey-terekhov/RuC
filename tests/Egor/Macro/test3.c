#define s(a, b, c) a + b - c
#define k(a, b) a + b
#define l(a, b) a - k(a, b)
#define E 9

void main()
{
int a = s(E, 6, 10);
int b = k(1, k(3, 4));
int c = l(11, 12);
}


