#define s(a, b, c) a + b - c

#define k(a, b) a + b

#define l(a, b) a - k(a, b)

#define A 5
#define B 6

#define C 7
void main()
{
    int r = s(5, 6, 10);
    /*
    длинный коментарий
    */
    k(1, 0);
    l(11, 12);
}