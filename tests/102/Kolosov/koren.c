float x, a, eps=1e-6;

void main ()
{
    getid (a);
    x = a;
    do
        x = 0.5 * ( x + a / x );
    while (abs(x * x - a) > eps);
    printid (x);
}