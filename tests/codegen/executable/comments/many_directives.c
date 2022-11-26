#define s(a, b, c) a + b - c

#define k(a, b) a + b

#define l(a, b) a - k(a, b)

#define A 5
#define B 6

#define C 7
void main()
{
    int r = s(5, 6, 10); 

    assert(r == 1, "r must be 1");

    /*
    длинный коментарий 
    */

    assert(k(1, 0) == 1, "k(1, 0) must be 1");
    assert(l(11, 12) == 12, "l(11, 12) must be 12");
}