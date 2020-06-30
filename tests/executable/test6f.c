float a, b[3] = {1.2e-1, 12e-2, 0.0012e2};
int i=2;
void main()
{
    a = 12e-2;
    printid(a);
    printid(b);
    b[i] -= b[1]+=i;
    printid(b);
    
}