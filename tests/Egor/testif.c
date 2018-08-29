#define a 5
#ifdef a
#define abc 45
#endif

void main()
{
    int n = abc + 5;
    printid(n);
}
