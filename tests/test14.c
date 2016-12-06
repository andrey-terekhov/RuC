int a,   b;
void main()
{
    switch (a)
    {
        case 1:
            a = 1;
            break;
        case 2:
            a = 2;
            return a;
        case 3:
            a = 3;
            break;            
        default:
            b=a;
    }
    a=b;
}
