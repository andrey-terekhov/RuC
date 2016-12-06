float f (float x)
{
    return sin (x);
}
void main()
{
    float a,b,d,S;
    int i = 0;
    d = 1e-2;
    S = 0;
    getid (a);
    getid (b);
    while (a + i * d < b)
    {
        S += d * f(a + i * d);
        i++;
    } 
    printid (S); 
}    


