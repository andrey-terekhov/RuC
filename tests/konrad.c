void main()
{
    int a=3,b=5;
    float c=3.14;
    c=(a>b) ? a : (c>0) ? c : b;
    printid(c);  // 3.14
}