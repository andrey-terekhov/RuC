float f (float x, float (*g) (float))
{
    return g(x);
}
float gg(float x)
{
    return sin(x);
}
void main ()
{
    print (f(1.57,gg));
}
