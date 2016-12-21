float eps = 1e-5, a, b, S, S0, L;
int i, n;

float f (float x)
{
   return sin (x);
}


void main()
{   
    getid (a);
    getid (b);
    n = 25;
    S0 = 1;
    S = 0;
    while (abs (S - S0) > eps)
        {   
            float x1, x2, h;
            L = (abs (a - b)) / n;
            x2 = a;
            S0 = S;
            S = 0;
            for (i = 1; i < n; i++)
            {
                x1 = x2 ;
                x2 = x1 + L;
                h = f(x1);
                S += abs ((x2 - x1)*h);
            }
            n = 3*n;
        }
    printid (S);
}
